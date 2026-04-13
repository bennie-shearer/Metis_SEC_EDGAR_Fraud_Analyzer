/**
 * SEC EDGAR Fraud Analyzer - SEC EDGAR Fetcher
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * SEC EDGAR API client for fetching company filings.
 */

#ifndef SEC_ANALYZER_SEC_FETCHER_H
#define SEC_ANALYZER_SEC_FETCHER_H

#include "types.h"
#include "cache.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace sec_analyzer {

class SECFetcher {
public:
    SECFetcher();
    explicit SECFetcher(const std::string& user_agent);
    ~SECFetcher();
    
    // Configuration
    void set_user_agent(const std::string& ua) { user_agent_ = ua; }
    void set_rate_limit_ms(int ms) { rate_limit_ms_ = ms; }
    void set_cache(Cache<std::string>* cache) { cache_ = cache; }
    void set_timeout(int seconds) { timeout_seconds_ = seconds; }
    
    // Company lookup
    std::optional<CompanyInfo> lookup_company_by_ticker(const std::string& ticker);
    std::optional<CompanyInfo> lookup_company_by_cik(const std::string& cik);
    std::vector<CompanyInfo> search_companies(const std::string& query);
    
    // Filing retrieval
    std::vector<Filing> get_filings(const std::string& cik, int years = 5);
    std::vector<Filing> get_filings_by_type(const std::string& cik, FilingType type, int count = 10);
    std::optional<std::string> get_filing_document(const std::string& accession, const std::string& filename);
    
    // Financial data extraction
    std::optional<FinancialData> get_financial_data(const Filing& filing);
    std::vector<FinancialData> get_all_financial_data(const std::string& cik, int years = 5);
    
    // Raw data access
    std::optional<std::string> fetch_url(const std::string& url);
    std::optional<std::string> fetch_json(const std::string& endpoint);
    
    // CIK utilities
    static std::string normalize_cik(const std::string& cik);
    std::string ticker_to_cik(const std::string& ticker);
    
    // Error handling
    std::string get_last_error() const { return last_error_; }
    bool has_error() const { return !last_error_.empty(); }
    void clear_error() { last_error_.clear(); }

private:
    std::string user_agent_;
    int rate_limit_ms_ = 100;
    int timeout_seconds_ = 30;
    Cache<std::string>* cache_ = nullptr;
    std::string last_error_;
    
    std::chrono::steady_clock::time_point last_request_time_;
    std::mutex rate_limit_mutex_;
    
    // HTTP implementation
    std::optional<std::string> http_get(const std::string& url);
    void rate_limit();
    
    // Parsing helpers
    CompanyInfo parse_company_info(const std::string& json);
    std::vector<Filing> parse_filings(const std::string& json, const std::string& cik);
    FinancialData parse_financial_data(const std::string& content, const Filing& filing);
    
    // XBRL parsing
    double extract_xbrl_value(const std::string& content, const std::string& xbrl_concept);
    std::vector<std::string> get_xbrl_concepts();
};

// SEC EDGAR base URLs
namespace sec_urls {
    const std::string BASE = "https://data.sec.gov";
    const std::string SUBMISSIONS = "https://data.sec.gov/submissions";
    const std::string COMPANY_TICKERS = "https://www.sec.gov/files/company_tickers.json";
    const std::string COMPANY_FACTS = "https://data.sec.gov/api/xbrl/companyfacts";
}

} // namespace sec_analyzer

#endif // SEC_ANALYZER_SEC_FETCHER_H
