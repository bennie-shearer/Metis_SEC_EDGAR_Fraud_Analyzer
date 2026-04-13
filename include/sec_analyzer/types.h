/**
 * SEC EDGAR Fraud Analyzer - Type Definitions
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Core data structures for financial analysis.
 */

#ifndef SEC_ANALYZER_TYPES_H
#define SEC_ANALYZER_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include "version.h"

namespace sec_analyzer {

// Enumerations
enum class FilingType {
    UNKNOWN,
    K10,        // 10-K Annual Report
    K10A,       // 10-K/A Amendment
    Q10,        // 10-Q Quarterly Report
    Q10A,       // 10-Q/A Amendment
    K8,         // 8-K Current Report
    F20         // 20-F Annual Report (Foreign)
};

enum class RiskLevel {
    LOW,
    MODERATE,
    ELEVATED,
    HIGH,
    CRITICAL
};

enum class TrendDirection {
    IMPROVING,
    STABLE,
    DECLINING
};

enum class ExportFormat {
    JSON,
    CSV,
    HTML
};

// Model weighting configuration (plain data; AppConfig lives in config.h)
struct RiskWeights {
    double beneish        = 0.30;
    double altman         = 0.25;
    double piotroski      = 0.15;
    double fraud_triangle = 0.15;
    double benford        = 0.05;
    double red_flags      = 0.10;

    void normalize() {
        double total = beneish + altman + piotroski + fraud_triangle + benford + red_flags;
        if (total > 0) {
            beneish        /= total;
            altman         /= total;
            piotroski      /= total;
            fraud_triangle /= total;
            benford        /= total;
            red_flags      /= total;
        }
    }
};

// Filing Information
struct Filing {
    std::string cik;                // Company CIK (needed for XBRL lookup)
    std::string accession_number;
    std::string accession_clean;
    std::string form_type;
    std::string filed_date;
    std::string report_date;
    FilingType type = FilingType::UNKNOWN;
    int fiscal_year = 0;
    int fiscal_quarter = 0;
    
    bool is_annual() const { return type == FilingType::K10 || type == FilingType::K10A; }
    bool is_quarterly() const { return type == FilingType::Q10 || type == FilingType::Q10A; }
};

// Financial Statement Data
struct BalanceSheet {
    double total_assets = 0;
    double current_assets = 0;
    double cash = 0;
    double accounts_receivable = 0;
    double inventory = 0;
    double ppe = 0;              // Property, Plant, Equipment
    double goodwill = 0;
    double intangible_assets = 0;
    double total_liabilities = 0;
    double current_liabilities = 0;
    double accounts_payable = 0;
    double long_term_debt = 0;
    double total_equity = 0;
    double retained_earnings = 0;
    double shares_outstanding = 0;
    
    double working_capital() const { return current_assets - current_liabilities; }
    double current_ratio() const { return current_liabilities > 0 ? current_assets / current_liabilities : 0; }
    double quick_ratio() const { return current_liabilities > 0 ? (current_assets - inventory) / current_liabilities : 0; }
    double debt_ratio() const { return total_assets > 0 ? total_liabilities / total_assets : 0; }
    double debt_to_equity() const { return total_equity > 0 ? total_liabilities / total_equity : 0; }
};

struct IncomeStatement {
    double revenue = 0;
    double cost_of_revenue = 0;
    double gross_profit = 0;
    double operating_expenses = 0;
    double rd_expense = 0;
    double sga_expense = 0;
    double depreciation = 0;
    double operating_income = 0;
    double interest_expense = 0;
    double net_income = 0;
    double eps = 0;
    
    double gross_margin() const { return revenue > 0 ? gross_profit / revenue : 0; }
    double operating_margin() const { return revenue > 0 ? operating_income / revenue : 0; }
    double net_margin() const { return revenue > 0 ? net_income / revenue : 0; }
};

struct CashFlowStatement {
    double operating_cash_flow = 0;
    double depreciation_amortization = 0;
    double accounts_receivable_change = 0;
    double inventory_change = 0;
    double accounts_payable_change = 0;
    double investing_cash_flow = 0;
    double capital_expenditures = 0;
    double financing_cash_flow = 0;
    double dividends_paid = 0;
    double stock_buybacks = 0;
    double net_change_in_cash = 0;
    
    double free_cash_flow() const { return operating_cash_flow - capital_expenditures; }
};

struct FinancialData {
    Filing filing;
    BalanceSheet balance_sheet;
    IncomeStatement income_statement;
    CashFlowStatement cash_flow;
    bool is_valid = false;
    std::string error_message;
};

// Model Results
struct BeneishResult {
    double m_score = 0;
    double dsri = 0;    // Days Sales in Receivables Index
    double gmi = 0;     // Gross Margin Index
    double aqi = 0;     // Asset Quality Index
    double sgi = 0;     // Sales Growth Index
    double depi = 0;    // Depreciation Index
    double sgai = 0;    // SG&A Index
    double lvgi = 0;    // Leverage Index
    double tata = 0;    // Total Accruals to Total Assets
    double risk_score = 0;
    bool likely_manipulator = false;
    std::string zone;
    std::vector<std::string> flags;
};

struct AltmanResult {
    double z_score = 0;
    double x1 = 0;      // Working Capital / Total Assets
    double x2 = 0;      // Retained Earnings / Total Assets
    double x3 = 0;      // EBIT / Total Assets
    double x4 = 0;      // Market Value Equity / Total Liabilities
    double x5 = 0;      // Sales / Total Assets
    double bankruptcy_probability = 0;
    double risk_score = 0;
    std::string zone;
};

struct PiotroskiResult {
    int f_score = 0;
    bool roa_positive = false;
    bool cfo_positive = false;
    bool roa_increasing = false;
    bool cfo_greater_than_ni = false;
    bool leverage_decreasing = false;
    bool current_ratio_increasing = false;
    bool no_dilution = false;
    bool gross_margin_increasing = false;
    bool asset_turnover_increasing = false;
    double risk_score = 0;
    std::string interpretation;
};

struct FraudTriangleResult {
    double pressure_score = 0;
    double opportunity_score = 0;
    double rationalization_score = 0;
    double overall_risk = 0;
    RiskLevel risk_level = RiskLevel::LOW;
    std::vector<std::string> pressure_indicators;
    std::vector<std::string> opportunity_indicators;
    std::vector<std::string> rationalization_indicators;
};

struct BenfordResult {
    std::vector<double> expected_distribution;
    std::vector<double> actual_distribution;
    double chi_square = 0;
    double mad = 0;     // Mean Absolute Deviation
    double deviation_percent = 0;
    bool is_suspicious = false;
    std::vector<std::string> anomalies;
};

// Red Flag Structure
struct RedFlag {
    std::string type;
    std::string title;
    std::string description;
    RiskLevel severity = RiskLevel::MODERATE;
    std::string source;
    double confidence = 0;
};

// Trend Analysis
struct TrendAnalysis {
    TrendDirection revenue_trend = TrendDirection::STABLE;
    TrendDirection income_trend = TrendDirection::STABLE;
    TrendDirection cash_flow_trend = TrendDirection::STABLE;
    TrendDirection debt_trend = TrendDirection::STABLE;
    TrendDirection margin_trend = TrendDirection::STABLE;
    std::vector<std::string> observations;
};

// Company Information
struct CompanyInfo {
    std::string name;
    std::string ticker;
    std::string cik;
    std::string sic;
    std::string industry;
    std::string exchange;
    std::string fiscal_year_end;
};

// Overall Analysis Result
struct AnalysisResult {
    CompanyInfo company;
    int filings_analyzed = 0;
    
    // Model results
    std::optional<BeneishResult> beneish;
    std::optional<AltmanResult> altman;
    std::optional<PiotroskiResult> piotroski;
    std::optional<FraudTriangleResult> fraud_triangle;
    std::optional<BenfordResult> benford;
    
    // Filing data
    std::vector<FinancialData> filings;
    
    // Composite analysis
    double composite_risk_score = 0;
    RiskLevel overall_risk_level = RiskLevel::LOW;
    std::string risk_summary;
    std::string recommendation;
    
    // Red flags and trends
    std::vector<RedFlag> red_flags;
    TrendAnalysis trends;
    
    // Metadata
    std::string analysis_timestamp;
    std::string version = SEC_ANALYZER_VERSION_STRING;
};

// Utility functions
inline std::string risk_level_to_string(RiskLevel level) {
    switch (level) {
        case RiskLevel::LOW: return "LOW";
        case RiskLevel::MODERATE: return "MODERATE";
        case RiskLevel::ELEVATED: return "ELEVATED";
        case RiskLevel::HIGH: return "HIGH";
        case RiskLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

inline std::string trend_to_string(TrendDirection trend) {
    switch (trend) {
        case TrendDirection::IMPROVING: return "IMPROVING";
        case TrendDirection::STABLE: return "STABLE";
        case TrendDirection::DECLINING: return "DECLINING";
        default: return "UNKNOWN";
    }
}

inline std::string filing_type_to_string(FilingType type) {
    switch (type) {
        case FilingType::K10: return "10-K";
        case FilingType::K10A: return "10-K/A";
        case FilingType::Q10: return "10-Q";
        case FilingType::Q10A: return "10-Q/A";
        case FilingType::K8: return "8-K";
        case FilingType::F20: return "20-F";
        default: return "UNKNOWN";
    }
}

} // namespace sec_analyzer

#endif // SEC_ANALYZER_TYPES_H
