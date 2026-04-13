/**
 * SEC EDGAR Fraud Analyzer - JSON Utilities
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Minimal JSON parser and builder (no external dependencies).
 */

#ifndef SEC_ANALYZER_JSON_H
#define SEC_ANALYZER_JSON_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace sec_analyzer {

class JsonValue;

using JsonNull = std::nullptr_t;
using JsonBool = bool;
using JsonNumber = double;
using JsonString = std::string;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

class JsonValue {
public:
    using Value = std::variant<JsonNull, JsonBool, JsonNumber, JsonString, JsonArray, JsonObject>;
    
    JsonValue() : value_(nullptr) {}
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int n) : value_(static_cast<double>(n)) {}
    JsonValue(double n) : value_(n) {}
    JsonValue(const char* s) : value_(std::string(s)) {}
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    JsonValue(const JsonArray& arr) : value_(arr) {}
    JsonValue(JsonArray&& arr) : value_(std::move(arr)) {}
    JsonValue(const JsonObject& obj) : value_(obj) {}
    JsonValue(JsonObject&& obj) : value_(std::move(obj)) {}
    
    bool is_null() const { return std::holds_alternative<JsonNull>(value_); }
    bool is_bool() const { return std::holds_alternative<JsonBool>(value_); }
    bool is_number() const { return std::holds_alternative<JsonNumber>(value_); }
    bool is_string() const { return std::holds_alternative<JsonString>(value_); }
    bool is_array() const { return std::holds_alternative<JsonArray>(value_); }
    bool is_object() const { return std::holds_alternative<JsonObject>(value_); }
    
    bool as_bool() const { return std::get<JsonBool>(value_); }
    double as_number() const { return std::get<JsonNumber>(value_); }
    int as_int() const { return static_cast<int>(std::get<JsonNumber>(value_)); }
    const std::string& as_string() const { return std::get<JsonString>(value_); }
    const JsonArray& as_array() const { return std::get<JsonArray>(value_); }
    const JsonObject& as_object() const { return std::get<JsonObject>(value_); }
    
    JsonArray& as_array() { return std::get<JsonArray>(value_); }
    JsonObject& as_object() { return std::get<JsonObject>(value_); }
    
    // Array access
    JsonValue& operator[](size_t index) {
        return std::get<JsonArray>(value_)[index];
    }
    
    const JsonValue& operator[](size_t index) const {
        return std::get<JsonArray>(value_)[index];
    }
    
    // Object access
    JsonValue& operator[](const std::string& key) {
        return std::get<JsonObject>(value_)[key];
    }
    
    const JsonValue& at(const std::string& key) const {
        return std::get<JsonObject>(value_).at(key);
    }
    
    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        return std::get<JsonObject>(value_).count(key) > 0;
    }
    
    size_t size() const {
        if (is_array()) return std::get<JsonArray>(value_).size();
        if (is_object()) return std::get<JsonObject>(value_).size();
        return 0;
    }
    
    // Serialize to string
    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dump_impl(oss, indent, 0);
        return oss.str();
    }

private:
    Value value_;
    
    void dump_impl(std::ostringstream& oss, int indent, int level) const {
        std::string ind = (indent >= 0) ? std::string(level * indent, ' ') : "";
        std::string ind_inner = (indent >= 0) ? std::string((level + 1) * indent, ' ') : "";
        std::string newline = (indent >= 0) ? "\n" : "";
        std::string sep = (indent >= 0) ? " " : "";
        
        if (is_null()) {
            oss << "null";
        } else if (is_bool()) {
            oss << (as_bool() ? "true" : "false");
        } else if (is_number()) {
            double n = as_number();
            if (std::isnan(n) || std::isinf(n)) {
                oss << "null";
            } else if (n == std::floor(n) && std::abs(n) < 1e15) {
                oss << static_cast<long long>(n);
            } else {
                oss << std::setprecision(15) << n;
            }
        } else if (is_string()) {
            oss << '"' << escape_string(as_string()) << '"';
        } else if (is_array()) {
            const auto& arr = as_array();
            if (arr.empty()) {
                oss << "[]";
            } else {
                oss << "[" << newline;
                for (size_t i = 0; i < arr.size(); ++i) {
                    oss << ind_inner;
                    arr[i].dump_impl(oss, indent, level + 1);
                    if (i + 1 < arr.size()) oss << ",";
                    oss << newline;
                }
                oss << ind << "]";
            }
        } else if (is_object()) {
            const auto& obj = as_object();
            if (obj.empty()) {
                oss << "{}";
            } else {
                oss << "{" << newline;
                size_t i = 0;
                for (const auto& [k, v] : obj) {
                    oss << ind_inner << '"' << escape_string(k) << '"' << ":" << sep;
                    v.dump_impl(oss, indent, level + 1);
                    if (i + 1 < obj.size()) oss << ",";
                    oss << newline;
                    ++i;
                }
                oss << ind << "}";
            }
        }
    }
    
    static std::string escape_string(const std::string& s) {
        std::string result;
        for (char c : s) {
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
};

// JSON Parser
class JsonParser {
public:
    static JsonValue parse(const std::string& json) {
        JsonParser parser(json);
        return parser.parse_value();
    }

private:
    explicit JsonParser(const std::string& json) : json_(json), pos_(0) {}
    
    JsonValue parse_value() {
        skip_whitespace();
        if (pos_ >= json_.size()) throw std::runtime_error("Unexpected end of JSON");
        
        char c = json_[pos_];
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        
        throw std::runtime_error("Invalid JSON value");
    }
    
    JsonValue parse_null() {
        expect("null");
        return JsonValue(nullptr);
    }
    
    JsonValue parse_bool() {
        if (json_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return JsonValue(true);
        }
        expect("false");
        return JsonValue(false);
    }
    
    JsonValue parse_number() {
        size_t start = pos_;
        if (json_[pos_] == '-') ++pos_;
        
        if (json_[pos_] == '0') {
            ++pos_;
        } else {
            while (pos_ < json_.size() && std::isdigit(static_cast<unsigned char>(json_[pos_]))) ++pos_;
        }
        
        if (pos_ < json_.size() && json_[pos_] == '.') {
            ++pos_;
            while (pos_ < json_.size() && std::isdigit(static_cast<unsigned char>(json_[pos_]))) ++pos_;
        }
        
        if (pos_ < json_.size() && (json_[pos_] == 'e' || json_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < json_.size() && (json_[pos_] == '+' || json_[pos_] == '-')) ++pos_;
            while (pos_ < json_.size() && std::isdigit(static_cast<unsigned char>(json_[pos_]))) ++pos_;
        }
        
        return JsonValue(std::stod(json_.substr(start, pos_ - start)));
    }
    
    JsonValue parse_string() {
        ++pos_; // Skip opening quote
        std::string result;
        
        while (pos_ < json_.size() && json_[pos_] != '"') {
            if (json_[pos_] == '\\') {
                ++pos_;
                if (pos_ >= json_.size()) throw std::runtime_error("Unterminated string");
                switch (json_[pos_]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        if (pos_ + 4 >= json_.size()) throw std::runtime_error("Invalid unicode escape");
                        int cp = std::stoi(json_.substr(pos_ + 1, 4), nullptr, 16);
                        if (cp < 0x80) {
                            result += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            result += static_cast<char>(0xC0 | (cp >> 6));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (cp >> 12));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        pos_ += 4;
                        break;
                    }
                    default: throw std::runtime_error("Invalid escape sequence");
                }
            } else {
                result += json_[pos_];
            }
            ++pos_;
        }
        
        if (pos_ >= json_.size()) throw std::runtime_error("Unterminated string");
        ++pos_; // Skip closing quote
        return JsonValue(std::move(result));
    }
    
    JsonValue parse_array() {
        ++pos_; // Skip '['
        skip_whitespace();
        
        JsonArray arr;
        if (pos_ < json_.size() && json_[pos_] == ']') {
            ++pos_;
            return JsonValue(std::move(arr));
        }
        
        while (true) {
            arr.push_back(parse_value());
            skip_whitespace();
            
            if (pos_ >= json_.size()) throw std::runtime_error("Unterminated array");
            if (json_[pos_] == ']') {
                ++pos_;
                return JsonValue(std::move(arr));
            }
            if (json_[pos_] != ',') throw std::runtime_error("Expected ',' in array");
            ++pos_;
            skip_whitespace();
        }
    }
    
    JsonValue parse_object() {
        ++pos_; // Skip '{'
        skip_whitespace();
        
        JsonObject obj;
        if (pos_ < json_.size() && json_[pos_] == '}') {
            ++pos_;
            return JsonValue(std::move(obj));
        }
        
        while (true) {
            if (json_[pos_] != '"') throw std::runtime_error("Expected string key in object");
            std::string key = parse_string().as_string();
            
            skip_whitespace();
            if (pos_ >= json_.size() || json_[pos_] != ':') throw std::runtime_error("Expected ':' in object");
            ++pos_;
            
            obj[key] = parse_value();
            skip_whitespace();
            
            if (pos_ >= json_.size()) throw std::runtime_error("Unterminated object");
            if (json_[pos_] == '}') {
                ++pos_;
                return JsonValue(std::move(obj));
            }
            if (json_[pos_] != ',') throw std::runtime_error("Expected ',' in object");
            ++pos_;
            skip_whitespace();
        }
    }
    
    void skip_whitespace() {
        while (pos_ < json_.size() && std::isspace(static_cast<unsigned char>(json_[pos_]))) ++pos_;
    }
    
    void expect(const std::string& s) {
        if (json_.substr(pos_, s.size()) != s) {
            throw std::runtime_error("Expected '" + s + "'");
        }
        pos_ += s.size();
    }
    
    const std::string& json_;
    size_t pos_;
};

// Convenience function
inline JsonValue parse_json(const std::string& json) {
    return JsonParser::parse(json);
}

} // namespace sec_analyzer

#endif // SEC_ANALYZER_JSON_H
