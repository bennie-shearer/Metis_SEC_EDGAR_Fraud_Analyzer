/**
 * Metis SEC EDGAR Fraud Analyzer - PSON Configuration Parser
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * Copyright (c) 2026 Bennie Shearer (Retired)
 *
 * Parses a simple key=value configuration format (PSON):
 *   # Comment lines begin with '#'
 *   key=value
 *   section.key=value
 *   key="value with spaces"
 *   key=123
 *   key=true
 */

#ifndef SEC_ANALYZER_PSON_H
#define SEC_ANALYZER_PSON_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm>

namespace sec_analyzer {

class Pson {
public:
    Pson() = default;

    // Load from file
    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::string line;
        while (std::getline(file, line)) {
            parse_line(line);
        }
        return true;
    }

    // Load from string
    void parse(const std::string& content) {
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            parse_line(line);
        }
    }

    // Accessors
    bool has(const std::string& key) const {
        return data_.count(key) > 0;
    }

    std::string get_string(const std::string& key, const std::string& default_val = "") const {
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : default_val;
    }

    int get_int(const std::string& key, int default_val = 0) const {
        auto it = data_.find(key);
        if (it == data_.end()) return default_val;
        try { return std::stoi(it->second); } catch (...) { return default_val; }
    }

    double get_double(const std::string& key, double default_val = 0.0) const {
        auto it = data_.find(key);
        if (it == data_.end()) return default_val;
        try { return std::stod(it->second); } catch (...) { return default_val; }
    }

    bool get_bool(const std::string& key, bool default_val = false) const {
        auto it = data_.find(key);
        if (it == data_.end()) return default_val;
        std::string v = it->second;
        std::transform(v.begin(), v.end(), v.begin(), ::tolower);
        return (v == "true" || v == "1" || v == "yes");
    }

    // Return all keys with a given prefix (prefix stripped from result keys)
    std::map<std::string, std::string> get_section(const std::string& prefix) const {
        std::map<std::string, std::string> result;
        std::string pfx = prefix + ".";
        for (const auto& [k, v] : data_) {
            if (k.substr(0, pfx.size()) == pfx) {
                result[k.substr(pfx.size())] = v;
            }
        }
        return result;
    }

    const std::map<std::string, std::string>& all() const { return data_; }

private:
    std::map<std::string, std::string> data_;

    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    void parse_line(const std::string& raw_line) {
        std::string line = trim(raw_line);
        if (line.empty() || line[0] == '#') return;

        size_t eq = line.find('=');
        if (eq == std::string::npos) return;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key.empty()) return;

        // Strip inline comments
        size_t comment_pos = val.find(" #");
        if (comment_pos != std::string::npos) {
            val = trim(val.substr(0, comment_pos));
        }

        // Strip surrounding quotes
        if (val.size() >= 2 &&
            ((val.front() == '"' && val.back() == '"') ||
             (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.size() - 2);
        }

        data_[key] = val;
    }
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_PSON_H
