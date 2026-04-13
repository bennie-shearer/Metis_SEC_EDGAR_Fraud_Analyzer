/**
 * SEC EDGAR Fraud Analyzer - Main Analyzer
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Core analysis engine coordinating fraud detection models.
 */

#ifndef SEC_ANALYZER_ANALYZER_H
#define SEC_ANALYZER_ANALYZER_H

#include "types.h"
#include "sec_fetcher.h"
#include "models/beneish.h"
#include "models/altman.h"
#include "models/piotroski.h"
#include "models/fraud_triangle.h"
#include "models/benford.h"
#include <memory>

namespace sec_analyzer {

class FraudAnalyzer {
public:
    FraudAnalyzer();
    explicit FraudAnalyzer(const RiskWeights& weights);
    ~FraudAnalyzer();
    
    // Configuration
    void set_weights(const RiskWeights& weights) { weights_ = weights; }
    void set_fetcher(std::shared_ptr<SECFetcher> fetcher) { fetcher_ = fetcher; }
    
    // Main analysis functions
    AnalysisResult analyze_by_ticker(const std::string& ticker, int years = 5);
    AnalysisResult analyze_by_cik(const std::string& cik, int years = 5);
    AnalysisResult analyze_financials(const std::vector<FinancialData>& financials, const CompanyInfo& company);
    
    // Individual model analysis
    BeneishResult calculate_beneish(const FinancialData& current, const FinancialData& prior);
    AltmanResult calculate_altman(const FinancialData& data, double market_cap = 0);
    PiotroskiResult calculate_piotroski(const FinancialData& current, const FinancialData& prior);
    FraudTriangleResult calculate_fraud_triangle(const std::vector<FinancialData>& financials);
    BenfordResult calculate_benford(const std::vector<double>& values);
    
    // Red flag detection
    std::vector<RedFlag> detect_red_flags(const AnalysisResult& result);
    
    // Trend analysis
    TrendAnalysis analyze_trends(const std::vector<FinancialData>& financials);
    
    // Composite scoring
    double calculate_composite_score(const AnalysisResult& result);
    RiskLevel determine_risk_level(double score);
    std::string generate_recommendation(const AnalysisResult& result);
    
    // Error handling
    std::string get_last_error() const { return last_error_; }
    bool has_error() const { return !last_error_.empty(); }

private:
    RiskWeights weights_;
    std::shared_ptr<SECFetcher> fetcher_;
    std::string last_error_;
    
    // Model instances
    std::unique_ptr<BeneishModel> beneish_model_;
    std::unique_ptr<AltmanModel> altman_model_;
    std::unique_ptr<PiotroskiModel> piotroski_model_;
    std::unique_ptr<FraudTriangleModel> fraud_triangle_model_;
    std::unique_ptr<BenfordModel> benford_model_;
    
    // Helper functions
    void validate_financials(const std::vector<FinancialData>& financials);
    std::vector<double> extract_all_values(const std::vector<FinancialData>& financials);
    void populate_analysis_metadata(AnalysisResult& result);
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_ANALYZER_H
