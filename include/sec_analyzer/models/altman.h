/**
 * SEC EDGAR Fraud Analyzer - Altman Z-Score Model
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Implementation of the Altman Z-Score model for bankruptcy prediction.
 * Reference: Altman, E.I. (1968) "Financial Ratios, Discriminant Analysis
 *            and the Prediction of Corporate Bankruptcy"
 */

#ifndef SEC_ANALYZER_MODELS_ALTMAN_H
#define SEC_ANALYZER_MODELS_ALTMAN_H

#include "../types.h"

namespace sec_analyzer {

/**
 * Altman Z-Score Model
 * 
 * For public manufacturing companies:
 * Z = 1.2*X1 + 1.4*X2 + 3.3*X3 + 0.6*X4 + 1.0*X5
 * 
 * Where:
 * - X1 = Working Capital / Total Assets
 * - X2 = Retained Earnings / Total Assets
 * - X3 = EBIT / Total Assets
 * - X4 = Market Value of Equity / Total Liabilities
 * - X5 = Sales / Total Assets
 * 
 * Zones:
 * - Z > 2.99: Safe Zone (low probability of bankruptcy)
 * - 1.81 < Z < 2.99: Gray Zone (uncertain)
 * - Z < 1.81: Distress Zone (high probability of bankruptcy)
 */
class AltmanModel {
public:
    AltmanModel() = default;
    ~AltmanModel() = default;
    
    // Calculate Z-Score
    AltmanResult calculate(const FinancialData& data, double market_cap = 0);
    
    // Calculate individual components
    double calculate_x1(const FinancialData& data);  // Working Capital / TA
    double calculate_x2(const FinancialData& data);  // Retained Earnings / TA
    double calculate_x3(const FinancialData& data);  // EBIT / TA
    double calculate_x4(const FinancialData& data, double market_cap);  // MV Equity / TL
    double calculate_x5(const FinancialData& data);  // Sales / TA
    
    // Interpret results
    static std::string get_zone(double z_score);
    static double score_to_probability(double z_score);
    static double probability_to_risk(double probability);
    
    // Zone thresholds
    static constexpr double SAFE_THRESHOLD = 2.99;
    static constexpr double DISTRESS_THRESHOLD = 1.81;
    
    // Model coefficients (public manufacturing)
    static constexpr double COEF_X1 = 1.2;
    static constexpr double COEF_X2 = 1.4;
    static constexpr double COEF_X3 = 3.3;
    static constexpr double COEF_X4 = 0.6;
    static constexpr double COEF_X5 = 1.0;

private:
    static double safe_divide(double num, double denom, double default_val = 0.0);
};

/**
 * Altman Z''-Score for non-manufacturing and emerging markets
 * Z'' = 6.56*X1 + 3.26*X2 + 6.72*X3 + 1.05*X4
 */
class AltmanZPrimeModel {
public:
    AltmanResult calculate(const FinancialData& data);
    
    static constexpr double SAFE_THRESHOLD = 2.60;
    static constexpr double DISTRESS_THRESHOLD = 1.10;
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_MODELS_ALTMAN_H
