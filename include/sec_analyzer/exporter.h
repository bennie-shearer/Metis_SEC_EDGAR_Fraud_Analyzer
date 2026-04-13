/**
 * SEC EDGAR Fraud Analyzer - Result Exporter
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Export analysis results to various formats.
 */

#ifndef SEC_ANALYZER_EXPORTER_H
#define SEC_ANALYZER_EXPORTER_H

#include "types.h"
#include <string>
#include <sstream>

namespace sec_analyzer {

class ResultExporter {
public:
    // Export to JSON
    static std::string to_json(const AnalysisResult& result, bool pretty = true);
    
    // Export to CSV
    static std::string to_csv(const AnalysisResult& result);
    static std::string filings_to_csv(const std::vector<FinancialData>& filings);
    
    // Export to HTML report
    static std::string to_html(const AnalysisResult& result);
    
    // Export specific sections
    static std::string models_to_json(const AnalysisResult& result);
    static std::string red_flags_to_json(const std::vector<RedFlag>& flags);
    static std::string trends_to_json(const TrendAnalysis& trends);
    
    // Health check response
    static std::string health_json(const std::string& version, int cache_entries = 0);
    
    // Error response
    static std::string error_json(const std::string& message, int code = 500);

private:
    // JSON helpers
    static std::string escape_json(const std::string& s);
    static std::string number_to_json(double value);
    static std::string risk_level_json(RiskLevel level);
    
    // HTML helpers
    static std::string escape_html(const std::string& s);
    static std::string risk_level_class(RiskLevel level);
    static std::string format_currency(double value);
    static std::string format_percentage(double value);
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_EXPORTER_H
