/**
 * SEC EDGAR Fraud Analyzer - Altman Z-Score Model Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/models/altman.h>
#include <cmath>
#include <algorithm>

namespace sec_analyzer {

double AltmanModel::safe_divide(double num, double denom, double default_val) {
    if (std::abs(denom) < 1e-10) return default_val;
    return num / denom;
}

double AltmanModel::calculate_x1(const FinancialData& data) {
    // X1 = Working Capital / Total Assets
    double wc = data.balance_sheet.current_assets - data.balance_sheet.current_liabilities;
    return safe_divide(wc, data.balance_sheet.total_assets);
}

double AltmanModel::calculate_x2(const FinancialData& data) {
    // X2 = Retained Earnings / Total Assets
    return safe_divide(data.balance_sheet.retained_earnings, data.balance_sheet.total_assets);
}

double AltmanModel::calculate_x3(const FinancialData& data) {
    // X3 = EBIT / Total Assets (using Operating Income as proxy)
    return safe_divide(data.income_statement.operating_income, data.balance_sheet.total_assets);
}

double AltmanModel::calculate_x4(const FinancialData& data, double market_cap) {
    // X4 = Market Value of Equity / Total Liabilities
    // If market cap not available, use book value of equity
    double equity = (market_cap > 0) ? market_cap : data.balance_sheet.total_equity;
    return safe_divide(equity, data.balance_sheet.total_liabilities);
}

double AltmanModel::calculate_x5(const FinancialData& data) {
    // X5 = Sales / Total Assets
    return safe_divide(data.income_statement.revenue, data.balance_sheet.total_assets);
}

AltmanResult AltmanModel::calculate(const FinancialData& data, double market_cap) {
    AltmanResult result;
    
    // Calculate components
    result.x1 = calculate_x1(data);
    result.x2 = calculate_x2(data);
    result.x3 = calculate_x3(data);
    result.x4 = calculate_x4(data, market_cap);
    result.x5 = calculate_x5(data);
    
    // Calculate Z-Score
    result.z_score = COEF_X1 * result.x1
                   + COEF_X2 * result.x2
                   + COEF_X3 * result.x3
                   + COEF_X4 * result.x4
                   + COEF_X5 * result.x5;
    
    // Interpret
    result.zone = get_zone(result.z_score);
    result.bankruptcy_probability = score_to_probability(result.z_score);
    result.risk_score = probability_to_risk(result.bankruptcy_probability);
    
    return result;
}

std::string AltmanModel::get_zone(double z_score) {
    if (z_score > SAFE_THRESHOLD) return "Safe";
    if (z_score > DISTRESS_THRESHOLD) return "Gray";
    return "Distress";
}

double AltmanModel::score_to_probability(double z_score) {
    // Approximate bankruptcy probability based on Z-Score
    // Using logistic approximation
    if (z_score > 3.0) return 0.01;
    if (z_score > 2.7) return 0.05;
    if (z_score > 2.4) return 0.10;
    if (z_score > 2.0) return 0.20;
    if (z_score > 1.8) return 0.35;
    if (z_score > 1.5) return 0.50;
    if (z_score > 1.2) return 0.65;
    if (z_score > 1.0) return 0.75;
    if (z_score > 0.5) return 0.85;
    return 0.95;
}

double AltmanModel::probability_to_risk(double probability) {
    return std::clamp(probability, 0.0, 1.0);
}

// Z'' Score for non-manufacturing
AltmanResult AltmanZPrimeModel::calculate(const FinancialData& data) {
    AltmanResult result;
    
    // Calculate components (same as Z-Score but X4 uses book value)
    double wc = data.balance_sheet.current_assets - data.balance_sheet.current_liabilities;
    result.x1 = (data.balance_sheet.total_assets > 0) ? 
                wc / data.balance_sheet.total_assets : 0;
    result.x2 = (data.balance_sheet.total_assets > 0) ? 
                data.balance_sheet.retained_earnings / data.balance_sheet.total_assets : 0;
    result.x3 = (data.balance_sheet.total_assets > 0) ? 
                data.income_statement.operating_income / data.balance_sheet.total_assets : 0;
    result.x4 = (data.balance_sheet.total_liabilities > 0) ? 
                data.balance_sheet.total_equity / data.balance_sheet.total_liabilities : 0;
    result.x5 = 0; // Not used in Z'' model
    
    // Calculate Z''-Score (different coefficients, no X5)
    result.z_score = 6.56 * result.x1
                   + 3.26 * result.x2
                   + 6.72 * result.x3
                   + 1.05 * result.x4;
    
    // Interpret with different thresholds
    if (result.z_score > SAFE_THRESHOLD) {
        result.zone = "Safe";
    } else if (result.z_score > DISTRESS_THRESHOLD) {
        result.zone = "Gray";
    } else {
        result.zone = "Distress";
    }
    
    result.bankruptcy_probability = AltmanModel::score_to_probability(result.z_score);
    result.risk_score = std::clamp(result.bankruptcy_probability, 0.0, 1.0);
    
    return result;
}

} // namespace sec_analyzer
