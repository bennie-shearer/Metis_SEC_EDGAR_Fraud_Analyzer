/**
 * SEC EDGAR Fraud Analyzer - Piotroski F-Score Model Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/models/piotroski.h>
#include <cmath>
#include <algorithm>

namespace sec_analyzer {

double PiotroskiModel::calculate_roa(const FinancialData& data) {
    if (std::abs(data.balance_sheet.total_assets) < 1e-10) return 0;
    return data.income_statement.net_income / data.balance_sheet.total_assets;
}

double PiotroskiModel::calculate_leverage(const FinancialData& data) {
    if (std::abs(data.balance_sheet.total_assets) < 1e-10) return 0;
    return data.balance_sheet.long_term_debt / data.balance_sheet.total_assets;
}

double PiotroskiModel::calculate_current_ratio(const FinancialData& data) {
    if (std::abs(data.balance_sheet.current_liabilities) < 1e-10) return 0;
    return data.balance_sheet.current_assets / data.balance_sheet.current_liabilities;
}

double PiotroskiModel::calculate_gross_margin(const FinancialData& data) {
    if (std::abs(data.income_statement.revenue) < 1e-10) return 0;
    return data.income_statement.gross_profit / data.income_statement.revenue;
}

double PiotroskiModel::calculate_asset_turnover(const FinancialData& data) {
    if (std::abs(data.balance_sheet.total_assets) < 1e-10) return 0;
    return data.income_statement.revenue / data.balance_sheet.total_assets;
}

int PiotroskiModel::check_roa_positive(const FinancialData& current) {
    return (current.income_statement.net_income > 0) ? 1 : 0;
}

int PiotroskiModel::check_cfo_positive(const FinancialData& current) {
    return (current.cash_flow.operating_cash_flow > 0) ? 1 : 0;
}

int PiotroskiModel::check_roa_increasing(const FinancialData& current, const FinancialData& prior) {
    return (calculate_roa(current) > calculate_roa(prior)) ? 1 : 0;
}

int PiotroskiModel::check_quality_of_earnings(const FinancialData& current) {
    // CFO > Net Income indicates higher quality earnings
    return (current.cash_flow.operating_cash_flow > current.income_statement.net_income) ? 1 : 0;
}

int PiotroskiModel::check_leverage_decreasing(const FinancialData& current, const FinancialData& prior) {
    return (calculate_leverage(current) < calculate_leverage(prior)) ? 1 : 0;
}

int PiotroskiModel::check_liquidity_increasing(const FinancialData& current, const FinancialData& prior) {
    return (calculate_current_ratio(current) > calculate_current_ratio(prior)) ? 1 : 0;
}

int PiotroskiModel::check_no_dilution(const FinancialData& current, const FinancialData& prior) {
    // No new shares issued (shares outstanding didn't increase)
    return (current.balance_sheet.shares_outstanding <= prior.balance_sheet.shares_outstanding) ? 1 : 0;
}

int PiotroskiModel::check_gross_margin_increasing(const FinancialData& current, const FinancialData& prior) {
    return (calculate_gross_margin(current) > calculate_gross_margin(prior)) ? 1 : 0;
}

int PiotroskiModel::check_asset_turnover_increasing(const FinancialData& current, const FinancialData& prior) {
    return (calculate_asset_turnover(current) > calculate_asset_turnover(prior)) ? 1 : 0;
}

PiotroskiResult PiotroskiModel::calculate(const FinancialData& current, const FinancialData& prior) {
    PiotroskiResult result;
    
    // Profitability signals (4 points)
    result.roa_positive = check_roa_positive(current);
    result.cfo_positive = check_cfo_positive(current);
    result.roa_increasing = check_roa_increasing(current, prior);
    result.cfo_greater_than_ni = check_quality_of_earnings(current);
    
    // Leverage/Liquidity signals (3 points)
    result.leverage_decreasing = check_leverage_decreasing(current, prior);
    result.current_ratio_increasing = check_liquidity_increasing(current, prior);
    result.no_dilution = check_no_dilution(current, prior);
    
    // Operating efficiency signals (2 points)
    result.gross_margin_increasing = check_gross_margin_increasing(current, prior);
    result.asset_turnover_increasing = check_asset_turnover_increasing(current, prior);
    
    // Calculate total F-Score
    result.f_score = result.roa_positive
                   + result.cfo_positive
                   + result.roa_increasing
                   + result.cfo_greater_than_ni
                   + result.leverage_decreasing
                   + result.current_ratio_increasing
                   + result.no_dilution
                   + result.gross_margin_increasing
                   + result.asset_turnover_increasing;
    
    // Interpret
    result.interpretation = get_interpretation(result.f_score);
    result.risk_score = score_to_risk(result.f_score);
    
    return result;
}

std::string PiotroskiModel::get_interpretation(int f_score) {
    if (f_score >= STRONG_THRESHOLD) return "Strong";
    if (f_score > WEAK_THRESHOLD) return "Moderate";
    return "Weak";
}

double PiotroskiModel::score_to_risk(int f_score) {
    // Lower F-Score = higher risk
    // Map 0-9 to 1.0-0.0 risk
    return std::clamp(1.0 - (f_score / 9.0), 0.0, 1.0);
}

} // namespace sec_analyzer
