/**
 * SEC EDGAR Fraud Analyzer - Fraud Triangle Model
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Implementation of the Fraud Triangle framework for fraud risk assessment.
 * Reference: Cressey, D.R. (1953) "Other People's Money: A Study in the
 *            Social Psychology of Embezzlement"
 */

#ifndef SEC_ANALYZER_MODELS_FRAUD_TRIANGLE_H
#define SEC_ANALYZER_MODELS_FRAUD_TRIANGLE_H

#include "../types.h"
#include <vector>

namespace sec_analyzer {

/**
 * Fraud Triangle Model
 * 
 * The Fraud Triangle identifies three conditions that are typically
 * present when fraud occurs:
 * 
 * 1. Pressure (Incentive/Motivation)
 *    - Financial targets/expectations
 *    - Debt covenants
 *    - Declining performance
 *    - Management compensation tied to results
 * 
 * 2. Opportunity
 *    - Weak internal controls
 *    - Complex transactions
 *    - Related party transactions
 *    - Management override capability
 * 
 * 3. Rationalization (Attitude)
 *    - Management's attitude toward financial reporting
 *    - History of violations
 *    - Aggressive accounting practices
 */
class FraudTriangleModel {
public:
    FraudTriangleModel() = default;
    ~FraudTriangleModel() = default;
    
    // Calculate fraud triangle risk
    FraudTriangleResult calculate(const std::vector<FinancialData>& financials);
    
    // Calculate individual components
    double calculate_pressure_score(const std::vector<FinancialData>& financials);
    double calculate_opportunity_score(const std::vector<FinancialData>& financials);
    double calculate_rationalization_score(const std::vector<FinancialData>& financials);
    
    // Pressure indicators
    std::vector<std::string> detect_pressure_indicators(const std::vector<FinancialData>& financials);
    bool check_declining_revenue(const std::vector<FinancialData>& financials);
    bool check_declining_margins(const std::vector<FinancialData>& financials);
    bool check_high_leverage(const FinancialData& data);
    bool check_negative_cash_flow(const FinancialData& data);
    bool check_earnings_miss_pattern(const std::vector<FinancialData>& financials);
    
    // Opportunity indicators
    std::vector<std::string> detect_opportunity_indicators(const std::vector<FinancialData>& financials);
    bool check_complex_structure(const std::vector<FinancialData>& financials);
    bool check_unusual_transactions(const std::vector<FinancialData>& financials);
    bool check_estimate_changes(const std::vector<FinancialData>& financials);
    
    // Rationalization indicators
    std::vector<std::string> detect_rationalization_indicators(const std::vector<FinancialData>& financials);
    bool check_aggressive_accounting(const std::vector<FinancialData>& financials);
    bool check_boundary_cases(const std::vector<FinancialData>& financials);
    
    // Overall risk
    static RiskLevel determine_risk_level(double overall_score);
    
    // Thresholds
    static constexpr double HIGH_RISK_THRESHOLD = 0.7;
    static constexpr double MODERATE_RISK_THRESHOLD = 0.4;
    static constexpr double LOW_RISK_THRESHOLD = 0.2;

private:
    // Scoring weights
    static constexpr double PRESSURE_WEIGHT = 0.35;
    static constexpr double OPPORTUNITY_WEIGHT = 0.35;
    static constexpr double RATIONALIZATION_WEIGHT = 0.30;
    
    double normalize_score(double raw_score, double max_indicators);
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_MODELS_FRAUD_TRIANGLE_H
