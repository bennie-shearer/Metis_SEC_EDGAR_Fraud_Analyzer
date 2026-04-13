/**
 * SEC EDGAR Fraud Analyzer - Result Exporter Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/exporter.h>
#include <sec_analyzer/json.h>
#include <sec_analyzer/util.h>

#include <sstream>
#include <iomanip>
#include <cmath>

namespace sec_analyzer {

std::string ResultExporter::to_json(const AnalysisResult& result, bool pretty) {
    JsonObject root;
    
    // Company info
    JsonObject company;
    company["name"] = result.company.name;
    company["ticker"] = result.company.ticker;
    company["cik"] = result.company.cik;
    company["sic"] = result.company.sic;
    root["company"] = company;
    
    root["ticker"] = result.company.ticker;
    root["filings_analyzed"] = static_cast<double>(result.filings_analyzed);
    
    // Overall risk
    JsonObject risk;
    risk["score"] = result.composite_risk_score;
    risk["level"] = risk_level_to_string(result.overall_risk_level);
    risk["summary"] = result.risk_summary;
    root["overall_risk"] = risk;
    
    root["recommendation"] = result.recommendation;
    
    // Models
    JsonObject models;
    
    if (result.beneish) {
        JsonObject beneish;
        beneish["m_score"] = result.beneish->m_score;
        beneish["dsri"] = result.beneish->dsri;
        beneish["gmi"] = result.beneish->gmi;
        beneish["aqi"] = result.beneish->aqi;
        beneish["sgi"] = result.beneish->sgi;
        beneish["depi"] = result.beneish->depi;
        beneish["sgai"] = result.beneish->sgai;
        beneish["lvgi"] = result.beneish->lvgi;
        beneish["tata"] = result.beneish->tata;
        beneish["likely_manipulator"] = result.beneish->likely_manipulator;
        beneish["zone"] = result.beneish->zone;
        models["beneish"] = beneish;
    }
    
    if (result.altman) {
        JsonObject altman;
        altman["z_score"] = result.altman->z_score;
        altman["x1"] = result.altman->x1;
        altman["x2"] = result.altman->x2;
        altman["x3"] = result.altman->x3;
        altman["x4"] = result.altman->x4;
        altman["x5"] = result.altman->x5;
        altman["zone"] = result.altman->zone;
        altman["bankruptcy_probability"] = result.altman->bankruptcy_probability;
        models["altman"] = altman;
    }
    
    if (result.piotroski) {
        JsonObject piotroski;
        piotroski["f_score"] = static_cast<double>(result.piotroski->f_score);
        piotroski["interpretation"] = result.piotroski->interpretation;
        models["piotroski"] = piotroski;
    }
    
    if (result.fraud_triangle) {
        JsonObject ft;
        ft["pressure_score"] = result.fraud_triangle->pressure_score;
        ft["opportunity_score"] = result.fraud_triangle->opportunity_score;
        ft["rationalization_score"] = result.fraud_triangle->rationalization_score;
        ft["overall_risk"] = result.fraud_triangle->overall_risk;
        ft["risk_level"] = risk_level_to_string(result.fraud_triangle->risk_level);
        models["fraud_triangle"] = ft;
    }
    
    if (result.benford) {
        JsonObject benford;
        benford["deviation"] = result.benford->deviation_percent;
        benford["chi_square"] = result.benford->chi_square;
        benford["mad"] = result.benford->mad;
        benford["suspicious"] = result.benford->is_suspicious;
        models["benford"] = benford;
    }
    
    root["models"] = models;
    
    // Red flags
    JsonArray flags;
    for (const auto& flag : result.red_flags) {
        JsonObject f;
        f["type"] = flag.type;
        f["title"] = flag.title;
        f["description"] = flag.description;
        f["severity"] = risk_level_to_string(flag.severity);
        flags.push_back(f);
    }
    root["red_flags"] = flags;
    
    // Trends
    JsonObject trends;
    trends["revenue_trend"] = trend_to_string(result.trends.revenue_trend);
    trends["income_trend"] = trend_to_string(result.trends.income_trend);
    trends["cash_flow_trend"] = trend_to_string(result.trends.cash_flow_trend);
    trends["debt_trend"] = trend_to_string(result.trends.debt_trend);
    root["trends"] = trends;
    
    // Filings summary
    JsonArray filings;
    for (const auto& fin : result.filings) {
        JsonObject f;
        f["accession"] = fin.filing.accession_number;
        f["form_type"] = fin.filing.form_type;
        f["filed_date"] = fin.filing.filed_date;
        f["revenue"] = fin.income_statement.revenue;
        f["net_income"] = fin.income_statement.net_income;
        filings.push_back(f);
    }
    root["filings"] = filings;
    
    // Metadata
    root["version"] = result.version;
    root["analysis_timestamp"] = result.analysis_timestamp;
    
    return JsonValue(root).dump(pretty ? 2 : -1);
}

std::string ResultExporter::to_csv(const AnalysisResult& result) {
    std::ostringstream oss;
    
    // Header
    oss << "Metric,Value\n";
    
    // Company info
    oss << "Company," << escape_json(result.company.name) << "\n";
    oss << "Ticker," << result.company.ticker << "\n";
    oss << "CIK," << result.company.cik << "\n";
    oss << "Filings Analyzed," << result.filings_analyzed << "\n";
    
    // Risk
    oss << "Risk Score," << std::fixed << std::setprecision(4) << result.composite_risk_score << "\n";
    oss << "Risk Level," << risk_level_to_string(result.overall_risk_level) << "\n";
    
    // Model scores
    if (result.beneish) {
        oss << "Beneish M-Score," << result.beneish->m_score << "\n";
    }
    if (result.altman) {
        oss << "Altman Z-Score," << result.altman->z_score << "\n";
    }
    if (result.piotroski) {
        oss << "Piotroski F-Score," << result.piotroski->f_score << "\n";
    }
    if (result.fraud_triangle) {
        oss << "Fraud Triangle Risk," << result.fraud_triangle->overall_risk << "\n";
    }
    if (result.benford) {
        oss << "Benford Deviation," << result.benford->deviation_percent << "%\n";
    }
    
    // Red flags
    oss << "Red Flags Count," << result.red_flags.size() << "\n";
    
    return oss.str();
}

std::string ResultExporter::filings_to_csv(const std::vector<FinancialData>& filings) {
    std::ostringstream oss;
    
    oss << "Accession,Form,Filed Date,Revenue,Net Income,Total Assets,Total Liabilities\n";
    
    for (const auto& f : filings) {
        oss << f.filing.accession_number << ","
            << f.filing.form_type << ","
            << f.filing.filed_date << ","
            << f.income_statement.revenue << ","
            << f.income_statement.net_income << ","
            << f.balance_sheet.total_assets << ","
            << f.balance_sheet.total_liabilities << "\n";
    }
    
    return oss.str();
}

std::string ResultExporter::to_html(const AnalysisResult& result) {
    std::ostringstream oss;
    
    oss << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fraud Analysis Report - )" << escape_html(result.company.ticker) << R"(</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; 
               max-width: 900px; margin: 0 auto; padding: 20px; background: #f5f5f5; }
        .report { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        h1 { color: #1f2937; border-bottom: 2px solid #2563eb; padding-bottom: 10px; }
        h2 { color: #374151; margin-top: 30px; }
        .meta { color: #6b7280; font-size: 14px; margin-bottom: 20px; }
        .risk-low { color: #10b981; }
        .risk-moderate { color: #f59e0b; }
        .risk-high { color: #ef4444; }
        .risk-critical { color: #7c2d12; font-weight: bold; }
        .score-card { display: inline-block; padding: 15px 25px; margin: 10px; background: #f3f4f6; 
                      border-radius: 8px; text-align: center; }
        .score-value { font-size: 2em; font-weight: bold; }
        .score-label { font-size: 12px; color: #6b7280; text-transform: uppercase; }
        table { width: 100%; border-collapse: collapse; margin: 15px 0; }
        th, td { padding: 10px; text-align: left; border-bottom: 1px solid #e5e7eb; }
        th { background: #f9fafb; font-weight: 600; }
        .red-flag { background: #fef2f2; border-left: 4px solid #ef4444; padding: 10px 15px; 
                    margin: 10px 0; border-radius: 4px; }
        footer { margin-top: 30px; padding-top: 20px; border-top: 1px solid #e5e7eb; 
                 font-size: 12px; color: #6b7280; text-align: center; }
    </style>
</head>
<body>
    <div class="report">
        <h1>SEC EDGAR Fraud Analysis Report</h1>
        <div class="meta">
            <p><strong>Company:</strong> )" << escape_html(result.company.name) << R"(</p>
            <p><strong>Ticker:</strong> )" << result.company.ticker << R"( | <strong>CIK:</strong> )" << result.company.cik << R"(</p>
            <p><strong>Generated:</strong> )" << result.analysis_timestamp << R"(</p>
        </div>
        
        <h2>Risk Summary</h2>
        <div class="score-card">
            <div class="score-value )" << risk_level_class(result.overall_risk_level) << R"(">)" 
            << std::fixed << std::setprecision(0) << (result.composite_risk_score * 100) << R"(%</div>
            <div class="score-label">Overall Risk</div>
        </div>
        <div class="score-card">
            <div class="score-value">)" << result.filings_analyzed << R"(</div>
            <div class="score-label">Filings Analyzed</div>
        </div>
        <div class="score-card">
            <div class="score-value )" << (result.red_flags.size() > 3 ? "risk-high" : "") << R"(">)" 
            << result.red_flags.size() << R"(</div>
            <div class="score-label">Red Flags</div>
        </div>
        
        <h2>Model Scores</h2>
        <table>
            <tr><th>Model</th><th>Score</th><th>Interpretation</th></tr>)";
    
    if (result.beneish) {
        oss << R"(<tr><td>Beneish M-Score</td><td>)" << std::setprecision(2) << result.beneish->m_score 
            << R"(</td><td>)" << (result.beneish->likely_manipulator ? "Likely Manipulator" : "Unlikely Manipulator") << "</td></tr>";
    }
    if (result.altman) {
        oss << R"(<tr><td>Altman Z-Score</td><td>)" << result.altman->z_score 
            << R"(</td><td>)" << result.altman->zone << "</td></tr>";
    }
    if (result.piotroski) {
        oss << R"(<tr><td>Piotroski F-Score</td><td>)" << result.piotroski->f_score 
            << R"(</td><td>)" << result.piotroski->interpretation << "</td></tr>";
    }
    
    oss << R"(</table>
        
        <h2>Red Flags</h2>)";
    
    if (result.red_flags.empty()) {
        oss << R"(<p style="color:#10b981;">No significant red flags detected.</p>)";
    } else {
        for (const auto& flag : result.red_flags) {
            oss << R"(<div class="red-flag"><strong>)" << escape_html(flag.title) 
                << R"(</strong><br>)" << escape_html(flag.description) << "</div>";
        }
    }
    
    oss << R"(
        <h2>Recommendation</h2>
        <p>)" << escape_html(result.recommendation) << R"(</p>
        
        <footer>
            <p>SEC EDGAR Fraud Analyzer v)" << result.version << R"(</p>
            <p>Author: Bennie Shearer (Retired) | For educational and research purposes</p>
        </footer>
    </div>
</body>
</html>)";
    
    return oss.str();
}

std::string ResultExporter::health_json(const std::string& version, int cache_entries) {
    JsonObject obj;
    obj["status"] = "healthy";
    obj["version"] = version;
    obj["timestamp"] = util::get_timestamp();
    obj["cache_entries"] = static_cast<double>(cache_entries);
    return JsonValue(obj).dump();
}

std::string ResultExporter::error_json(const std::string& message, int code) {
    JsonObject obj;
    obj["error"] = message;
    obj["code"] = static_cast<double>(code);
    return JsonValue(obj).dump();
}

std::string ResultExporter::escape_json(const std::string& s) {
    return util::json_escape(s);
}

std::string ResultExporter::escape_html(const std::string& s) {
    return util::html_escape(s);
}

std::string ResultExporter::risk_level_class(RiskLevel level) {
    switch (level) {
        case RiskLevel::LOW: return "risk-low";
        case RiskLevel::MODERATE: return "risk-moderate";
        case RiskLevel::ELEVATED: return "risk-moderate";
        case RiskLevel::HIGH: return "risk-high";
        case RiskLevel::CRITICAL: return "risk-critical";
        default: return "";
    }
}

std::string ResultExporter::format_currency(double value) {
    return util::format_currency(value);
}

std::string ResultExporter::format_percentage(double value) {
    return util::format_percentage(value);
}

} // namespace sec_analyzer
