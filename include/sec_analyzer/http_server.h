/**
 * SEC EDGAR Fraud Analyzer - HTTP Server
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Cross-platform HTTP server implementation.
 */

#ifndef SEC_ANALYZER_HTTP_SERVER_H
#define SEC_ANALYZER_HTTP_SERVER_H

#include <string>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
typedef SOCKET socket_t;
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define CLOSE_SOCKET closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET_VALUE -1
#define CLOSE_SOCKET close
#endif

namespace sec_analyzer {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
    std::string body;
    std::string client_ip;
    
    std::string get_param(const std::string& name, const std::string& default_val = "") const {
        auto it = params.find(name);
        return (it != params.end()) ? it->second : default_val;
    }
    
    bool has_param(const std::string& name) const {
        return params.find(name) != params.end();
    }
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() = default;
    HttpResponse(int code, const std::string& text) : status_code(code), status_text(text) {}
    
    static HttpResponse ok(const std::string& body, const std::string& content_type = "application/json") {
        HttpResponse res(200, "OK");
        res.body = body;
        res.headers["Content-Type"] = content_type;
        return res;
    }
    
    static HttpResponse error(int code, const std::string& message) {
        HttpResponse res(code, message);
        res.body = "{\"error\":\"" + message + "\"}";
        res.headers["Content-Type"] = "application/json";
        return res;
    }
    
    static HttpResponse not_found() {
        return error(404, "Not Found");
    }
    
    static HttpResponse bad_request(const std::string& message = "Bad Request") {
        return error(400, message);
    }
    
    static HttpResponse internal_error(const std::string& message = "Internal Server Error") {
        return error(500, message);
    }
};

using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    HttpServer();
    ~HttpServer();
    
    // Configuration
    void set_port(int port) { port_ = port; }
    void set_static_dir(const std::string& dir) { static_dir_ = dir; }
    void set_cors_enabled(bool enabled) { cors_enabled_ = enabled; }
    void set_max_body_size(size_t size) { max_body_size_ = size; }
    
    // Route registration
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);
    void route(const std::string& method, const std::string& path, RequestHandler handler);
    
    // Server control
    bool start();
    void stop();
    bool is_running() const { return running_; }
    int get_port() const { return port_; }
    
private:
    int port_ = 8080;
    std::string static_dir_ = "./web";
    bool cors_enabled_ = true;
    size_t max_body_size_ = 10 * 1024 * 1024; // 10 MB
    
    socket_t server_socket_ = INVALID_SOCKET_VALUE;
    std::atomic<bool> running_{false};
    std::vector<std::thread> worker_threads_;
    std::mutex handlers_mutex_;
    
    struct RouteKey {
        std::string method;
        std::string path;
        bool operator==(const RouteKey& other) const {
            return method == other.method && path == other.path;
        }
    };
    
    struct RouteKeyHash {
        size_t operator()(const RouteKey& k) const {
            return std::hash<std::string>()(k.method + ":" + k.path);
        }
    };
    
    std::unordered_map<RouteKey, RequestHandler, RouteKeyHash> handlers_;
    
    void accept_connections();
    void handle_client(socket_t client_socket, const std::string& client_ip);
    HttpRequest parse_request(const std::string& raw);
    std::string serialize_response(const HttpResponse& res);
    HttpResponse serve_static_file(const std::string& path);
    std::string get_mime_type(const std::string& path);
    void add_cors_headers(HttpResponse& res);
    
    // Platform-specific initialization
    bool init_sockets();
    void cleanup_sockets();
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_HTTP_SERVER_H
