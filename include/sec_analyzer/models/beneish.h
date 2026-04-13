/**
 * SEC EDGAR Fraud Analyzer - Beneish M-Score Model
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Implementation of the Beneish M-Score model for detecting earnings manipulation.
 * Reference: Beneish, M.D. (1999) "The Detection of Earnings Manipulation"
 */

#ifndef SEC_ANALYZER_MODELS_BENEISH_H
#define SEC_ANALYZER_MODELS_BENEISH_H

#include "../types.h"

namespace sec_analyzer {

/**
 * Beneish M-Score Model
 * 
 * The M-Score is calculated using 8 financial ratios:
 * - DSRI: Days Sales in Receivables Index
 * - GMI: Gross Margin Index
 * - AQI: Asset Quality Index
 * - SGI: Sales Growth Index
 * - DEPI: Depreciation Index
 * - SGAI: SG&A Index
 * - LVGI: Leverage Index
 * - TATA: Total Accruals to Total Assets
 * 
 * M-Score = -4.84 + 0.920*DSRI + 0.528*GMI + 0.404*AQI + 0.892*SGI
 *           + 0.115*DEPI - 0.172*SGAI + 4.679*TATA - 0.327*LVGI
 * 
 * Interpretation:
 * - M-Score > -2.22: Likely manipulator
 * - M-Score < -2.22: Unlikely manipulator
 */
class BeneishModel {
public:
    BeneishModel() = default;
    ~BeneishModel() = default;
    
    // Calculate M-Score from two periods
    BeneishResult calculate(const FinancialData& current, const FinancialData& prior);
    
    // Calculate individual components
    double calculate_dsri(const FinancialData& current, const FinancialData& prior);
    double calculate_gmi(const FinancialData& current, const FinancialData& prior);
    double calculate_aqi(const FinancialData& current, const FinancialData& prior);
    double calculate_sgi(const FinancialData& current, const FinancialData& prior);
    double calculate_depi(const FinancialData& current, const FinancialData& prior);
    double calculate_sgai(const FinancialData& current, const FinancialData& prior);
    double calculate_lvgi(const FinancialData& current, const FinancialData& prior);
    double calculate_tata(const FinancialData& current);
    
    // Interpret results
    static bool is_likely_manipulator(double m_score) { return m_score > THRESHOLD; }
    static std::string get_zone(double m_score);
    static double score_to_probability(double m_score);
    static double probability_to_risk(double probability);
    
    // Threshold
    static constexpr double THRESHOLD = -2.22;
    
    // Model coefficients
    static constexpr double INTERCEPT = -4.84;
    static constexpr double COEF_DSRI = 0.920;
    static constexpr double COEF_GMI = 0.528;
    static constexpr double COEF_AQI = 0.404;
    static constexpr double COEF_SGI = 0.892;
    static constexpr double COEF_DEPI = 0.115;
    static constexpr double COEF_SGAI = -0.172;
    static constexpr double COEF_TATA = 4.679;
    static constexpr double COEF_LVGI = -0.327;

private:
    // Safe division helper
    static double safe_divide(double num, double denom, double default_val = 1.0);
    
    // Flag generation
    std::vector<std::string> generate_flags(const BeneishResult& result);
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_MODELS_BENEISH_H
