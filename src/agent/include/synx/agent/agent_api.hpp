#pragma once

#include "synx/types.hpp"
#include "synx/config.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/agent/backup_manager.hpp"
#include <string>
#include <memory>
#include <thread>
#include <atomic>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif

namespace synx::agent {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code{200};
    std::string status_text{"OK"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    static HttpResponse json(int code, const std::string& json_str) {
        HttpResponse res;
        res.status_code = code;
        res.status_text = (code == 200) ? "OK" : (code == 201 ? "Created" : (code == 400 ? "Bad Request" : (code == 401 ? "Unauthorized" : (code == 404 ? "Not Found" : "Internal Server Error"))));
        res.headers["Content-Type"] = "application/json; charset=utf-8";
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization, X-Synx-Agent-Token";
        res.body = json_str;
        return res;
    }

    static HttpResponse file(int code, const std::string& content_type, const std::string& body_data) {
        HttpResponse res;
        res.status_code = code;
        res.status_text = (code == 200) ? "OK" : "Not Found";
        res.headers["Content-Type"] = content_type;
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.body = body_data;
        return res;
    }

    std::string serialize() const;
};

class AgentApiServer {
public:
    AgentApiServer(
        const SynxAgentConfig& config,
        std::shared_ptr<InstanceManager> instance_mgr,
        std::shared_ptr<NetworkManager> network_mgr,
        std::shared_ptr<ConsoleManager> console_mgr,
        std::shared_ptr<BackupManager> backup_mgr
    );
    ~AgentApiServer();

    Result<void> start();
    void stop();
    bool is_running() const { return is_running_.load(); }

    // Direct in-memory request handler for fast unit & integration tests
    HttpResponse handle_request(const HttpRequest& req);

private:
    void server_loop();
    void handle_client_connection(uintptr_t client_socket);
    void handle_websocket_connection(uintptr_t client_socket, const HttpRequest& req, const std::string& instance_id);
    static HttpRequest parse_request(const std::string& raw_request);

    SynxAgentConfig config_;
    std::shared_ptr<InstanceManager> instance_mgr_;
    std::shared_ptr<NetworkManager> network_mgr_;
    std::shared_ptr<ConsoleManager> console_mgr_;
    std::shared_ptr<BackupManager> backup_mgr_;

    std::atomic<bool> is_running_{false};
    std::unique_ptr<std::thread> server_thread_;
#ifdef _WIN32
    SOCKET listen_socket_{INVALID_SOCKET};
#endif
};

} // namespace synx::agent

