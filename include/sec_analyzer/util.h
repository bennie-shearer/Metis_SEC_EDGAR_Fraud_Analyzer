/**
 * SEC EDGAR Fraud Analyzer - Utility Functions
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Cross-platform utility functions.
 */

#ifndef SEC_ANALYZER_UTIL_H
#define SEC_ANALYZER_UTIL_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR "\\"
#else
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEPARATOR "/"
#endif

namespace sec_analyzer {
namespace util {

/**
 * String utilities
 */
inline std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

inline std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

inline std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

inline std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += delimiter;
        result += parts[i];
    }
    return result;
}

inline bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string replace_all(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

/**
 * URL encoding
 */
inline std::string url_encode(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return oss.str();
}

inline std::string url_decode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int hex = 0;
            std::istringstream iss(str.substr(i + 1, 2));
            iss >> std::hex >> hex;
            result += static_cast<char>(hex);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

/**
 * Date/Time utilities
 */
inline std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    
#ifdef _WIN32
    struct tm tm_buf;
    gmtime_s(&tm_buf, &time);
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
#else
    struct tm tm_buf;
    gmtime_r(&time, &tm_buf);
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
#endif
    
    return oss.str();
}

inline std::string format_date(const std::string& date_str) {
    // Convert various date formats to YYYY-MM-DD
    if (date_str.empty()) return "";
    
    // Remove any non-alphanumeric characters except dashes
    std::string clean;
    for (char c : date_str) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
            clean += c;
        }
    }
    
    return clean;
}

/**
 * Number utilities
 */
inline double safe_divide(double numerator, double denominator, double default_val = 0.0) {
    if (std::abs(denominator) < 1e-10) return default_val;
    return numerator / denominator;
}

inline double clamp(double value, double min_val, double max_val) {
    return std::max(min_val, std::min(max_val, value));
}

inline std::string format_currency(double value) {
    std::ostringstream oss;
    if (std::abs(value) >= 1e12) {
        oss << "$" << std::fixed << std::setprecision(2) << (value / 1e12) << "T";
    } else if (std::abs(value) >= 1e9) {
        oss << "$" << std::fixed << std::setprecision(2) << (value / 1e9) << "B";
    } else if (std::abs(value) >= 1e6) {
        oss << "$" << std::fixed << std::setprecision(2) << (value / 1e6) << "M";
    } else if (std::abs(value) >= 1e3) {
        oss << "$" << std::fixed << std::setprecision(2) << (value / 1e3) << "K";
    } else {
        oss << "$" << std::fixed << std::setprecision(2) << value;
    }
    return oss.str();
}

inline std::string format_percentage(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (value * 100) << "%";
    return oss.str();
}

/**
 * File system utilities
 */
inline bool file_exists(const std::string& path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
}

inline bool directory_exists(const std::string& path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

inline bool create_directory(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

inline std::string get_extension(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "";
    return to_lower(path.substr(dot + 1));
}

inline std::string get_filename(const std::string& path) {
    size_t sep = path.find_last_of("/\\");
    if (sep == std::string::npos) return path;
    return path.substr(sep + 1);
}

/**
 * CIK utilities
 */
inline std::string normalize_cik(const std::string& cik) {
    // Pad CIK to 10 digits with leading zeros
    std::string clean;
    for (char c : cik) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            clean += c;
        }
    }
    while (clean.length() < 10) {
        clean = "0" + clean;
    }
    return clean;
}

inline std::string cik_to_path(const std::string& cik) {
    // Convert CIK to SEC EDGAR path format (without leading zeros)
    std::string clean = normalize_cik(cik);
    size_t start = clean.find_first_not_of('0');
    if (start == std::string::npos) return "0";
    return clean.substr(start);
}

/**
 * JSON escape
 */
inline std::string json_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                    result += oss.str();
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

/**
 * HTML escape
 */
inline std::string html_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c; break;
        }
    }
    return result;
}

} // namespace util
} // namespace sec_analyzer

#endif // SEC_ANALYZER_UTIL_H
