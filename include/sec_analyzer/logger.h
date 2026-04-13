/**
 * SEC EDGAR Fraud Analyzer - Logger
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Thread-safe logging utility.
 */

#ifndef SEC_ANALYZER_LOGGER_H
#define SEC_ANALYZER_LOGGER_H

#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <cctype>

namespace sec_analyzer {

// Undefine Windows ERROR macro to avoid conflict with LogLevel::ERROR
#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    void set_level(LogLevel level) {
        level_ = level;
    }
    
    // Parse log level from string (debug, info, warning, error, critical)
    static LogLevel parse_level(const std::string& level_str) {
        std::string lower = level_str;
        for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        
        if (lower == "debug" || lower == "d") return LogLevel::DEBUG;
        if (lower == "info" || lower == "i") return LogLevel::INFO;
        if (lower == "warning" || lower == "warn" || lower == "w") return LogLevel::WARNING;
        if (lower == "error" || lower == "err" || lower == "e") return LogLevel::ERROR;
        if (lower == "critical" || lower == "crit" || lower == "c") return LogLevel::CRITICAL;
        return LogLevel::INFO; // Default
    }
    
    void set_level_from_string(const std::string& level_str) {
        level_ = parse_level(level_str);
    }
    
    LogLevel get_level() const {
        return level_;
    }
    
    void set_file(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_.open(path, std::ios::app);
    }
    
    void set_console_output(bool enabled) {
        console_output_ = enabled;
    }
    
    template<typename... Args>
    void debug(const std::string& msg, Args&&... args) {
        log(LogLevel::DEBUG, msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& msg, Args&&... args) {
        log(LogLevel::INFO, msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(const std::string& msg, Args&&... args) {
        log(LogLevel::WARNING, msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& msg, Args&&... args) {
        log(LogLevel::ERROR, msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(const std::string& msg, Args&&... args) {
        log(LogLevel::CRITICAL, msg, std::forward<Args>(args)...);
    }

private:
    Logger() : level_(LogLevel::INFO), console_output_(true) {}
    ~Logger() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    template<typename... Args>
    void log(LogLevel level, const std::string& msg, Args&&... args) {
        if (level < level_) return;
        
        std::ostringstream oss;
        oss << get_timestamp() << " [" << level_to_string(level) << "] " << format(msg, std::forward<Args>(args)...);
        
        std::string line = oss.str();
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (console_output_) {
            std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
            out << line << std::endl;
        }
        
        if (file_.is_open()) {
            file_ << line << std::endl;
            file_.flush();
        }
    }
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
#ifdef _WIN32
        struct tm tm_buf;
        localtime_s(&tm_buf, &time);
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
#else
        struct tm tm_buf;
        localtime_r(&time, &tm_buf);
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
#endif
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
    
    static std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO ";
            case LogLevel::WARNING: return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRIT ";
            default: return "?????";
        }
    }
    
    // Simple format function for variadic arguments
    std::string format(const std::string& msg) {
        return msg;
    }
    
    template<typename T, typename... Args>
    std::string format(const std::string& msg, T&& first, Args&&... rest) {
        std::ostringstream oss;
        size_t pos = msg.find("{}");
        if (pos != std::string::npos) {
            oss << msg.substr(0, pos) << first;
            return oss.str() + format(msg.substr(pos + 2), std::forward<Args>(rest)...);
        }
        return msg;
    }
    
    LogLevel level_;
    bool console_output_;
    std::mutex mutex_;
    std::ofstream file_;
};

// Convenience macros
#define LOG_DEBUG(msg, ...) sec_analyzer::Logger::instance().debug(msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) sec_analyzer::Logger::instance().info(msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) sec_analyzer::Logger::instance().warning(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) sec_analyzer::Logger::instance().error(msg, ##__VA_ARGS__)
#define LOG_CRITICAL(msg, ...) sec_analyzer::Logger::instance().critical(msg, ##__VA_ARGS__)

} // namespace sec_analyzer

#endif // SEC_ANALYZER_LOGGER_H
