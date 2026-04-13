/**
 * SEC EDGAR Fraud Analyzer - Cache Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/cache.h>
#include <sec_analyzer/util.h>

#include <fstream>
#include <algorithm>

namespace sec_analyzer {

void FileCache::ensure_directory() {
    util::create_directory(cache_dir_);
}

std::string FileCache::key_to_path(const std::string& key) {
    // Sanitize key for filesystem
    std::string safe_key = key;
    std::replace(safe_key.begin(), safe_key.end(), ':', '_');
    std::replace(safe_key.begin(), safe_key.end(), '/', '_');
    std::replace(safe_key.begin(), safe_key.end(), '\\', '_');
    return cache_dir_ + PATH_SEPARATOR + safe_key + ".cache";
}

bool FileCache::write(const std::string& key, const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string path = key_to_path(key);
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file << data;
    return file.good();
}

std::optional<std::string> FileCache::read(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string path = key_to_path(key);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

bool FileCache::exists(const std::string& key) {
    std::string path = key_to_path(key);
    return util::file_exists(path);
}

bool FileCache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string path = key_to_path(key);
    return std::remove(path.c_str()) == 0;
}

void FileCache::clear() {
    // Note: Would need to iterate directory and remove files
    // Simplified implementation
}

} // namespace sec_analyzer
