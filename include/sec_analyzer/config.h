/**
 * Metis SEC EDGAR Fraud Analyzer - Configuration
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * Copyright (c) 2026 Bennie Shearer (Retired)
 *
 * Typed configuration structure loaded from config.pson.
 * All runtime parameters are sourced from this file.
 */

#ifndef SEC_ANALYZER_CONFIG_H
#define SEC_ANALYZER_CONFIG_H

#include "pson.h"
#include "version.h"
#include "types.h"
#include <string>

namespace sec_analyzer {

// Full server configuration (RiskWeights is defined in types.h)
struct AppConfig {
    // Server
    int         server_port           = 8080;
    std::string server_host           = "0.0.0.0";
    int         server_threads        = 4;
    bool        server_cors           = true;
    int         server_max_body_mb    = 10;       // max request body in megabytes

    // Paths
    std::string static_dir            = "client";
    std::string cache_dir             = "cache";
    std::string log_file              = "logs/metis.log";

    // Logging
    std::string log_level             = "info";
    bool        log_console           = true;     // echo log lines to stdout/stderr

    // SEC EDGAR
    // User-Agent format required by SEC: "FirstName LastName email@domain.com"
    // Do NOT use parentheses, app names, or version strings — causes HTTP 403.
    std::string sec_user_agent        = "Bob Doe bob@doe.com";
    int         sec_rate_limit_ms     = 100;
    int         sec_timeout_seconds   = 30;
    int         sec_max_results       = 10;       // max company search results

    // Cache
    int         cache_ttl_seconds     = 3600;

    // Analysis defaults
    int         default_years         = 5;
    int         max_years             = 10;

    // GPU stub (reserved for future C++20 GPU implementation)
    bool        gpu_enabled           = false;
    std::string gpu_device            = "auto";

    // Kubernetes / container stub (reserved)
    bool        k8s_enabled           = false;
    std::string k8s_namespace         = "default";

    // Model weights
    RiskWeights weights;

    // Load all values from a Pson object
    void load(const Pson& p) {
        server_port         = p.get_int   ("server.port",              server_port);
        server_host         = p.get_string("server.host",              server_host);
        server_threads      = p.get_int   ("server.threads",           server_threads);
        server_cors         = p.get_bool  ("server.cors",              server_cors);
        server_max_body_mb  = p.get_int   ("server.max_body_mb",       server_max_body_mb);

        static_dir          = p.get_string("paths.static_dir",         static_dir);
        cache_dir           = p.get_string("paths.cache_dir",          cache_dir);
        log_file            = p.get_string("paths.log_file",           log_file);

        log_level           = p.get_string("logging.level",            log_level);
        log_console         = p.get_bool  ("logging.console",          log_console);

        sec_user_agent      = p.get_string("sec.user_agent",           sec_user_agent);
        sec_rate_limit_ms   = p.get_int   ("sec.rate_limit_ms",        sec_rate_limit_ms);
        sec_timeout_seconds = p.get_int   ("sec.timeout_seconds",      sec_timeout_seconds);
        sec_max_results     = p.get_int   ("sec.max_results",          sec_max_results);

        cache_ttl_seconds   = p.get_int   ("cache.ttl_seconds",        cache_ttl_seconds);

        default_years       = p.get_int   ("analysis.default_years",   default_years);
        max_years           = p.get_int   ("analysis.max_years",       max_years);

        gpu_enabled         = p.get_bool  ("gpu.enabled",              gpu_enabled);
        gpu_device          = p.get_string("gpu.device",               gpu_device);

        k8s_enabled         = p.get_bool  ("k8s.enabled",              k8s_enabled);
        k8s_namespace       = p.get_string("k8s.namespace",            k8s_namespace);

        // Model weights
        weights.beneish        = p.get_double("weights.beneish",        weights.beneish);
        weights.altman         = p.get_double("weights.altman",         weights.altman);
        weights.piotroski      = p.get_double("weights.piotroski",      weights.piotroski);
        weights.fraud_triangle = p.get_double("weights.fraud_triangle", weights.fraud_triangle);
        weights.benford        = p.get_double("weights.benford",        weights.benford);
        weights.red_flags      = p.get_double("weights.red_flags",      weights.red_flags);
        weights.normalize();
    }

    // Convenience: load from file path
    bool load_file(const std::string& path) {
        Pson p;
        if (!p.load(path)) return false;
        load(p);
        return true;
    }
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_CONFIG_H
