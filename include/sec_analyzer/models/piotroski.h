/**
 * SEC EDGAR Fraud Analyzer - Piotroski F-Score Model
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Implementation of the Piotroski F-Score for financial strength assessment.
 * Reference: Piotroski, J.D. (2000) "Value Investing: The Use of Historical
 *            Financial Statement Information to Separate Winners from Losers"
 */

#ifndef SEC_ANALYZER_MODELS_PIOTROSKI_H
#define SEC_ANALYZER_MODELS_PIOTROSKI_H

#include "../types.h"

namespace sec_analyzer {

/**
 * Piotroski F-Score Model
 * 
 * 9-point scoring system based on:
 * 
 * Profitability (4 points):
 * 1. ROA > 0 (positive net income)
 * 2. Operating Cash Flow > 0
 * 3. ROA increasing year-over-year
 * 4. Cash Flow from Operations > Net Income (quality of earnings)
 * 
 * Leverage/Liquidity (3 points):
 * 5. Decrease in leverage (long-term debt / avg total assets)
 * 6. Increase in current ratio
 * 7. No new share issuance (no dilution)
 * 
 * Operating Efficiency (2 points):
 * 8. Increase in gross margin
 * 9. Increase in asset turnover (sales / avg total assets)
 * 
 * Interpretation:
 * - 0-3: Weak (potential short candidate)
 * - 4-6: Moderate
 * - 7-9: Strong (potential long candidate)
 */
class PiotroskiModel {
public:
    PiotroskiModel() = default;
    ~PiotroskiModel() = default;
    
    // Calculate F-Score
    PiotroskiResult calculate(const FinancialData& current, const FinancialData& prior);
    
    // Individual criteria (each returns 0 or 1)
    int check_roa_positive(const FinancialData& current);
    int check_cfo_positive(const FinancialData& current);
    int check_roa_increasing(const FinancialData& current, const FinancialData& prior);
    int check_quality_of_earnings(const FinancialData& current);
    int check_leverage_decreasing(const FinancialData& current, const FinancialData& prior);
    int check_liquidity_increasing(const FinancialData& current, const FinancialData& prior);
    int check_no_dilution(const FinancialData& current, const FinancialData& prior);
    int check_gross_margin_increasing(const FinancialData& current, const FinancialData& prior);
    int check_asset_turnover_increasing(const FinancialData& current, const FinancialData& prior);
    
    // Interpret results
    static std::string get_interpretation(int f_score);
    static double score_to_risk(int f_score);
    
    // Thresholds
    static constexpr int WEAK_THRESHOLD = 3;
    static constexpr int STRONG_THRESHOLD = 7;

private:
    static double calculate_roa(const FinancialData& data);
    static double calculate_leverage(const FinancialData& data);
    static double calculate_current_ratio(const FinancialData& data);
    static double calculate_gross_margin(const FinancialData& data);
    static double calculate_asset_turnover(const FinancialData& data);
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_MODELS_PIOTROSKI_H
