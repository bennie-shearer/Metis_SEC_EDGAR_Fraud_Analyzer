/**
 * Metis SEC EDGAR Fraud Analyzer - Main Entry Point
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * Copyright (c) 2026 Bennie Shearer (Retired)
 *
 * DISCLAIMER: This project is NOT funded, endorsed, or approved by the
 * U.S. Securities and Exchange Commission (SEC).
 *
 * Cross-platform C++20 fraud detection server for SEC EDGAR filings.
 * All parameters are sourced from config.pson.
 */

#include <sec_analyzer/version.h>
#include <sec_analyzer/types.h>
#include <sec_analyzer/logger.h>
#include <sec_analyzer/json.h>
#include <sec_analyzer/util.h>
#include <sec_analyzer/pson.h>
#include <sec_analyzer/config.h>
#include <sec_analyzer/cache.h>
#include <sec_analyzer/http_server.h>
#include <sec_analyzer/sec_fetcher.h>
#include <sec_analyzer/analyzer.h>
#include <sec_analyzer/exporter.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace sec_analyzer;

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static std::atomic<bool> g_running{true};
static std::unique_ptr<HttpServer> g_server;

// ---------------------------------------------------------------------------
// Signal handler
// ---------------------------------------------------------------------------
void signal_handler(int signal) {
    LOG_INFO("Received signal {}, shutting down...", signal);
    g_running = false;
    if (g_server) {
        g_server->stop();
    }
}

// ---------------------------------------------------------------------------
// Banner
// ---------------------------------------------------------------------------
void print_banner() {
    std::cout << "\n";
    std::cout << "+===============================================================+\n";
    std::cout << "|            Metis SEC EDGAR Fraud Analyzer                     |\n";
    std::cout << "|                    Version " << SEC_ANALYZER_VERSION_STRING << "                          |\n";
    std::cout << "|                                                               |\n";
    std::cout << "|  Author : Bennie Shearer (Retired)                            |\n";
    std::cout << "|  License: MIT                                                 |\n";
    std::cout << "+===============================================================+\n";
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// Usage
// ---------------------------------------------------------------------------
void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --config <file>     Load configuration from .pson file (default: config.pson)\n";
    std::cout << "  --port <port>       Override server port\n";
    std::cout << "  --static <dir>      Override static files directory\n";
    std::cout << "  --log-level <lvl>   Override log level: debug|info|warning|error|critical\n";
    std::cout << "  --log-file <file>   Override log file path\n";
    std::cout << "  --verbose           Shorthand for --log-level debug\n";
    std::cout << "  --quiet             Shorthand for --log-level error\n";
    std::cout << "  --version           Print version information\n";
    std::cout << "  --help              Print this message\n";
    std::cout << "\nCLI Mode (no server started):\n";
    std::cout << "  --ticker <sym>      Analyze company by ticker symbol\n";
    std::cout << "  --cik <num>         Analyze company by CIK number\n";
    std::cout << "  --years <n>         Number of years to analyze (default: 5)\n";
    std::cout << "  --format <fmt>      Output format: json|csv|html (default: json)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program << "\n";
    std::cout << "  " << program << " --config config.pson\n";
    std::cout << "  " << program << " --ticker AAPL --years 3 --format json\n";
    std::cout << "  " << program << " --cik 0001024401 --format html > report.html\n\n";
}

// ---------------------------------------------------------------------------
// CLI analysis mode
// ---------------------------------------------------------------------------
int run_cli_analysis(const AppConfig& cfg,
                     const std::string& ticker, const std::string& cik,
                     int years, const std::string& format) {
    LOG_INFO("Running CLI analysis...");

    auto fetcher = std::make_shared<SECFetcher>(cfg.sec_user_agent);
    auto analyzer = std::make_shared<FraudAnalyzer>(cfg.weights);
    analyzer->set_fetcher(fetcher);

    AnalysisResult result;
    if (!ticker.empty()) {
        LOG_INFO("Analyzing ticker: {}", ticker);
        result = analyzer->analyze_by_ticker(ticker, years);
    } else if (!cik.empty()) {
        LOG_INFO("Analyzing CIK: {}", cik);
        result = analyzer->analyze_by_cik(cik, years);
    } else {
        LOG_ERROR("No ticker or CIK specified");
        return 1;
    }

    if (analyzer->has_error()) {
        LOG_ERROR("Analysis failed: {}", analyzer->get_last_error());
        std::cerr << "Error: " << analyzer->get_last_error() << "\n";
        return 1;
    }

    std::string output;
    if (format == "csv") {
        output = ResultExporter::to_csv(result);
    } else if (format == "html") {
        output = ResultExporter::to_html(result);
    } else {
        output = ResultExporter::to_json(result, true);
    }

    std::cout << output << "\n";
    return 0;
}

// ---------------------------------------------------------------------------
// REST API route registration
// ---------------------------------------------------------------------------
void setup_routes(HttpServer& server,
                  std::shared_ptr<SECFetcher> fetcher,
                  std::shared_ptr<FraudAnalyzer> analyzer,
                  std::shared_ptr<Cache<std::string>> cache,
                  AppConfig cfg) {          // by value: lambdas capture a copy, no dangling ref

    // ---- GET /config.pson ------------------------------------------------
    // Serves the live config file to the browser client so it can read
    // client.api_url and auto-connect without manual Connection Settings.
    // The file is resolved from the same directory as the other runtime files.
    server.get("/config.pson", [cfg](const HttpRequest&) {
        // cfg.static_dir is already absolute (e.g. .../bin/client).
        // config.pson lives one level up, in .../bin/.
        std::string pson_path;
        size_t sep = cfg.static_dir.find_last_of("/\\");
        if (sep != std::string::npos) {
            pson_path = cfg.static_dir.substr(0, sep + 1) + "config.pson";
        } else {
            pson_path = "config.pson";
        }
        std::ifstream f(pson_path);
        if (!f.is_open()) {
            // Fall back to a minimal inline response so the browser still connects
            return HttpResponse::ok(
                "# config.pson not found on server\nclient.api_url=http://localhost:"
                + std::to_string(cfg.server_port) + "\n",
                "text/plain; charset=utf-8");
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        return HttpResponse::ok(ss.str(), "text/plain; charset=utf-8");
    });

    // ---- GET /api/health -------------------------------------------------
    server.get("/api/health", [cache](const HttpRequest&) {
        std::string json = ResultExporter::health_json(
            SEC_ANALYZER_VERSION_STRING,
            static_cast<int>(cache->size())
        );
        return HttpResponse::ok(json);
    });

    // ---- GET /api/version ------------------------------------------------
    server.get("/api/version", [](const HttpRequest&) {
        JsonObject obj;
        obj["version"]   = SEC_ANALYZER_VERSION_STRING;
        obj["app_name"]  = SEC_ANALYZER_APP_NAME;
        obj["copyright"] = SEC_ANALYZER_COPYRIGHT;
        obj["license"]   = SEC_ANALYZER_LICENSE;
        obj["disclaimer"]= SEC_ANALYZER_DISCLAIMER;
        return HttpResponse::ok(JsonValue(obj).dump());
    });

    // ---- GET /api/company ------------------------------------------------
    server.get("/api/company", [fetcher](const HttpRequest& req) {
        std::string ticker = req.get_param("ticker");
        std::string cik    = req.get_param("cik");

        if (ticker.empty() && cik.empty())
            return HttpResponse::bad_request("Missing ticker or cik parameter");

        std::optional<CompanyInfo> company;
        if (!ticker.empty())
            company = fetcher->lookup_company_by_ticker(ticker);
        else
            company = fetcher->lookup_company_by_cik(cik);

        if (!company)
            return HttpResponse::not_found();

        JsonObject obj;
        obj["name"]   = company->name;
        obj["ticker"] = company->ticker;
        obj["cik"]    = company->cik;
        obj["sic"]    = company->sic;
        return HttpResponse::ok(JsonValue(obj).dump());
    });

    // ---- GET /api/analyze ------------------------------------------------
    server.get("/api/analyze", [analyzer, cache, cfg](const HttpRequest& req) {
        std::string ticker = req.get_param("ticker");
        std::string cik    = req.get_param("cik");
        int years = cfg.default_years;
        try { years = std::stoi(req.get_param("years", std::to_string(cfg.default_years))); }
        catch (...) {}
        years = std::min(years, cfg.max_years);

        if (ticker.empty() && cik.empty())
            return HttpResponse::bad_request("Missing ticker or cik parameter");

        std::string cache_key = "analysis:" + (ticker.empty() ? cik : ticker)
                              + ":" + std::to_string(years);
        auto cached = cache->get(cache_key);
        if (cached) {
            LOG_DEBUG("Cache hit for {}", cache_key);
            return HttpResponse::ok(*cached);
        }

        AnalysisResult result;
        if (!ticker.empty())
            result = analyzer->analyze_by_ticker(ticker, years);
        else
            result = analyzer->analyze_by_cik(cik, years);

        if (analyzer->has_error())
            return HttpResponse::error(500, analyzer->get_last_error());

        std::string json = ResultExporter::to_json(result);
        cache->set(cache_key, json);
        return HttpResponse::ok(json);
    });

    // ---- GET /api/filings ------------------------------------------------
    server.get("/api/filings", [fetcher, cfg](const HttpRequest& req) {
        std::string ticker = req.get_param("ticker");
        std::string cik    = req.get_param("cik");
        int years = cfg.default_years;
        try { years = std::stoi(req.get_param("years", std::to_string(cfg.default_years))); }
        catch (...) {}

        if (ticker.empty() && cik.empty())
            return HttpResponse::bad_request("Missing ticker or cik parameter");

        std::string target_cik = cik;
        if (!ticker.empty()) {
            auto company = fetcher->lookup_company_by_ticker(ticker);
            if (!company) return HttpResponse::not_found();
            target_cik = company->cik;
        }

        auto filings = fetcher->get_filings(target_cik, years);

        JsonArray arr;
        for (const auto& f : filings) {
            JsonObject obj;
            obj["accession"] = f.accession_number;
            obj["form_type"] = f.form_type;
            obj["filed_date"]= f.filed_date;
            obj["fiscal_year"]= static_cast<double>(f.fiscal_year);
            arr.push_back(obj);
        }

        JsonObject result;
        result["filings"] = arr;
        result["count"]   = static_cast<double>(filings.size());
        return HttpResponse::ok(JsonValue(result).dump());
    });

    // ---- GET /api/search -------------------------------------------------
    server.get("/api/search", [fetcher](const HttpRequest& req) {
        std::string query = req.get_param("q");
        if (query.empty())
            return HttpResponse::bad_request("Missing q parameter");

        auto companies = fetcher->search_companies(query);

        JsonArray arr;
        for (const auto& c : companies) {
            JsonObject obj;
            obj["name"]   = c.name;
            obj["ticker"] = c.ticker;
            obj["cik"]    = c.cik;
            arr.push_back(obj);
        }

        JsonObject result;
        result["results"] = arr;
        return HttpResponse::ok(JsonValue(result).dump());
    });

    // ---- GET /api/models -------------------------------------------------
    // Returns the list of available models and their descriptions.
    // Built as a raw JSON string to avoid GCC 13 -Wmaybe-uninitialized
    // false positives on std::variant move in JsonArray push_back.
    server.get("/api/models", [](const HttpRequest&) {
        const char* json =
            "{"
              "\"models\":["
                "{\"id\":\"beneish\","
                 "\"name\":\"Beneish M-Score\","
                 "\"description\":\"Detects earnings manipulation; M > -2.22 indicates likely manipulator\"},"
                "{\"id\":\"altman\","
                 "\"name\":\"Altman Z-Score\","
                 "\"description\":\"Predicts bankruptcy probability; Z < 1.81 = distress zone\"},"
                "{\"id\":\"piotroski\","
                 "\"name\":\"Piotroski F-Score\","
                 "\"description\":\"0-9 financial strength score; <= 3 is weak, >= 7 is strong\"},"
                "{\"id\":\"fraud_triangle\","
                 "\"name\":\"Fraud Triangle\","
                 "\"description\":\"Assesses Pressure, Opportunity, and Rationalization indicators\"},"
                "{\"id\":\"benford\","
                 "\"name\":\"Benford's Law\","
                 "\"description\":\"Checks digit distribution of financial figures for anomalies\"}"
              "]"
            "}";
        return HttpResponse::ok(json);
    });

    // ---- GET /api/weights ------------------------------------------------
    server.get("/api/weights", [cfg](const HttpRequest&) {
        JsonObject obj;
        obj["beneish"]        = cfg.weights.beneish;
        obj["altman"]         = cfg.weights.altman;
        obj["piotroski"]      = cfg.weights.piotroski;
        obj["fraud_triangle"] = cfg.weights.fraud_triangle;
        obj["benford"]        = cfg.weights.benford;
        obj["red_flags"]      = cfg.weights.red_flags;
        return HttpResponse::ok(JsonValue(obj).dump());
    });

    // ---- POST /api/cache/clear -------------------------------------------
    server.post("/api/cache/clear", [cache](const HttpRequest&) {
        cache->clear();
        return HttpResponse::ok("{\"status\":\"cleared\"}");
    });

    // ---- GET /api/cache/stats --------------------------------------------
    server.get("/api/cache/stats", [cache](const HttpRequest&) {
        JsonObject obj;
        obj["entries"] = static_cast<double>(cache->size());
        obj["ttl_seconds"] = static_cast<double>(cache->get_ttl());
        return HttpResponse::ok(JsonValue(obj).dump());
    });

    // ---- GET /api/export/json --------------------------------------------
    server.get("/api/export/json", [analyzer, cfg](const HttpRequest& req) {
        std::string ticker = req.get_param("ticker");
        std::string cik    = req.get_param("cik");
        int years = cfg.default_years;
        try { years = std::stoi(req.get_param("years", std::to_string(cfg.default_years))); }
        catch (...) {}

        if (ticker.empty() && cik.empty())
            return HttpResponse::bad_request("Missing ticker or cik parameter");

        AnalysisResult result;
        if (!ticker.empty())
            result = analyzer->analyze_by_ticker(ticker, years);
        else
            result = analyzer->analyze_by_cik(cik, years);

        if (analyzer->has_error())
            return HttpResponse::error(500, analyzer->get_last_error());

        std::string json = ResultExporter::to_json(result, true);
        HttpResponse res = HttpResponse::ok(json, "application/json");
        std::string fname = (ticker.empty() ? cik : ticker) + "_analysis.json";
        res.headers["Content-Disposition"] = "attachment; filename=\"" + fname + "\"";
        return res;
    });

    // ---- GET /api/export/csv ---------------------------------------------
    server.get("/api/export/csv", [analyzer, cfg](const HttpRequest& req) {
        std::string ticker = req.get_param("ticker");
        std::string cik    = req.get_param("cik");
        int years = cfg.default_years;
        try { years = std::stoi(req.get_param("years", std::to_string(cfg.default_years))); }
        catch (...) {}

        if (ticker.empty() && cik.empty())
            return HttpResponse::bad_request("Missing ticker or cik parameter");

        AnalysisResult result;
        if (!ticker.empty())
            result = analyzer->analyze_by_ticker(ticker, years);
        else
            result = analyzer->analyze_by_cik(cik, years);

        if (analyzer->has_error())
            return HttpResponse::error(500, analyzer->get_last_error());

        std::string csv = ResultExporter::to_csv(result);
        HttpResponse res = HttpResponse::ok(csv, "text/csv");
        std::string fname = (ticker.empty() ? cik : ticker) + "_analysis.csv";
        res.headers["Content-Disposition"] = "attachment; filename=\"" + fname + "\"";
        return res;
    });

    // ---- GET /api/export/html --------------------------------------------
    server.get("/api/export/html", [analyzer, cfg](const HttpRequest& req) {
        std::string ticker = req.get_param("ticker");
        std::string cik    = req.get_param("cik");
        int years = cfg.default_years;
        try { years = std::stoi(req.get_param("years", std::to_string(cfg.default_years))); }
        catch (...) {}

        if (ticker.empty() && cik.empty())
            return HttpResponse::bad_request("Missing ticker or cik parameter");

        AnalysisResult result;
        if (!ticker.empty())
            result = analyzer->analyze_by_ticker(ticker, years);
        else
            result = analyzer->analyze_by_cik(cik, years);

        if (analyzer->has_error())
            return HttpResponse::error(500, analyzer->get_last_error());

        std::string html = ResultExporter::to_html(result);
        return HttpResponse::ok(html, "text/html");
    });

    // ---- GPU stub (reserved for future C++20 GPU backend) ----------------
    server.get("/api/gpu/status", [cfg](const HttpRequest&) {
        JsonObject obj;
        obj["enabled"] = cfg.gpu_enabled;
        obj["device"]  = cfg.gpu_device;
        obj["status"]  = cfg.gpu_enabled ? "reserved" : "disabled";
        obj["note"]    = "GPU acceleration is reserved for a future C++20 implementation";
        return HttpResponse::ok(JsonValue(obj).dump());
    });

    // ---- Kubernetes stub (reserved) --------------------------------------
    server.get("/api/k8s/status", [cfg](const HttpRequest&) {
        JsonObject obj;
        obj["enabled"]   = cfg.k8s_enabled;
        obj["namespace"] = cfg.k8s_namespace;
        obj["status"]    = cfg.k8s_enabled ? "reserved" : "disabled";
        obj["note"]      = "Kubernetes/container deployment is reserved for a future C++20 implementation";
        return HttpResponse::ok(JsonValue(obj).dump());
    });

    // ---- POST /api/shutdown -----------------------------------------------
    // Gracefully stops the server. Called by the browser Exit button.
    server.post("/api/shutdown", [](const HttpRequest& req) {
        LOG_INFO("Shutdown requested from {}", req.client_ip);
        // Signal main loop to exit after response is sent
        g_running = false;
        if (g_server) {
            // Stop in a detached thread so the response can be sent first
            std::thread([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                if (g_server) g_server->stop();
            }).detach();
        }
        return HttpResponse::ok("{\"status\":\"shutting_down\"}");
    });

    // ---- GET /api/logs ---------------------------------------------------
    // Returns the tail of the server log file as plain text.
    // Query params:
    //   lines=N   number of lines to return from the end (default: 200)
    //   raw=1     return raw text instead of JSON
    server.get("/api/logs", [cfg](const HttpRequest& req) {
        if (cfg.log_file.empty()) {
            return HttpResponse::ok(
                "{\"error\":\"No log file configured\",\"lines\":[]}");
        }

        int max_lines = 200;
        try { max_lines = std::stoi(req.get_param("lines", "200")); }
        catch (...) {}
        if (max_lines < 1)   max_lines = 1;
        if (max_lines > 5000) max_lines = 5000;

        bool raw_mode = (req.get_param("raw") == "1");

        // Read the log file
        std::ifstream file(cfg.log_file);
        if (!file.is_open()) {
            return HttpResponse::ok(
                "{\"error\":\"Log file not found or not readable\",\"lines\":[],\"path\":\""
                + util::json_escape(cfg.log_file) + "\"}");
        }

        // Collect all lines then take the tail
        std::vector<std::string> all_lines;
        std::string line;
        while (std::getline(file, line)) {
            all_lines.push_back(line);
        }

        int start = std::max(0, static_cast<int>(all_lines.size()) - max_lines);

        if (raw_mode) {
            std::string text;
            for (int i = start; i < static_cast<int>(all_lines.size()); ++i) {
                text += all_lines[i] + "\n";
            }
            return HttpResponse::ok(text, "text/plain; charset=utf-8");
        }

        // Build JSON response
        JsonArray arr;
        for (int i = start; i < static_cast<int>(all_lines.size()); ++i) {
            arr.push_back(JsonValue(all_lines[i]));
        }
        JsonObject result;
        result["lines"]       = arr;
        result["total_lines"] = static_cast<double>(all_lines.size());
        result["returned"]    = static_cast<double>(arr.size());
        result["path"]        = cfg.log_file;
        return HttpResponse::ok(JsonValue(result).dump());
    });

    // ---- POST /api/logs/clear --------------------------------------------
    // Truncates the server log file. A new log entry is written afterwards.
    server.post("/api/logs/clear", [cfg](const HttpRequest& req) {
        if (cfg.log_file.empty()) {
            return HttpResponse::bad_request("No log file configured");
        }
        // Open with truncation
        std::ofstream f(cfg.log_file, std::ios::out | std::ios::trunc);
        if (!f.is_open()) {
            return HttpResponse::error(500, "Failed to clear log file");
        }
        f.close();
        LOG_INFO("Log file cleared by request from {}", req.client_ip);
        return HttpResponse::ok("{\"status\":\"cleared\",\"path\":\""
                                + util::json_escape(cfg.log_file) + "\"}");
    });
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    print_banner();

    // --- Defaults -----------------------------------------------------------
    AppConfig cfg;
    std::string config_file = "config.pson";

    // CLI overrides
    std::string cli_ticker;
    std::string cli_cik;
    int         cli_years  = -1;       // -1 = use config default
    std::string cli_format = "json";
    bool        cli_mode   = false;

    // Override log level / file from command line before config is loaded
    std::string cli_log_level;
    std::string cli_log_file;
    int         cli_port   = -1;
    std::string cli_static;

    // --- Parse arguments ----------------------------------------------------
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        if (arg == "--version" || arg == "-v") {
            std::cout << get_version_info() << "\n";
            return 0;
        }
        if (arg == "--config" && i + 1 < argc)        { config_file    = argv[++i]; }
        else if (arg == "--port" && i + 1 < argc)     { cli_port       = std::stoi(argv[++i]); }
        else if (arg == "--static" && i + 1 < argc)   { cli_static     = argv[++i]; }
        else if (arg == "--log-level" && i + 1 < argc){ cli_log_level  = argv[++i]; }
        else if (arg == "--log-file" && i + 1 < argc) { cli_log_file   = argv[++i]; }
        else if (arg == "--verbose")                   { cli_log_level  = "debug"; }
        else if (arg == "--quiet")                     { cli_log_level  = "error"; }
        else if (arg == "--ticker" && i + 1 < argc)   { cli_ticker     = argv[++i]; cli_mode = true; }
        else if (arg == "--cik" && i + 1 < argc)      { cli_cik        = argv[++i]; cli_mode = true; }
        else if (arg == "--years" && i + 1 < argc)    { cli_years      = std::stoi(argv[++i]); }
        else if (arg == "--format" && i + 1 < argc)   { cli_format     = argv[++i]; }
    }

    // --- Locate config.pson -------------------------------------------------
    // Search order:
    //   1. Exact path provided (CWD-relative or absolute)
    //   2. Directory containing the executable (argv[0])
    //   3. Built-in defaults
    if (!util::file_exists(config_file)) {
        // Build a candidate path: <exe-dir>/config.pson
        std::string exe_path = argv[0];
        size_t sep = exe_path.find_last_of("/\\");
        std::string exe_dir = (sep != std::string::npos)
                              ? exe_path.substr(0, sep + 1)
                              : "./";
        std::string candidate = exe_dir + "config.pson";
        if (util::file_exists(candidate)) {
            config_file = candidate;
            std::cout << "Using config: " << config_file << "\n";
        }
    }

    // --- Load PSON config file ----------------------------------------------
    if (util::file_exists(config_file)) {
        if (cfg.load_file(config_file)) {
            std::cout << "Loaded configuration from " << config_file << "\n";
        } else {
            std::cerr << "Warning: failed to parse " << config_file << "\n";
        }
    } else {
        std::cout << "No config.pson found in CWD or exe directory - using built-in defaults.\n\n";
    }

    // Apply command-line overrides (higher priority than file)
    if (cli_port > 0)              cfg.server_port = cli_port;
    if (!cli_static.empty())       cfg.static_dir  = cli_static;
    if (!cli_log_level.empty())    cfg.log_level   = cli_log_level;
    if (!cli_log_file.empty())     cfg.log_file    = cli_log_file;
    if (cli_years > 0)             cfg.default_years = cli_years;

    // --- Resolve relative paths against the exe directory -------------------
    // When CLion runs the exe from cmake-build-release/bin/, relative paths
    // like "client" and "logs/metis.log" must resolve from there, not from
    // the project root. We prefix any relative path with the exe directory.
    {
        std::string exe_path = argv[0];
        size_t sep = exe_path.find_last_of("/\\");
        std::string exe_dir = (sep != std::string::npos)
                              ? exe_path.substr(0, sep + 1)
                              : "./";

        auto make_absolute = [&](std::string& path) {
            if (path.empty()) return;
            // Already absolute?
            bool is_abs = (path[0] == '/' || path[0] == '\\');
#ifdef _WIN32
            is_abs = is_abs || (path.size() >= 3 && path[1] == ':');
#endif
            if (!is_abs) {
                path = exe_dir + path;
            }
        };

        make_absolute(cfg.static_dir);
        make_absolute(cfg.cache_dir);
        make_absolute(cfg.log_file);
    }

    // --- Logging setup ------------------------------------------------------
    Logger::instance().set_level_from_string(cfg.log_level);
    Logger::instance().set_console_output(cfg.log_console);
    if (!cfg.log_file.empty()) {
        // Ensure log directory exists
        size_t sep = cfg.log_file.find_last_of("/\\");
        if (sep != std::string::npos) {
            util::create_directory(cfg.log_file.substr(0, sep));
        }
        Logger::instance().set_file(cfg.log_file);
        LOG_INFO("Logging to file: {}", cfg.log_file);
    }

    // --- CLI mode -----------------------------------------------------------
    if (cli_mode) {
        int years = (cfg.default_years > 0) ? cfg.default_years : 5;
        return run_cli_analysis(cfg, cli_ticker, cli_cik, years, cli_format);
    }

    // --- Server mode --------------------------------------------------------
    LOG_INFO("Starting Metis SEC EDGAR Fraud Analyzer v{}", SEC_ANALYZER_VERSION_STRING);
    LOG_INFO("Log level       : {} (console: {})", cfg.log_level, cfg.log_console ? "on" : "off");
    LOG_INFO("Log file        : {}", cfg.log_file.empty() ? "(none)" : cfg.log_file);
    LOG_INFO("Static dir      : {}", cfg.static_dir);
    LOG_INFO("Cache dir       : {}", cfg.cache_dir);
    LOG_INFO("Cache TTL       : {}s", cfg.cache_ttl_seconds);
    LOG_INFO("Port            : {}", cfg.server_port);
    LOG_INFO("Host            : {}", cfg.server_host);
    LOG_INFO("Threads         : {}", cfg.server_threads);
    LOG_INFO("CORS            : {}", cfg.server_cors ? "enabled" : "disabled");
    LOG_INFO("Max body        : {}MB", cfg.server_max_body_mb);
    LOG_INFO("SEC user-agent  : {}", cfg.sec_user_agent);
    LOG_INFO("SEC rate limit  : {}ms", cfg.sec_rate_limit_ms);
    LOG_INFO("SEC timeout     : {}s", cfg.sec_timeout_seconds);
    LOG_INFO("Default years   : {}", cfg.default_years);
    LOG_INFO("Max years       : {}", cfg.max_years);
    LOG_INFO("GPU enabled     : {}", cfg.gpu_enabled ? "yes (reserved)" : "no");
    LOG_INFO("K8s enabled     : {}", cfg.k8s_enabled ? "yes (reserved)" : "no");
    {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "Weights beneish=%.2f altman=%.2f piotroski=%.2f "
            "fraud_triangle=%.2f benford=%.2f red_flags=%.2f",
            cfg.weights.beneish, cfg.weights.altman, cfg.weights.piotroski,
            cfg.weights.fraud_triangle, cfg.weights.benford, cfg.weights.red_flags);
        LOG_DEBUG("{}", std::string(buf));
    }

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Ensure cache directory exists
    util::create_directory(cfg.cache_dir);

    // Create shared components
    auto cache    = std::make_shared<Cache<std::string>>(cfg.cache_ttl_seconds);
    auto fetcher  = std::make_shared<SECFetcher>(cfg.sec_user_agent);
    fetcher->set_rate_limit_ms(cfg.sec_rate_limit_ms);
    fetcher->set_timeout(cfg.sec_timeout_seconds);
    auto analyzer = std::make_shared<FraudAnalyzer>(cfg.weights);
    analyzer->set_fetcher(fetcher);

    // Create and configure server
    g_server = std::make_unique<HttpServer>();
    g_server->set_port(cfg.server_port);
    g_server->set_static_dir(cfg.static_dir);
    g_server->set_cors_enabled(cfg.server_cors);
    g_server->set_max_body_size(static_cast<size_t>(cfg.server_max_body_mb) * 1024 * 1024);

    setup_routes(*g_server, fetcher, analyzer, cache, cfg);

    if (!g_server->start()) {
        LOG_CRITICAL("Failed to start server on port {}", cfg.server_port);
        return 1;
    }

    LOG_INFO("Server running on http://localhost:{}", cfg.server_port);
    LOG_INFO("Press Ctrl+C to stop");

    while (g_running && g_server->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Server stopped");
    return 0;
}
