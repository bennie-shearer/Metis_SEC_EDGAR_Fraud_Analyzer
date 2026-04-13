/**
 * SEC EDGAR Fraud Analyzer - HTTP Server Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/version.h>
#include <sec_analyzer/http_server.h>
#include <sec_analyzer/logger.h>
#include <sec_analyzer/util.h>

#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace sec_analyzer {

HttpServer::HttpServer() {
    init_sockets();
}

HttpServer::~HttpServer() {
    stop();
    cleanup_sockets();
}

bool HttpServer::init_sockets() {
#ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
#else
    return true;
#endif
}

void HttpServer::cleanup_sockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::get(const std::string& path, RequestHandler handler) {
    route("GET", path, handler);
}

void HttpServer::post(const std::string& path, RequestHandler handler) {
    route("POST", path, handler);
}

void HttpServer::put(const std::string& path, RequestHandler handler) {
    route("PUT", path, handler);
}

void HttpServer::del(const std::string& path, RequestHandler handler) {
    route("DELETE", path, handler);
}

void HttpServer::route(const std::string& method, const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_[{method, path}] = handler;
}

bool HttpServer::start() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET_VALUE) {
        LOG_ERROR("Failed to create socket");
        return false;
    }
    
    // Allow address reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    
    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind to port {}", port_);
        CLOSE_SOCKET(server_socket_);
        return false;
    }
    
    if (listen(server_socket_, SOMAXCONN) < 0) {
        LOG_ERROR("Failed to listen on socket");
        CLOSE_SOCKET(server_socket_);
        return false;
    }
    
    running_ = true;
    
    // Start accept thread
    worker_threads_.emplace_back(&HttpServer::accept_connections, this);
    
    return true;
}

void HttpServer::stop() {
    running_ = false;
    
    if (server_socket_ != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(server_socket_);
        server_socket_ = INVALID_SOCKET_VALUE;
    }
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
}

void HttpServer::accept_connections() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        socket_t client_socket = accept(server_socket_,
            reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        
        if (client_socket == INVALID_SOCKET_VALUE) {
            if (running_) {
                LOG_WARNING("Accept failed");
            }
            continue;
        }
        
        // Get client IP
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        std::string client_ip = ip_str;
        
        // Handle client in new thread
        std::thread([this, client_socket, client_ip]() {
            handle_client(client_socket, client_ip);
        }).detach();
    }
}

void HttpServer::handle_client(socket_t client_socket, const std::string& client_ip) {
    // Read request
    std::string raw_request;
    char buffer[4096];
    int bytes_read;
    
    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        raw_request += buffer;
        
        // Check if we have complete headers
        if (raw_request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    
    if (raw_request.empty()) {
        CLOSE_SOCKET(client_socket);
        return;
    }
    
    // Parse request
    HttpRequest request = parse_request(raw_request);
    request.client_ip = client_ip;
    
    LOG_DEBUG("{} {} from {}", request.method, request.path, client_ip);
    
    // Handle CORS preflight
    if (cors_enabled_ && request.method == "OPTIONS") {
        HttpResponse response(204, "No Content");
        add_cors_headers(response);
        std::string raw_response = serialize_response(response);
        send(client_socket, raw_response.c_str(), static_cast<int>(raw_response.length()), 0);
        CLOSE_SOCKET(client_socket);
        return;
    }
    
    // Find handler
    HttpResponse response;
    bool found = false;
    
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto it = handlers_.find({request.method, request.path});
        if (it != handlers_.end()) {
            try {
                response = it->second(request);
                found = true;
            } catch (const std::exception& e) {
                LOG_ERROR("Handler error: {}", e.what());
                response = HttpResponse::internal_error(e.what());
                found = true;
            }
        }
    }
    
    // Try static file serving
    if (!found && request.method == "GET") {
        response = serve_static_file(request.path);
        found = true;
    }
    
    if (!found) {
        response = HttpResponse::not_found();
    }
    
    // Add CORS headers
    if (cors_enabled_) {
        add_cors_headers(response);
    }
    
    // Log request + response status
    LOG_DEBUG("{} {} {} -> {}", client_ip, request.method, request.path, response.status_code);

    // Send response
    std::string raw_response = serialize_response(response);
    send(client_socket, raw_response.c_str(), static_cast<int>(raw_response.length()), 0);
    
    CLOSE_SOCKET(client_socket);
}

HttpRequest HttpServer::parse_request(const std::string& raw) {
    HttpRequest request;
    std::istringstream stream(raw);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::istringstream request_line(line);
        std::string version;
        request_line >> request.method >> request.path >> version;
        
        // Parse query string
        size_t query_pos = request.path.find('?');
        if (query_pos != std::string::npos) {
            request.query_string = request.path.substr(query_pos + 1);
            request.path = request.path.substr(0, query_pos);
            
            // Parse query parameters
            std::istringstream params_stream(request.query_string);
            std::string param;
            while (std::getline(params_stream, param, '&')) {
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = util::url_decode(param.substr(0, eq_pos));
                    std::string value = util::url_decode(param.substr(eq_pos + 1));
                    request.params[key] = value;
                }
            }
        }
    }
    
    // Parse headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) break;
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = util::trim(line.substr(0, colon_pos));
            std::string value = util::trim(line.substr(colon_pos + 1));
            request.headers[key] = value;
        }
    }
    
    // Read body if Content-Length present
    auto cl_it = request.headers.find("Content-Length");
    if (cl_it != request.headers.end()) {
        size_t content_length = std::stoul(cl_it->second);
        std::string remaining;
        while (std::getline(stream, line)) {
            remaining += line + "\n";
        }
        request.body = remaining.substr(0, content_length);
    }
    
    return request;
}

std::string HttpServer::serialize_response(const HttpResponse& response) {
    std::ostringstream oss;
    
    oss << "HTTP/1.1 " << response.status_code << " " << response.status_text << "\r\n";
    
    // Add standard headers
    oss << "Server: MetisSECEDGARFraudAnalyzer/" SEC_ANALYZER_VERSION_STRING "\r\n";
    oss << "Connection: close\r\n";
    
    // Add content length
    oss << "Content-Length: " << response.body.length() << "\r\n";
    
    // Add custom headers
    for (const auto& [key, value] : response.headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n";
    oss << response.body;
    
    return oss.str();
}

HttpResponse HttpServer::serve_static_file(const std::string& path) {
    std::string file_path = static_dir_ + (path == "/" ? "/index.html" : path);
    
    // Security: prevent directory traversal
    if (file_path.find("..") != std::string::npos) {
        return HttpResponse::error(403, "Forbidden");
    }
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return HttpResponse::not_found();
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    
    HttpResponse response = HttpResponse::ok(oss.str(), get_mime_type(file_path));
    return response;
}

std::string HttpServer::get_mime_type(const std::string& path) {
    std::string ext = util::get_extension(path);
    
    static const std::map<std::string, std::string> mime_types = {
        {"html", "text/html; charset=utf-8"},
        {"htm", "text/html; charset=utf-8"},
        {"css", "text/css; charset=utf-8"},
        {"js", "application/javascript; charset=utf-8"},
        {"json", "application/json; charset=utf-8"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain; charset=utf-8"},
        {"xml", "application/xml"},
        {"pdf", "application/pdf"},
        {"csv", "text/csv"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
    };
    
    auto it = mime_types.find(ext);
    return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

void HttpServer::add_cors_headers(HttpResponse& response) {
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    response.headers["Access-Control-Max-Age"] = "86400";
}

} // namespace sec_analyzer
