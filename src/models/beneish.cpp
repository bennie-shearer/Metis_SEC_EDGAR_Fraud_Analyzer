/**
 * SEC EDGAR Fraud Analyzer - Beneish M-Score Model Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/models/beneish.h>
#include <cmath>
#include <algorithm>

namespace sec_analyzer {

double BeneishModel::safe_divide(double num, double denom, double default_val) {
    if (std::abs(denom) < 1e-10) return default_val;
    return num / denom;
}

double BeneishModel::calculate_dsri(const FinancialData& current, const FinancialData& prior) {
    // DSRI = (Receivables_t / Sales_t) / (Receivables_t-1 / Sales_t-1)
    double current_ratio = safe_divide(current.balance_sheet.accounts_receivable, 
                                       current.income_statement.revenue);
    double prior_ratio = safe_divide(prior.balance_sheet.accounts_receivable, 
                                     prior.income_statement.revenue);
    return safe_divide(current_ratio, prior_ratio);
}

double BeneishModel::calculate_gmi(const FinancialData& current, const FinancialData& prior) {
    // GMI = Gross Margin_t-1 / Gross Margin_t
    double current_gm = current.income_statement.gross_margin();
    double prior_gm = prior.income_statement.gross_margin();
    return safe_divide(prior_gm, current_gm);
}

double BeneishModel::calculate_aqi(const FinancialData& current, const FinancialData& prior) {
    // AQI = [1 - (CA_t + PPE_t) / TA_t] / [1 - (CA_t-1 + PPE_t-1) / TA_t-1]
    double current_aq = 1.0 - safe_divide(
        current.balance_sheet.current_assets + current.balance_sheet.ppe,
        current.balance_sheet.total_assets, 0.0);
    double prior_aq = 1.0 - safe_divide(
        prior.balance_sheet.current_assets + prior.balance_sheet.ppe,
        prior.balance_sheet.total_assets, 0.0);
    return safe_divide(current_aq, prior_aq);
}

double BeneishModel::calculate_sgi(const FinancialData& current, const FinancialData& prior) {
    // SGI = Sales_t / Sales_t-1
    return safe_divide(current.income_statement.revenue, prior.income_statement.revenue);
}

double BeneishModel::calculate_depi(const FinancialData& current, const FinancialData& prior) {
    // DEPI = Depreciation Rate_t-1 / Depreciation Rate_t
    double current_rate = safe_divide(current.income_statement.depreciation,
        current.income_statement.depreciation + current.balance_sheet.ppe);
    double prior_rate = safe_divide(prior.income_statement.depreciation,
        prior.income_statement.depreciation + prior.balance_sheet.ppe);
    return safe_divide(prior_rate, current_rate);
}

double BeneishModel::calculate_sgai(const FinancialData& current, const FinancialData& prior) {
    // SGAI = (SGA_t / Sales_t) / (SGA_t-1 / Sales_t-1)
    double current_ratio = safe_divide(current.income_statement.sga_expense,
                                       current.income_statement.revenue);
    double prior_ratio = safe_divide(prior.income_statement.sga_expense,
                                     prior.income_statement.revenue);
    return safe_divide(current_ratio, prior_ratio);
}

double BeneishModel::calculate_lvgi(const FinancialData& current, const FinancialData& prior) {
    // LVGI = Leverage_t / Leverage_t-1
    double current_lev = safe_divide(current.balance_sheet.total_liabilities,
                                     current.balance_sheet.total_assets);
    double prior_lev = safe_divide(prior.balance_sheet.total_liabilities,
                                   prior.balance_sheet.total_assets);
    return safe_divide(current_lev, prior_lev);
}

double BeneishModel::calculate_tata(const FinancialData& current) {
    // TATA = (Working Capital Change - Cash Change - Depreciation) / Total Assets
    double accruals = current.income_statement.net_income - current.cash_flow.operating_cash_flow;
    return safe_divide(accruals, current.balance_sheet.total_assets, 0.0);
}

BeneishResult BeneishModel::calculate(const FinancialData& current, const FinancialData& prior) {
    BeneishResult result;
    
    // Calculate components
    result.dsri = calculate_dsri(current, prior);
    result.gmi = calculate_gmi(current, prior);
    result.aqi = calculate_aqi(current, prior);
    result.sgi = calculate_sgi(current, prior);
    result.depi = calculate_depi(current, prior);
    result.sgai = calculate_sgai(current, prior);
    result.lvgi = calculate_lvgi(current, prior);
    result.tata = calculate_tata(current);
    
    // Calculate M-Score
    result.m_score = INTERCEPT
        + COEF_DSRI * result.dsri
        + COEF_GMI * result.gmi
        + COEF_AQI * result.aqi
        + COEF_SGI * result.sgi
        + COEF_DEPI * result.depi
        + COEF_SGAI * result.sgai
        + COEF_TATA * result.tata
        + COEF_LVGI * result.lvgi;
    
    // Interpret
    result.likely_manipulator = is_likely_manipulator(result.m_score);
    result.zone = get_zone(result.m_score);
    result.risk_score = probability_to_risk(score_to_probability(result.m_score));
    result.flags = generate_flags(result);
    
    return result;
}

std::string BeneishModel::get_zone(double m_score) {
    if (m_score > -1.78) return "High Risk";
    if (m_score > THRESHOLD) return "Elevated Risk";
    if (m_score > -2.50) return "Moderate Risk";
    return "Low Risk";
}

double BeneishModel::score_to_probability(double m_score) {
    // Convert M-Score to manipulation probability using logistic function
    return 1.0 / (1.0 + std::exp(-(m_score + 2.22)));
}

double BeneishModel::probability_to_risk(double probability) {
    return std::clamp(probability, 0.0, 1.0);
}

std::vector<std::string> BeneishModel::generate_flags(const BeneishResult& result) {
    std::vector<std::string> flags;
    
    if (result.dsri > 1.465) {
        flags.push_back("High Days Sales in Receivables - potential revenue manipulation");
    }
    if (result.gmi > 1.193) {
        flags.push_back("Declining gross margins - pressure to manipulate");
    }
    if (result.aqi > 1.254) {
        flags.push_back("Increasing non-current assets - potential capitalization abuse");
    }
    if (result.sgi > 1.607) {
        flags.push_back("Rapid sales growth - higher manipulation risk");
    }
    if (result.tata > 0.018) {
        flags.push_back("High accruals relative to assets - earnings quality concern");
    }
    if (result.lvgi > 1.111) {
        flags.push_back("Increasing leverage - financial pressure");
    }
    
    return flags;
}

} // namespace sec_analyzer
