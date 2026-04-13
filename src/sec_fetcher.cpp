/**
 * SEC EDGAR Fraud Analyzer - SEC Fetcher Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/version.h>
#include <sec_analyzer/sec_fetcher.h>
#include <sec_analyzer/logger.h>
#include <sec_analyzer/util.h>
#include <sec_analyzer/json.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdio>

namespace sec_analyzer {

SECFetcher::SECFetcher() : user_agent_("Bob Doe bob@doe.com") {
    last_request_time_ = std::chrono::steady_clock::now() - std::chrono::seconds(1);
}

SECFetcher::SECFetcher(const std::string& user_agent) : user_agent_(user_agent) {
    last_request_time_ = std::chrono::steady_clock::now() - std::chrono::seconds(1);
}

SECFetcher::~SECFetcher() = default;

void SECFetcher::rate_limit() {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_request_time_).count();
    
    if (elapsed < rate_limit_ms_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(rate_limit_ms_ - elapsed));
    }
    
    last_request_time_ = std::chrono::steady_clock::now();
}

std::optional<CompanyInfo> SECFetcher::lookup_company_by_ticker(const std::string& ticker) {
    LOG_INFO("Looking up company by ticker: {}", ticker);
    
    // Normalize ticker: convert periods to hyphens (BRK.A -> BRK-A)
    std::string normalized_ticker = ticker;
    for (char& c : normalized_ticker) {
        if (c == '.') c = '-';
    }
    
    // Check cache first
    std::string cache_key = "company:ticker:" + normalized_ticker;
    if (cache_) {
        auto cached = cache_->get(cache_key);
        if (cached) {
            return parse_company_info(*cached);
        }
    }
    
    // Fetch from SEC
    auto json = fetch_json(sec_urls::COMPANY_TICKERS);
    if (!json) {
        LOG_ERROR("Failed to fetch company tickers: {}", last_error_);
        last_error_ = "Failed to fetch company tickers: " + last_error_;
        return std::nullopt;
    }
    
    LOG_DEBUG("Received {} bytes from SEC", json->size());
    
    // Parse and find ticker
    try {
        auto data = parse_json(*json);
        
        if (!data.is_object()) {
            LOG_ERROR("SEC response is not a JSON object");
            last_error_ = "Invalid SEC response format";
            return std::nullopt;
        }
        
        const auto& obj = data.as_object();
        LOG_DEBUG("Parsed JSON with {} entries", obj.size());
        
        std::string ticker_upper = util::to_upper(normalized_ticker);
        
        // SEC company_tickers.json is object with numeric keys
        for (const auto& item : obj) {
            const auto& value = item.second;
            if (value.is_object() && value.contains("ticker")) {
                std::string entry_ticker = value.at("ticker").as_string();
                if (util::to_upper(entry_ticker) == ticker_upper) {
                    CompanyInfo info;
                    info.ticker = entry_ticker;
                    info.name = value.contains("title") ? value.at("title").as_string() : "";
                    info.cik = normalize_cik(std::to_string(value.at("cik_str").as_int()));
                    LOG_INFO("Found company: {} (CIK: {})", info.name, info.cik);
                    return info;
                }
            }
        }
        
        LOG_WARNING("Ticker {} not found in {} entries", ticker, obj.size());
        
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse error: {}", e.what());
        last_error_ = std::string("Parse error: ") + e.what();
        return std::nullopt;
    }
    
    last_error_ = "Company not found: " + ticker;
    return std::nullopt;
}

std::optional<CompanyInfo> SECFetcher::lookup_company_by_cik(const std::string& cik) {
    LOG_INFO("Looking up company by CIK: {}", cik);
    
    std::string normalized = normalize_cik(cik);
    std::string url = sec_urls::SUBMISSIONS + "/CIK" + normalized + ".json";
    
    auto json = fetch_url(url);
    if (!json) {
        last_error_ = "Failed to fetch company info for CIK: " + cik;
        return std::nullopt;
    }
    
    return parse_company_info(*json);
}

std::vector<CompanyInfo> SECFetcher::search_companies(const std::string& query) {
    std::vector<CompanyInfo> results;
    
    auto json = fetch_json(sec_urls::COMPANY_TICKERS);
    if (!json) return results;
    
    std::string query_upper = util::to_upper(query);
    
    try {
        auto data = parse_json(*json);
        for (const auto& item : data.as_object()) {
            const auto& value = item.second;
            if (value.is_object()) {
                std::string name = value.contains("title") ? util::to_upper(value.at("title").as_string()) : "";
                std::string ticker = value.contains("ticker") ? util::to_upper(value.at("ticker").as_string()) : "";
                
                if (name.find(query_upper) != std::string::npos || ticker.find(query_upper) != std::string::npos) {
                    CompanyInfo info;
                    info.ticker = value.at("ticker").as_string();
                    info.name = value.contains("title") ? value.at("title").as_string() : "";
                    info.cik = normalize_cik(std::to_string(value.at("cik_str").as_int()));
                    results.push_back(info);
                    
                    if (results.size() >= 10) break;
                }
            }
        }
    } catch (...) {}
    
    return results;
}

std::vector<Filing> SECFetcher::get_filings(const std::string& cik, int years) {
    std::vector<Filing> filings;
    
    std::string normalized = normalize_cik(cik);
    std::string url = sec_urls::SUBMISSIONS + "/CIK" + normalized + ".json";
    
    auto json = fetch_url(url);
    if (!json) return filings;
    
    return parse_filings(*json, normalized);  // Pass CIK to populate filings
}

std::vector<Filing> SECFetcher::get_filings_by_type(const std::string& cik, FilingType type, int count) {
    std::vector<Filing> filtered;
    auto all_filings = get_filings(cik, 10);  // Get more years to find enough of specific type
    
    for (const auto& filing : all_filings) {
        if (filing.type == type) {
            filtered.push_back(filing);
            if (static_cast<int>(filtered.size()) >= count) break;
        }
    }
    
    LOG_DEBUG("Found {} filings of requested type for CIK {}", filtered.size(), cik);
    return filtered;
}

std::optional<std::string> SECFetcher::get_filing_document(const std::string& accession, const std::string& filename) {
    // Build URL to specific filing document
    std::string clean_accession = accession;
    // Remove dashes from accession number for URL
    clean_accession.erase(std::remove(clean_accession.begin(), clean_accession.end(), '-'), clean_accession.end());
    
    std::string url = sec_urls::BASE + "/Archives/edgar/data/" + clean_accession + "/" + filename;
    LOG_DEBUG("Fetching document: {}", url);
    
    return fetch_url(url);
}

std::string SECFetcher::ticker_to_cik(const std::string& ticker) {
    auto company = lookup_company_by_ticker(ticker);
    if (company) {
        return company->cik;
    }
    return "";
}

// Helper to extract value from company facts for a specific concept and period
static double extract_fact_value(const JsonValue& facts, const std::string& concept_name, 
                                  const std::string& accession, int fiscal_year, bool annual) {
    try {
        if (!facts.contains("us-gaap")) return 0.0;
        const auto& us_gaap = facts.at("us-gaap");
        
        if (!us_gaap.contains(concept_name)) return 0.0;
        const auto& concept_data = us_gaap.at(concept_name);
        
        if (!concept_data.contains("units")) return 0.0;
        const auto& units = concept_data.at("units");
        
        // Try USD first, then pure
        const char* unit_types[] = {"USD", "pure", "shares"};
        for (const char* unit_type : unit_types) {
            if (!units.contains(unit_type)) continue;
            const auto& values = units.at(unit_type);
            
            for (size_t i = 0; i < values.size(); ++i) {
                const auto& entry = values[i];
                if (!entry.contains("fy") || !entry.contains("val")) continue;
                
                int fy = entry.at("fy").as_int();
                std::string form = entry.contains("form") ? entry.at("form").as_string() : "";
                std::string fp = entry.contains("fp") ? entry.at("fp").as_string() : "";
                
                // Match by fiscal year and form type
                if (fy == fiscal_year) {
                    if ((annual && (form == "10-K" || fp == "FY")) ||
                        (!annual && (form == "10-Q" || fp == "Q1" || fp == "Q2" || fp == "Q3"))) {
                        return entry.at("val").as_number();
                    }
                }
            }
        }
    } catch (...) {}
    return 0.0;
}

std::optional<FinancialData> SECFetcher::get_financial_data(const Filing& filing) {
    LOG_DEBUG("Fetching financial data for filing: {}", filing.accession_number);
    
    FinancialData data;
    data.filing = filing;
    data.is_valid = false;
    
    // Use the CIK stored in the filing
    if (filing.cik.empty()) {
        LOG_WARNING("Filing has no CIK");
        return data;
    }
    
    // Fetch company facts
    std::string url = sec_urls::COMPANY_FACTS + "/CIK" + normalize_cik(filing.cik) + ".json";
    auto json = fetch_url(url);
    if (!json) {
        LOG_WARNING("Failed to fetch company facts for CIK {}", filing.cik);
        return data;
    }
    
    try {
        auto facts_data = parse_json(*json);
        if (!facts_data.contains("facts")) {
            LOG_WARNING("No facts in company data");
            return data;
        }
        
        const auto& facts = facts_data.at("facts");
        bool is_annual = filing.is_annual();
        int fy = filing.fiscal_year > 0 ? filing.fiscal_year : 2024;  // Default to recent
        
        // Extract income statement data
        data.income_statement.revenue = extract_fact_value(facts, "Revenues", filing.accession_number, fy, is_annual);
        if (data.income_statement.revenue == 0) {
            data.income_statement.revenue = extract_fact_value(facts, "RevenueFromContractWithCustomerExcludingAssessedTax", filing.accession_number, fy, is_annual);
        }
        if (data.income_statement.revenue == 0) {
            data.income_statement.revenue = extract_fact_value(facts, "SalesRevenueNet", filing.accession_number, fy, is_annual);
        }
        
        data.income_statement.net_income = extract_fact_value(facts, "NetIncomeLoss", filing.accession_number, fy, is_annual);
        data.income_statement.operating_income = extract_fact_value(facts, "OperatingIncomeLoss", filing.accession_number, fy, is_annual);
        data.income_statement.gross_profit = extract_fact_value(facts, "GrossProfit", filing.accession_number, fy, is_annual);
        data.income_statement.cost_of_revenue = extract_fact_value(facts, "CostOfGoodsAndServicesSold", filing.accession_number, fy, is_annual);
        if (data.income_statement.cost_of_revenue == 0) {
            data.income_statement.cost_of_revenue = extract_fact_value(facts, "CostOfRevenue", filing.accession_number, fy, is_annual);
        }
        
        // Extract balance sheet data
        data.balance_sheet.total_assets = extract_fact_value(facts, "Assets", filing.accession_number, fy, is_annual);
        data.balance_sheet.total_liabilities = extract_fact_value(facts, "Liabilities", filing.accession_number, fy, is_annual);
        data.balance_sheet.total_equity = extract_fact_value(facts, "StockholdersEquity", filing.accession_number, fy, is_annual);
        data.balance_sheet.current_assets = extract_fact_value(facts, "AssetsCurrent", filing.accession_number, fy, is_annual);
        data.balance_sheet.current_liabilities = extract_fact_value(facts, "LiabilitiesCurrent", filing.accession_number, fy, is_annual);
        data.balance_sheet.cash = extract_fact_value(facts, "CashAndCashEquivalentsAtCarryingValue", filing.accession_number, fy, is_annual);
        data.balance_sheet.accounts_receivable = extract_fact_value(facts, "AccountsReceivableNetCurrent", filing.accession_number, fy, is_annual);
        data.balance_sheet.inventory = extract_fact_value(facts, "InventoryNet", filing.accession_number, fy, is_annual);
        data.balance_sheet.long_term_debt = extract_fact_value(facts, "LongTermDebt", filing.accession_number, fy, is_annual);
        
        // Extract cash flow data
        data.cash_flow.operating_cash_flow = extract_fact_value(facts, "NetCashProvidedByUsedInOperatingActivities", filing.accession_number, fy, is_annual);
        data.cash_flow.investing_cash_flow = extract_fact_value(facts, "NetCashProvidedByUsedInInvestingActivities", filing.accession_number, fy, is_annual);
        data.cash_flow.financing_cash_flow = extract_fact_value(facts, "NetCashProvidedByUsedInFinancingActivities", filing.accession_number, fy, is_annual);
        data.cash_flow.capital_expenditures = extract_fact_value(facts, "PaymentsToAcquirePropertyPlantAndEquipment", filing.accession_number, fy, is_annual);
        
        data.is_valid = (data.income_statement.revenue > 0 || data.balance_sheet.total_assets > 0);
        
        LOG_INFO("Extracted financial data - Revenue: ${:.0f}M, Net Income: ${:.0f}M", 
                 data.income_statement.revenue / 1e6, data.income_statement.net_income / 1e6);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse company facts: {}", e.what());
        data.error_message = e.what();
    }
    
    return data;
}

std::vector<FinancialData> SECFetcher::get_all_financial_data(const std::string& cik, int years) {
    std::vector<FinancialData> all_data;
    
    LOG_DEBUG("Fetching all financial data for CIK: {}, years: {}", cik, years);
    
    // Get filings for the company
    auto filings = get_filings(cik, years);
    
    // Get financial data for each filing
    for (const auto& filing : filings) {
        auto data = get_financial_data(filing);
        if (data) {
            all_data.push_back(*data);
        }
    }
    
    LOG_INFO("Retrieved {} financial data records for CIK {}", all_data.size(), cik);
    return all_data;
}

std::optional<std::string> SECFetcher::fetch_url(const std::string& url) {
    rate_limit();
    return http_get(url);
}

std::optional<std::string> SECFetcher::fetch_json(const std::string& endpoint) {
    return fetch_url(endpoint);
}

#ifdef _WIN32

// URL parsing helper
struct ParsedUrl {
    std::wstring host;
    std::wstring path;
    int port = 443;
    bool https = true;
};

static ParsedUrl parse_url(const std::string& url) {
    ParsedUrl parsed;
    std::string work = url;
    
    // Check protocol
    if (work.substr(0, 8) == "https://") {
        parsed.https = true;
        parsed.port = 443;
        work = work.substr(8);
    } else if (work.substr(0, 7) == "http://") {
        parsed.https = false;
        parsed.port = 80;
        work = work.substr(7);
    }
    
    // Find path
    size_t slash_pos = work.find('/');
    std::string host_part;
    std::string path_part = "/";
    
    if (slash_pos != std::string::npos) {
        host_part = work.substr(0, slash_pos);
        path_part = work.substr(slash_pos);
    } else {
        host_part = work;
    }
    
    // Check for port
    size_t colon_pos = host_part.find(':');
    if (colon_pos != std::string::npos) {
        parsed.port = std::stoi(host_part.substr(colon_pos + 1));
        host_part = host_part.substr(0, colon_pos);
    }
    
    // Convert to wide strings
    parsed.host = std::wstring(host_part.begin(), host_part.end());
    parsed.path = std::wstring(path_part.begin(), path_part.end());
    
    return parsed;
}

std::optional<std::string> SECFetcher::http_get(const std::string& url) {
    LOG_DEBUG("HTTP GET: {}", url);
    
    auto parsed = parse_url(url);
    
    // Initialize WinHTTP session — pass NULL User-Agent here; we set it
    // explicitly per-request via WinHttpAddRequestHeaders so it is always
    // correct regardless of how the SECFetcher was constructed.
    HINTERNET hSession = WinHttpOpen(
        NULL,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) {
        last_error_ = "WinHttpOpen failed: " + std::to_string(GetLastError());
        return std::nullopt;
    }
    
    // Connect
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        parsed.host.c_str(),
        static_cast<INTERNET_PORT>(parsed.port),
        0
    );
    
    if (!hConnect) {
        last_error_ = "WinHttpConnect failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }
    
    // Create request
    DWORD flags = parsed.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        parsed.path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );
    
    if (!hRequest) {
        last_error_ = "WinHttpOpenRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }
    
    // Add headers — use REPLACE so the request-level User-Agent definitively
    // overrides the session-level one set in WinHttpOpen. SEC EDGAR requires
    // a valid User-Agent with a real email in the HTTP request header.
    std::wstring ua_wide(user_agent_.begin(), user_agent_.end());
    std::wstring headers =
        L"User-Agent: " + ua_wide + L"\r\n"
        L"Accept: application/json, text/plain, */*\r\n"
        L"Accept-Encoding: identity\r\n";
    WinHttpAddRequestHeaders(
        hRequest,
        headers.c_str(),
        static_cast<DWORD>(-1),
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
    );
    
    // Send request
    BOOL result = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );
    
    if (!result) {
        last_error_ = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }
    
    // Receive response
    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result) {
        last_error_ = "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }
    
    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX
    );
    
    if (statusCode != 200) {
        std::string error_msg = "HTTP error " + std::to_string(statusCode);
        if (statusCode == 403) {
            error_msg += " - SEC requires valid User-Agent with email";
        } else if (statusCode == 404) {
            error_msg += " - Resource not found";
        } else if (statusCode == 429) {
            error_msg += " - Rate limited, please wait";
        }
        last_error_ = error_msg;
        LOG_ERROR("{}", error_msg);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }
    
    // Read response
    std::string response;
    DWORD bytesAvailable = 0;
    
    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }
        
        if (bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable + 1);
            DWORD bytesRead = 0;
            
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                response.append(buffer.data(), bytesRead);
            }
        }
    } while (bytesAvailable > 0);
    
    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    LOG_DEBUG("HTTP response: {} bytes", response.size());
    return response;
}

#else
// Linux/macOS implementation using system curl command
// This avoids requiring OpenSSL or libcurl as linked dependencies
std::optional<std::string> SECFetcher::http_get(const std::string& url) {
    LOG_DEBUG("HTTP GET: {}", url);
    
    // Build curl command
    // -s: silent, -S: show errors, -f: fail on HTTP errors
    // -L: follow redirects, -A: user agent
    std::string cmd = "curl -sSfL -A \"" + user_agent_ + "\" \"" + url + "\" 2>&1";
    
    // Execute curl and capture output
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute curl command";
        return std::nullopt;
    }
    
    std::string response;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) {
        // curl returns non-zero on HTTP errors (due to -f flag)
        last_error_ = "HTTP request failed: " + response;
        return std::nullopt;
    }
    
    LOG_DEBUG("HTTP response: {} bytes", response.size());
    return response;
}
#endif

std::string SECFetcher::normalize_cik(const std::string& cik) {
    return util::normalize_cik(cik);
}

CompanyInfo SECFetcher::parse_company_info(const std::string& json) {
    CompanyInfo info;
    try {
        auto data = parse_json(json);
        if (data.contains("name")) info.name = data.at("name").as_string();
        if (data.contains("tickers") && data.at("tickers").size() > 0) {
            info.ticker = data.at("tickers")[0].as_string();
        }
        if (data.contains("cik")) info.cik = normalize_cik(data.at("cik").as_string());
        if (data.contains("sic")) info.sic = data.at("sic").as_string();
    } catch (...) {}
    return info;
}

std::vector<Filing> SECFetcher::parse_filings(const std::string& json, const std::string& cik) {
    std::vector<Filing> filings;
    try {
        auto data = parse_json(json);
        if (data.contains("filings") && data.at("filings").contains("recent")) {
            const auto& recent = data.at("filings").at("recent");
            
            // SEC EDGAR returns arrays for each field
            if (recent.contains("form") && recent.contains("filingDate") && 
                recent.contains("accessionNumber")) {
                
                size_t count = recent.at("form").size();
                for (size_t i = 0; i < count && i < 100; ++i) {  // Limit to 100 filings
                    Filing filing;
                    filing.cik = cik;  // Store the company CIK
                    filing.form_type = recent.at("form")[i].as_string();
                    filing.filed_date = recent.at("filingDate")[i].as_string();
                    filing.accession_number = recent.at("accessionNumber")[i].as_string();
                    
                    // Extract fiscal year from report date or filing date
                    if (recent.contains("reportDate") && i < recent.at("reportDate").size()) {
                        std::string report_date = recent.at("reportDate")[i].as_string();
                        filing.report_date = report_date;
                        if (report_date.size() >= 4) {
                            filing.fiscal_year = std::stoi(report_date.substr(0, 4));
                        }
                    } else if (filing.filed_date.size() >= 4) {
                        filing.fiscal_year = std::stoi(filing.filed_date.substr(0, 4));
                    }
                    
                    // Determine filing type
                    if (filing.form_type == "10-K") filing.type = FilingType::K10;
                    else if (filing.form_type == "10-K/A") filing.type = FilingType::K10A;
                    else if (filing.form_type == "10-Q") filing.type = FilingType::Q10;
                    else if (filing.form_type == "10-Q/A") filing.type = FilingType::Q10A;
                    else if (filing.form_type == "8-K") filing.type = FilingType::K8;
                    
                    // Only include 10-K and 10-Q filings for analysis
                    if (filing.type != FilingType::UNKNOWN) {
                        filings.push_back(filing);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_WARNING("Failed to parse filings: {}", e.what());
    }
    return filings;
}

} // namespace sec_analyzer
