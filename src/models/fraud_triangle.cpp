/**
 * SEC EDGAR Fraud Analyzer - Fraud Triangle Model Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/models/fraud_triangle.h>
#include <cmath>
#include <algorithm>

namespace sec_analyzer {

double FraudTriangleModel::normalize_score(double raw_score, double max_indicators) {
    if (max_indicators <= 0) return 0;
    return std::clamp(raw_score / max_indicators, 0.0, 1.0);
}

bool FraudTriangleModel::check_declining_revenue(const std::vector<FinancialData>& financials) {
    if (financials.size() < 2) return false;
    int declining_periods = 0;
    for (size_t i = 1; i < financials.size(); ++i) {
        if (financials[i].income_statement.revenue < financials[i-1].income_statement.revenue) {
            declining_periods++;
        }
    }
    return declining_periods >= static_cast<int>(financials.size()) / 2;
}

bool FraudTriangleModel::check_declining_margins(const std::vector<FinancialData>& financials) {
    if (financials.size() < 2) return false;
    int declining_periods = 0;
    for (size_t i = 1; i < financials.size(); ++i) {
        double current_margin = financials[i].income_statement.gross_margin();
        double prior_margin = financials[i-1].income_statement.gross_margin();
        if (current_margin < prior_margin) {
            declining_periods++;
        }
    }
    return declining_periods >= static_cast<int>(financials.size()) / 2;
}

bool FraudTriangleModel::check_high_leverage(const FinancialData& data) {
    double debt_ratio = data.balance_sheet.debt_ratio();
    return debt_ratio > 0.6; // Debt > 60% of assets
}

bool FraudTriangleModel::check_negative_cash_flow(const FinancialData& data) {
    return data.cash_flow.operating_cash_flow < 0;
}

bool FraudTriangleModel::check_earnings_miss_pattern(const std::vector<FinancialData>& financials) {
    // Check for pattern of barely meeting targets (suspicious consistency)
    if (financials.size() < 3) return false;
    int near_misses = 0;
    for (const auto& f : financials) {
        // Net margin between 0-2% might indicate earnings management
        double margin = f.income_statement.net_margin();
        if (margin > 0 && margin < 0.02) {
            near_misses++;
        }
    }
    return near_misses >= 2;
}

std::vector<std::string> FraudTriangleModel::detect_pressure_indicators(const std::vector<FinancialData>& financials) {
    std::vector<std::string> indicators;
    
    if (financials.empty()) return indicators;
    
    if (check_declining_revenue(financials)) {
        indicators.push_back("Declining revenue trend");
    }
    if (check_declining_margins(financials)) {
        indicators.push_back("Declining profit margins");
    }
    if (check_high_leverage(financials[0])) {
        indicators.push_back("High leverage ratio");
    }
    if (check_negative_cash_flow(financials[0])) {
        indicators.push_back("Negative operating cash flow");
    }
    if (check_earnings_miss_pattern(financials)) {
        indicators.push_back("Pattern of barely meeting earnings targets");
    }
    
    return indicators;
}

double FraudTriangleModel::calculate_pressure_score(const std::vector<FinancialData>& financials) {
    auto indicators = detect_pressure_indicators(financials);
    return normalize_score(static_cast<double>(indicators.size()), 5.0);
}

bool FraudTriangleModel::check_complex_structure(const std::vector<FinancialData>& financials) {
    if (financials.empty()) return false;
    // Check for high intangible/goodwill ratio (often indicates complex acquisitions)
    const auto& bs = financials[0].balance_sheet;
    if (bs.total_assets > 0) {
        double intangible_ratio = (bs.goodwill + bs.intangible_assets) / bs.total_assets;
        return intangible_ratio > 0.3;
    }
    return false;
}

bool FraudTriangleModel::check_unusual_transactions(const std::vector<FinancialData>& financials) {
    if (financials.size() < 2) return false;
    // Look for unusual spikes in receivables or inventory
    for (size_t i = 1; i < financials.size(); ++i) {
        double ar_change = 0, inv_change = 0;
        if (financials[i-1].balance_sheet.accounts_receivable > 0) {
            ar_change = (financials[i].balance_sheet.accounts_receivable - 
                        financials[i-1].balance_sheet.accounts_receivable) /
                        financials[i-1].balance_sheet.accounts_receivable;
        }
        if (financials[i-1].balance_sheet.inventory > 0) {
            inv_change = (financials[i].balance_sheet.inventory - 
                         financials[i-1].balance_sheet.inventory) /
                         financials[i-1].balance_sheet.inventory;
        }
        if (ar_change > 0.5 || inv_change > 0.5) return true;
    }
    return false;
}

bool FraudTriangleModel::check_estimate_changes(const std::vector<FinancialData>& financials) {
    // Check for volatile depreciation patterns
    if (financials.size() < 2) return false;
    for (size_t i = 1; i < financials.size(); ++i) {
        double curr_depr_rate = 0, prior_depr_rate = 0;
        if (financials[i].balance_sheet.ppe > 0) {
            curr_depr_rate = financials[i].income_statement.depreciation / 
                            financials[i].balance_sheet.ppe;
        }
        if (financials[i-1].balance_sheet.ppe > 0) {
            prior_depr_rate = financials[i-1].income_statement.depreciation / 
                             financials[i-1].balance_sheet.ppe;
        }
        if (prior_depr_rate > 0 && std::abs(curr_depr_rate - prior_depr_rate) / prior_depr_rate > 0.3) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> FraudTriangleModel::detect_opportunity_indicators(const std::vector<FinancialData>& financials) {
    std::vector<std::string> indicators;
    
    if (check_complex_structure(financials)) {
        indicators.push_back("Complex organizational structure (high intangibles)");
    }
    if (check_unusual_transactions(financials)) {
        indicators.push_back("Unusual changes in receivables or inventory");
    }
    if (check_estimate_changes(financials)) {
        indicators.push_back("Significant changes in accounting estimates");
    }
    
    return indicators;
}

double FraudTriangleModel::calculate_opportunity_score(const std::vector<FinancialData>& financials) {
    auto indicators = detect_opportunity_indicators(financials);
    return normalize_score(static_cast<double>(indicators.size()), 3.0);
}

bool FraudTriangleModel::check_aggressive_accounting(const std::vector<FinancialData>& financials) {
    if (financials.empty()) return false;
    // Net income significantly higher than operating cash flow
    for (const auto& f : financials) {
        if (f.income_statement.net_income > 0 && 
            f.cash_flow.operating_cash_flow > 0 &&
            f.income_statement.net_income > f.cash_flow.operating_cash_flow * 1.5) {
            return true;
        }
    }
    return false;
}

bool FraudTriangleModel::check_boundary_cases(const std::vector<FinancialData>& financials) {
    // Check for earnings just above zero (suspicious)
    int boundary_count = 0;
    for (const auto& f : financials) {
        double margin = f.income_statement.net_margin();
        if (margin > 0 && margin < 0.01) {
            boundary_count++;
        }
    }
    return boundary_count >= 2;
}

std::vector<std::string> FraudTriangleModel::detect_rationalization_indicators(const std::vector<FinancialData>& financials) {
    std::vector<std::string> indicators;
    
    if (check_aggressive_accounting(financials)) {
        indicators.push_back("Aggressive accounting (income >> cash flow)");
    }
    if (check_boundary_cases(financials)) {
        indicators.push_back("Earnings consistently at boundary levels");
    }
    
    return indicators;
}

double FraudTriangleModel::calculate_rationalization_score(const std::vector<FinancialData>& financials) {
    auto indicators = detect_rationalization_indicators(financials);
    return normalize_score(static_cast<double>(indicators.size()), 2.0);
}

RiskLevel FraudTriangleModel::determine_risk_level(double overall_score) {
    if (overall_score >= HIGH_RISK_THRESHOLD) return RiskLevel::HIGH;
    if (overall_score >= MODERATE_RISK_THRESHOLD) return RiskLevel::MODERATE;
    if (overall_score >= LOW_RISK_THRESHOLD) return RiskLevel::ELEVATED;
    return RiskLevel::LOW;
}

FraudTriangleResult FraudTriangleModel::calculate(const std::vector<FinancialData>& financials) {
    FraudTriangleResult result;
    
    // Calculate component scores
    result.pressure_score = calculate_pressure_score(financials);
    result.opportunity_score = calculate_opportunity_score(financials);
    result.rationalization_score = calculate_rationalization_score(financials);
    
    // Calculate weighted overall risk
    result.overall_risk = PRESSURE_WEIGHT * result.pressure_score
                        + OPPORTUNITY_WEIGHT * result.opportunity_score
                        + RATIONALIZATION_WEIGHT * result.rationalization_score;
    
    // Determine risk level
    result.risk_level = determine_risk_level(result.overall_risk);
    
    // Collect indicators
    result.pressure_indicators = detect_pressure_indicators(financials);
    result.opportunity_indicators = detect_opportunity_indicators(financials);
    result.rationalization_indicators = detect_rationalization_indicators(financials);
    
    return result;
}

} // namespace sec_analyzer
