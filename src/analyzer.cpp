/**
 * SEC EDGAR Fraud Analyzer - Main Analyzer Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/analyzer.h>
#include <sec_analyzer/logger.h>
#include <sec_analyzer/util.h>

namespace sec_analyzer {

FraudAnalyzer::FraudAnalyzer() {
    beneish_model_ = std::make_unique<BeneishModel>();
    altman_model_ = std::make_unique<AltmanModel>();
    piotroski_model_ = std::make_unique<PiotroskiModel>();
    fraud_triangle_model_ = std::make_unique<FraudTriangleModel>();
    benford_model_ = std::make_unique<BenfordModel>();
}

FraudAnalyzer::FraudAnalyzer(const RiskWeights& weights) : FraudAnalyzer() {
    weights_ = weights;
}

FraudAnalyzer::~FraudAnalyzer() = default;

AnalysisResult FraudAnalyzer::analyze_by_ticker(const std::string& ticker, int years) {
    LOG_INFO("Analyzing ticker: {} for {} years", ticker, years);
    
    AnalysisResult result;
    result.analysis_timestamp = util::get_timestamp();
    
    if (!fetcher_) {
        last_error_ = "No SEC fetcher configured";
        return result;
    }
    
    auto company = fetcher_->lookup_company_by_ticker(ticker);
    if (!company) {
        last_error_ = fetcher_->get_last_error();
        return result;
    }
    
    result.company = *company;
    
    auto financials = fetcher_->get_all_financial_data(company->cik, years);
    return analyze_financials(financials, *company);
}

AnalysisResult FraudAnalyzer::analyze_by_cik(const std::string& cik, int years) {
    LOG_INFO("Analyzing CIK: {} for {} years", cik, years);
    
    AnalysisResult result;
    result.analysis_timestamp = util::get_timestamp();
    
    if (!fetcher_) {
        last_error_ = "No SEC fetcher configured";
        return result;
    }
    
    auto company = fetcher_->lookup_company_by_cik(cik);
    if (!company) {
        last_error_ = fetcher_->get_last_error();
        return result;
    }
    
    result.company = *company;
    
    auto financials = fetcher_->get_all_financial_data(cik, years);
    return analyze_financials(financials, *company);
}

AnalysisResult FraudAnalyzer::analyze_financials(const std::vector<FinancialData>& financials, const CompanyInfo& company) {
    AnalysisResult result;
    result.company = company;
    result.filings = financials;
    result.filings_analyzed = static_cast<int>(financials.size());
    result.analysis_timestamp = util::get_timestamp();
    
    if (financials.size() < 2) {
        last_error_ = "Insufficient financial data for analysis";
        return result;
    }
    
    // Calculate models
    result.beneish = beneish_model_->calculate(financials[0], financials[1]);
    result.altman = altman_model_->calculate(financials[0]);
    result.piotroski = piotroski_model_->calculate(financials[0], financials[1]);
    result.fraud_triangle = fraud_triangle_model_->calculate(financials);
    
    auto values = extract_all_values(financials);
    result.benford = benford_model_->calculate(values);
    
    // Detect red flags
    result.red_flags = detect_red_flags(result);
    
    // Analyze trends
    result.trends = analyze_trends(financials);
    
    // Calculate composite score
    result.composite_risk_score = calculate_composite_score(result);
    result.overall_risk_level = determine_risk_level(result.composite_risk_score);
    result.recommendation = generate_recommendation(result);
    result.risk_summary = "Analysis complete with " + std::to_string(result.red_flags.size()) + " red flags detected.";
    
    return result;
}

double FraudAnalyzer::calculate_composite_score(const AnalysisResult& result) {
    double score = 0.0;
    
    if (result.beneish) {
        score += weights_.beneish * result.beneish->risk_score;
    }
    if (result.altman) {
        score += weights_.altman * result.altman->risk_score;
    }
    if (result.piotroski) {
        score += weights_.piotroski * result.piotroski->risk_score;
    }
    if (result.fraud_triangle) {
        score += weights_.fraud_triangle * result.fraud_triangle->overall_risk;
    }
    if (result.benford) {
        double benford_risk = result.benford->is_suspicious ? 0.8 : 0.2;
        score += weights_.benford * benford_risk;
    }
    
    // Red flags contribution
    double flag_risk = std::min(1.0, result.red_flags.size() / 5.0);
    score += weights_.red_flags * flag_risk;
    
    return std::min(1.0, std::max(0.0, score));
}

RiskLevel FraudAnalyzer::determine_risk_level(double score) {
    if (score >= 0.8) return RiskLevel::CRITICAL;
    if (score >= 0.6) return RiskLevel::HIGH;
    if (score >= 0.4) return RiskLevel::ELEVATED;
    if (score >= 0.2) return RiskLevel::MODERATE;
    return RiskLevel::LOW;
}

std::string FraudAnalyzer::generate_recommendation(const AnalysisResult& result) {
    switch (result.overall_risk_level) {
        case RiskLevel::CRITICAL:
            return "CRITICAL RISK: Multiple fraud indicators detected. Recommend immediate detailed investigation.";
        case RiskLevel::HIGH:
            return "HIGH RISK: Significant fraud indicators present. Exercise extreme caution and conduct thorough due diligence.";
        case RiskLevel::ELEVATED:
            return "ELEVATED RISK: Some concerning indicators detected. Recommend additional scrutiny of financial statements.";
        case RiskLevel::MODERATE:
            return "MODERATE RISK: Minor concerns noted. Standard due diligence procedures recommended.";
        case RiskLevel::LOW:
        default:
            return "LOW RISK: No significant fraud indicators detected. Financial statements appear consistent with expected patterns.";
    }
}

std::vector<RedFlag> FraudAnalyzer::detect_red_flags(const AnalysisResult& result) {
    std::vector<RedFlag> flags;
    
    if (result.beneish && result.beneish->likely_manipulator) {
        flags.push_back({
            "EARNINGS_MANIPULATION",
            "Beneish M-Score Above Threshold",
            "M-Score indicates potential earnings manipulation",
            RiskLevel::HIGH,
            "Beneish Model",
            0.9
        });
    }
    
    if (result.altman && result.altman->z_score < 1.81) {
        flags.push_back({
            "BANKRUPTCY_RISK",
            "Altman Z-Score in Distress Zone",
            "High probability of bankruptcy within 2 years",
            RiskLevel::HIGH,
            "Altman Model",
            0.85
        });
    }
    
    if (result.piotroski && result.piotroski->f_score <= 3) {
        flags.push_back({
            "WEAK_FUNDAMENTALS",
            "Low Piotroski F-Score",
            "Financial fundamentals indicate weakness",
            RiskLevel::ELEVATED,
            "Piotroski Model",
            0.7
        });
    }
    
    if (result.fraud_triangle && result.fraud_triangle->overall_risk > 0.6) {
        flags.push_back({
            "FRAUD_TRIANGLE",
            "High Fraud Triangle Risk",
            "Multiple fraud risk factors detected",
            RiskLevel::HIGH,
            "Fraud Triangle Model",
            0.8
        });
    }
    
    if (result.benford && result.benford->is_suspicious) {
        flags.push_back({
            "BENFORD_ANOMALY",
            "Benford's Law Deviation",
            "Unusual digit distribution in financial figures",
            RiskLevel::ELEVATED,
            "Benford Model",
            0.65
        });
    }
    
    return flags;
}

TrendAnalysis FraudAnalyzer::analyze_trends(const std::vector<FinancialData>& financials) {
    TrendAnalysis trends;
    
    if (financials.size() < 2) return trends;
    
    // Compare first and last periods
    const auto& recent = financials.front();
    const auto& prior = financials.back();
    
    // Revenue trend
    if (recent.income_statement.revenue > prior.income_statement.revenue * 1.05) {
        trends.revenue_trend = TrendDirection::IMPROVING;
    } else if (recent.income_statement.revenue < prior.income_statement.revenue * 0.95) {
        trends.revenue_trend = TrendDirection::DECLINING;
    }
    
    // Income trend
    if (recent.income_statement.net_income > prior.income_statement.net_income * 1.05) {
        trends.income_trend = TrendDirection::IMPROVING;
    } else if (recent.income_statement.net_income < prior.income_statement.net_income * 0.95) {
        trends.income_trend = TrendDirection::DECLINING;
    }
    
    return trends;
}

std::vector<double> FraudAnalyzer::extract_all_values(const std::vector<FinancialData>& financials) {
    std::vector<double> values;
    for (const auto& f : financials) {
        values.push_back(f.income_statement.revenue);
        values.push_back(f.income_statement.net_income);
        values.push_back(f.balance_sheet.total_assets);
        values.push_back(f.balance_sheet.total_liabilities);
        values.push_back(f.cash_flow.operating_cash_flow);
    }
    return values;
}

} // namespace sec_analyzer
