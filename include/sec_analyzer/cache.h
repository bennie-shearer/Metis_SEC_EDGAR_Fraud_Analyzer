/**
 * SEC EDGAR Fraud Analyzer - Cache Manager
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Thread-safe data caching with TTL support.
 */

#ifndef SEC_ANALYZER_CACHE_H
#define SEC_ANALYZER_CACHE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <fstream>

namespace sec_analyzer {

template<typename T>
class Cache {
public:
    explicit Cache(int ttl_seconds = 3600) : ttl_seconds_(ttl_seconds) {}
    
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_[key] = CacheEntry{value, std::chrono::steady_clock::now()};
    }
    
    std::optional<T> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) return std::nullopt;
        
        auto age = std::chrono::steady_clock::now() - it->second.timestamp;
        if (std::chrono::duration_cast<std::chrono::seconds>(age).count() > ttl_seconds_) {
            entries_.erase(it);
            return std::nullopt;
        }
        
        return it->second.value;
    }
    
    bool contains(const std::string& key) {
        return get(key).has_value();
    }
    
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.erase(key);
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }
    
    void cleanup_expired() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        for (auto it = entries_.begin(); it != entries_.end(); ) {
            auto age = now - it->second.timestamp;
            if (std::chrono::duration_cast<std::chrono::seconds>(age).count() > ttl_seconds_) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void set_ttl(int seconds) { ttl_seconds_ = seconds; }
    int get_ttl() const { return ttl_seconds_; }

private:
    struct CacheEntry {
        T value;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CacheEntry> entries_;
    int ttl_seconds_;
};

/**
 * File-based cache for persistent storage
 */
class FileCache {
public:
    explicit FileCache(const std::string& cache_dir) : cache_dir_(cache_dir) {
        ensure_directory();
    }
    
    bool write(const std::string& key, const std::string& data);
    std::optional<std::string> read(const std::string& key);
    bool exists(const std::string& key);
    bool remove(const std::string& key);
    void clear();
    
private:
    std::string cache_dir_;
    std::mutex mutex_;
    
    void ensure_directory();
    std::string key_to_path(const std::string& key);
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_CACHE_H
