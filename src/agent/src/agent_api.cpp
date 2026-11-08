#include "synx/agent/agent_api.hpp"
#include "synx/system_info.hpp"
#include "synx/crypto.hpp"
#include "synx/websocket.hpp"
#include "synx/logger.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using json = nlohmann::json;

namespace synx::agent {

std::string HttpResponse::serialize() const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    for (const auto& [k, v] : headers) {
        ss << k << ": " << v << "\r\n";
    }
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << body;
    return ss.str();
}

AgentApiServer::AgentApiServer(
    const SynxAgentConfig& config,
    std::shared_ptr<InstanceManager> instance_mgr,
    std::shared_ptr<NetworkManager> network_mgr,
    std::shared_ptr<ConsoleManager> console_mgr,
    std::shared_ptr<BackupManager> backup_mgr
) : config_(config),
    instance_mgr_(std::move(instance_mgr)),
    network_mgr_(std::move(network_mgr)),
    console_mgr_(std::move(console_mgr)),
    backup_mgr_(std::move(backup_mgr)) {}

AgentApiServer::~AgentApiServer() {
    stop();
}

Result<void> AgentApiServer::start() {
    if (is_running_.load()) {
        return Result<void>::Ok();
    }

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return Result<void>::Err("Failed to initialize WinSock2");
    }

    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        WSACleanup();
        return Result<void>::Err("Failed to create socket: " + std::to_string(WSAGetLastError()));
    }

    // Set SO_REUSEADDR
    BOOL reuse = TRUE;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.agent.listen_port);
    inet_pton(AF_INET, config_.agent.listen_host.c_str(), &addr.sin_addr);

    if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        int err = WSAGetLastError();
        closesocket(listen_socket_);
        WSACleanup();
        return Result<void>::Err("Failed to bind agent API to " + config_.agent.listen_host + ":" +
                                 std::to_string(config_.agent.listen_port) + " (Error: " + std::to_string(err) + ")");
    }

    if (listen(listen_socket_, SOMAXCONN) != 0) {
        int err = WSAGetLastError();
        closesocket(listen_socket_);
        WSACleanup();
        return Result<void>::Err("Failed to listen on socket (Error: " + std::to_string(err) + ")");
    }

    is_running_ = true;
    server_thread_ = std::make_unique<std::thread>(&AgentApiServer::server_loop, this);

    SYNX_LOG_INFO("AgentAPI", "SYNX Host Agent API listening at http://" +
                  config_.agent.listen_host + ":" + std::to_string(config_.agent.listen_port));
    return Result<void>::Ok();
#else
    return Result<void>::Err("Unsupported platform");
#endif
}

void AgentApiServer::stop() {
    if (!is_running_.load()) return;
    is_running_ = false;

#ifdef _WIN32
    if (listen_socket_ != INVALID_SOCKET) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }
#endif

    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
    SYNX_LOG_INFO("AgentAPI", "SYNX Host Agent API stopped cleanly.");
}

void AgentApiServer::server_loop() {
#ifdef _WIN32
    while (is_running_.load()) {
        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listen_socket_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_sock == INVALID_SOCKET) {
            if (!is_running_.load()) break;
            continue;
        }

        // Handle connection in lightweight inline or detached thread
        handle_client_connection(static_cast<uintptr_t>(client_sock));
    }
#endif
}

void AgentApiServer::handle_client_connection(uintptr_t client_socket) {
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(client_socket);
    std::string raw_data;
    char buffer[8192];
    int bytes_received = recv(s, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        raw_data.append(buffer, bytes_received);
    }

    if (!raw_data.empty()) {
        HttpRequest req = parse_request(raw_data);

        // If Content-Length > body received, continue receiving the remaining body
        auto it_cl = req.headers.find("content-length");
        if (it_cl != req.headers.end()) {
            size_t content_len = 0;
            try { content_len = std::stoull(it_cl->second); } catch (...) {}
            while (req.body.size() < content_len) {
                int more = recv(s, buffer, sizeof(buffer) - 1, 0);
                if (more <= 0) break;
                req.body.append(buffer, more);
            }
        }

        // Check for WebSocket Upgrade request
        auto it_up = req.headers.find("upgrade");
        if (it_up != req.headers.end() && it_up->second == "websocket") {
            const std::string ws_prefix = "/api/v1/instances/";
            const std::string ws_suffix = "/ws";
            if (req.path.rfind(ws_prefix, 0) == 0 && req.path.size() > ws_prefix.size() + ws_suffix.size()) {
                std::string inst_id = req.path.substr(ws_prefix.size(), req.path.size() - ws_prefix.size() - ws_suffix.size());
                handle_websocket_connection(client_socket, req, inst_id);
                return;
            }
        }

        HttpResponse res = handle_request(req);
        std::string serialized = res.serialize();
        send(s, serialized.data(), static_cast<int>(serialized.size()), 0);
    }

    closesocket(s);
#endif
}

HttpRequest AgentApiServer::parse_request(const std::string& raw) {
    HttpRequest req;
    auto header_end = raw.find("\r\n\r\n");
    size_t body_start = 0;
    std::string header_part;

    if (header_end != std::string::npos) {
        header_part = raw.substr(0, header_end);
        body_start = header_end + 4;
    } else {
        header_end = raw.find("\n\n");
        if (header_end != std::string::npos) {
            header_part = raw.substr(0, header_end);
            body_start = header_end + 2;
        } else {
            header_part = raw;
            body_start = raw.size();
        }
    }

    if (body_start < raw.size()) {
        req.body = raw.substr(body_start);
    }

    std::istringstream stream(header_part);
    std::string line;

    if (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream line_stream(line);
        line_stream >> req.method >> req.path;

        auto q_pos = req.path.find('?');
        if (q_pos != std::string::npos) {
            req.query = req.path.substr(q_pos + 1);
            req.path = req.path.substr(0, q_pos);
        }
    }

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            while (!val.empty() && val.front() == ' ') val.erase(val.begin());
            while (!val.empty() && val.back() == ' ') val.pop_back();
            std::string key_lower = key;
            std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            req.headers[key_lower] = val;
        }
    }

    return req;
}

void AgentApiServer::handle_websocket_connection(uintptr_t client_socket, const HttpRequest& req, const std::string& instance_id) {
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(client_socket);

    // Verify token from query or header
    std::string token;
    auto it_token = req.headers.find("x-synx-agent-token");
    if (it_token != req.headers.end()) {
        token = it_token->second;
    } else if (!req.query.empty()) {
        auto t_pos = req.query.find("token=");
        if (t_pos != std::string::npos) {
            token = req.query.substr(t_pos + 6);
            auto amp = token.find('&');
            if (amp != std::string::npos) token = token.substr(0, amp);
        }
    }

    if (token.empty() || !Crypto::constant_time_compare(token, config_.agent.auth_token)) {
        HttpResponse res = HttpResponse::json(401, "{\"error\": \"Unauthorized WebSocket connection\"}");
        std::string serialized = res.serialize();
        send(s, serialized.data(), static_cast<int>(serialized.size()), 0);
        closesocket(s);
        return;
    }

    auto it_key = req.headers.find("sec-websocket-key");
    if (it_key == req.headers.end()) {
        closesocket(s);
        return;
    }

    std::string accept_key = WebSocketProtocol::compute_accept_key(it_key->second);
    std::ostringstream handshake;
    handshake << "HTTP/1.1 101 Switching Protocols\r\n"
              << "Upgrade: websocket\r\n"
              << "Connection: Upgrade\r\n"
              << "Sec-WebSocket-Accept: " << accept_key << "\r\n\r\n";

    std::string hs_str = handshake.str();
    send(s, hs_str.data(), static_cast<int>(hs_str.size()), 0);

    // Send recent backlog lines first
    if (console_mgr_) {
        auto recent = console_mgr_->get_lines(instance_id, 100);
        for (const auto& line : recent) {
            auto frame = WebSocketProtocol::encode_text_frame(line);
            send(s, reinterpret_cast<const char*>(frame.data()), static_cast<int>(frame.size()), 0);
        }
    }

    // Set non-blocking socket timeout for recv loop
    DWORD timeout_ms = 100;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));

    std::atomic<bool> ws_active{true};
    std::shared_ptr<std::mutex> send_mutex = std::make_shared<std::mutex>();

    // Register listener for real-time stdout forwarding
    ConsoleListener listener = [s, &ws_active, send_mutex, instance_id](const std::string& id, const std::string& line) {
        if (!ws_active.load() || id != instance_id) return;
        auto frame = WebSocketProtocol::encode_text_frame(line);
        std::lock_guard<std::mutex> lk(*send_mutex);
        send(s, reinterpret_cast<const char*>(frame.data()), static_cast<int>(frame.size()), 0);
    };

    if (console_mgr_) {
        console_mgr_->add_listener(listener);
    }

    std::vector<uint8_t> in_buffer;
    uint8_t chunk[4096];

    while (ws_active.load() && is_running_.load()) {
        int bytes = recv(s, reinterpret_cast<char*>(chunk), sizeof(chunk), 0);
        if (bytes > 0) {
            in_buffer.insert(in_buffer.end(), chunk, chunk + bytes);
            while (!in_buffer.empty()) {
                size_t consumed = 0;
                auto frame_res = WebSocketProtocol::decode_frame(in_buffer.data(), in_buffer.size(), consumed);
                if (frame_res.is_err()) {
                    break; // Wait for more data
                }

                in_buffer.erase(in_buffer.begin(), in_buffer.begin() + consumed);
                const auto& frame = frame_res.value();

                if (frame.opcode == WebSocketOpCode::Close) {
                    ws_active = false;
                    break;
                } else if (frame.opcode == WebSocketOpCode::Ping) {
                    auto pong = WebSocketProtocol::encode_pong_frame(frame.payload);
                    std::lock_guard<std::mutex> lk(*send_mutex);
                    send(s, reinterpret_cast<const char*>(pong.data()), static_cast<int>(pong.size()), 0);
                } else if (frame.opcode == WebSocketOpCode::Text) {
                    // Client dispatched console command to Minecraft process
                    if (instance_mgr_) {
                        instance_mgr_->send_command(instance_id, frame.payload);
                    }
                }
            }
        } else if (bytes == 0) {
            // Client closed connection
            ws_active = false;
            break;
        } else {
            int err = WSAGetLastError();
            if (err != WSAETIMEDOUT && err != WSAEWOULDBLOCK) {
                ws_active = false;
                break;
            }
        }
    }

    ws_active = false;
    closesocket(s);
#endif
}

HttpResponse AgentApiServer::handle_request(const HttpRequest& req) {
    // Handle CORS preflight
    if (req.method == "OPTIONS") {
        HttpResponse res;
        res.status_code = 204;
        res.status_text = "No Content";
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization, X-Synx-Agent-Token";
        return res;
    }

    // Health check endpoint requires no auth
    if (req.method == "GET" && req.path == "/api/v1/health") {
        json j = {
            {"status", "healthy"},
            {"version", "1.0.0"},
            {"brand", "SYNX HOSTING"},
            {"node_id", config_.agent.node_id},
            {"node_name", config_.agent.node_name},
            {"uptime_seconds", SystemInfo::uptime_seconds()}
        };
        return HttpResponse::json(200, j.dump());
    }

    // Static Web Dashboard file serving (Frontend UI)
    if (req.method == "GET" && req.path.rfind("/api/", 0) != 0) {
        std::filesystem::path dist_dir = "web/dist";
        if (!std::filesystem::exists(dist_dir)) {
            dist_dir = "../web/dist";
        }

        std::string subpath = req.path;
        if (subpath == "/" || subpath.empty()) {
            subpath = "/index.html";
        }

        if (subpath.find("..") == std::string::npos) {
            std::filesystem::path target = dist_dir / subpath.substr(1);
            if (!std::filesystem::exists(target) || std::filesystem::is_directory(target)) {
                target = dist_dir / "index.html";
            }

            if (std::filesystem::exists(target) && !std::filesystem::is_directory(target)) {
                std::ifstream f(target, std::ios::binary);
                if (f) {
                    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    std::string ext = target.extension().string();
                    std::string mime = "application/octet-stream";
                    if (ext == ".html") mime = "text/html; charset=utf-8";
                    else if (ext == ".js") mime = "application/javascript; charset=utf-8";
                    else if (ext == ".css") mime = "text/css; charset=utf-8";
                    else if (ext == ".svg") mime = "image/svg+xml";
                    else if (ext == ".json") mime = "application/json; charset=utf-8";
                    else if (ext == ".png") mime = "image/png";
                    else if (ext == ".ico") mime = "image/x-icon";
                    return HttpResponse::file(200, mime, content);
                }
            }
        }
    }

    // Authenticate all other endpoints
    std::string token;
    auto it_token = req.headers.find("x-synx-agent-token");
    if (it_token != req.headers.end()) {
        token = it_token->second;
    } else {
        auto it_auth = req.headers.find("authorization");
        if (it_auth != req.headers.end() && it_auth->second.rfind("Bearer ", 0) == 0) {
            token = it_auth->second.substr(7);
        }
    }

    if (token.empty() || !Crypto::constant_time_compare(token, config_.agent.auth_token)) {
        json j = {{"error", "Unauthorized: Invalid or missing SYNX Agent token"}};
        return HttpResponse::json(401, j.dump());
    }

    // 1. Host Metrics
    if (req.method == "GET" && req.path == "/api/v1/host/metrics") {
        auto metrics = SystemInfo::query_host_metrics();
        if (instance_mgr_) {
            auto instances = instance_mgr_->list_all_instances();
            metrics.total_instances_count = static_cast<uint32_t>(instances.size());
            uint32_t active = 0;
            for (const auto& inst : instances) {
                auto st = instance_mgr_->get_instance_state(inst.id);
                if (st.is_ok() && st.value() == InstanceState::Running) {
                    active++;
                }
            }
            metrics.active_instances_count = active;
        }

        json j = {
            {"cpu_usage_pct", metrics.cpu_usage_pct},
            {"ram_total_mb", metrics.ram_total_mb},
            {"ram_used_mb", metrics.ram_used_mb},
            {"ram_free_mb", metrics.ram_free_mb},
            {"disk_total_gb", metrics.disk_total_gb},
            {"disk_used_gb", metrics.disk_used_gb},
            {"disk_free_gb", metrics.disk_free_gb},
            {"active_instances", metrics.active_instances_count},
            {"total_instances", metrics.total_instances_count},
            {"uptime_seconds", metrics.uptime_seconds}
        };
        return HttpResponse::json(200, j.dump());
    }

    // 2. List Instances
    if (req.method == "GET" && req.path == "/api/v1/instances") {
        if (!instance_mgr_) return HttpResponse::json(500, "{\"error\": \"Instance manager uninitialized\"}");
        auto instances = instance_mgr_->list_all_instances();
        json arr = json::array();
        for (const auto& inst : instances) {
            auto st = instance_mgr_->get_instance_state(inst.id);
            json item = {
                {"id", inst.id},
                {"identifier", inst.identifier},
                {"name", inst.name},
                {"server_type", server_type_to_string(inst.server_type)},
                {"mc_version", inst.mc_version},
                {"port", inst.primary_allocation.port},
                {"cracked_enabled", inst.cracked_enabled},
                {"crossplay_enabled", inst.crossplay_enabled},
                {"bedrock_port", inst.bedrock_port.has_value() ? inst.bedrock_port.value() : 0},
                {"ram_limit_mb", inst.limits.ram_limit_mb},
                {"cpu_limit_pct", inst.limits.cpu_limit_pct},
                {"status", st.is_ok() ? instance_state_to_string(st.value()) : "unknown"}
            };
            arr.push_back(item);
        }
        return HttpResponse::json(200, arr.dump());
    }

    // 3. Create / Register Instance
    if (req.method == "POST" && req.path == "/api/v1/instances") {
        try {
            json body = json::parse(req.body);
            InstanceConfig cfg;
            cfg.id = body.value("id", generate_uuid());
            cfg.identifier = body.value("identifier", generate_short_identifier());
            cfg.name = body.value("name", "New Minecraft Server");
            cfg.server_type = server_type_from_string(body.value("server_type", "paper"));
            cfg.mc_version = body.value("mc_version", "1.21.4");
            cfg.cracked_enabled = body.value("cracked_enabled", true);
            cfg.crossplay_enabled = body.value("crossplay_enabled", false);
            cfg.limits.ram_limit_mb = body.value("ram_limit_mb", 2048);
            cfg.limits.cpu_limit_pct = body.value("cpu_limit_pct", 100);
            cfg.limits.disk_limit_mb = body.value("disk_limit_mb", 20480);
            cfg.java_path = config_.runtime.java_path;

            // Allocate or assign port
            if (body.contains("port")) {
                cfg.primary_allocation.port = body["port"].get<uint16_t>();
            } else if (network_mgr_) {
                auto p_res = network_mgr_->allocate_java_port(cfg.id);
                if (p_res.is_err()) {
                    return HttpResponse::json(400, json({{"error", p_res.error()}}).dump());
                }
                cfg.primary_allocation.port = p_res.value();
            }

            if (cfg.crossplay_enabled && network_mgr_) {
                auto b_res = network_mgr_->allocate_bedrock_port(cfg.id);
                if (b_res.is_ok()) {
                    cfg.bedrock_port = b_res.value();
                }
            }

            auto reg_res = instance_mgr_->register_instance(cfg);
            if (reg_res.is_err()) {
                return HttpResponse::json(400, json({{"error", reg_res.error()}}).dump());
            }

            json resp = {
                {"success", true},
                {"id", cfg.id},
                {"identifier", cfg.identifier},
                {"port", cfg.primary_allocation.port},
                {"bedrock_port", cfg.bedrock_port.has_value() ? cfg.bedrock_port.value() : 0}
            };
            return HttpResponse::json(201, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(400, json({{"error", e.what()}}).dump());
        }
    }

    // Instance specific subroutes: /api/v1/instances/<id>/...
    const std::string inst_prefix = "/api/v1/instances/";
    if (req.path.rfind(inst_prefix, 0) == 0) {
        std::string sub = req.path.substr(inst_prefix.size());
        auto slash = sub.find('/');
        std::string inst_id = (slash == std::string::npos) ? sub : sub.substr(0, slash);
        std::string action = (slash == std::string::npos) ? "" : sub.substr(slash + 1);

        // GET /api/v1/instances/<id>
        if (req.method == "GET" && action.empty()) {
            auto cfg_res = instance_mgr_->get_instance_config(inst_id);
            if (cfg_res.is_err()) return HttpResponse::json(404, json({{"error", cfg_res.error()}}).dump());
            auto metrics_res = instance_mgr_->get_instance_metrics(inst_id);

            const auto& cfg = cfg_res.value();
            json j = {
                {"id", cfg.id},
                {"identifier", cfg.identifier},
                {"name", cfg.name},
                {"server_type", server_type_to_string(cfg.server_type)},
                {"mc_version", cfg.mc_version},
                {"port", cfg.primary_allocation.port},
                {"cracked_enabled", cfg.cracked_enabled},
                {"status", metrics_res.is_ok() ? instance_state_to_string(metrics_res.value().state) : "unknown"},
                {"ram_limit_mb", cfg.limits.ram_limit_mb},
                {"ram_used_mb", metrics_res.is_ok() ? metrics_res.value().ram_used_mb : 0},
                {"cpu_usage_pct", metrics_res.is_ok() ? metrics_res.value().cpu_usage_pct : 0.0},
                {"disk_used_mb", metrics_res.is_ok() ? metrics_res.value().disk_used_mb : 0},
                {"uptime_seconds", metrics_res.is_ok() ? metrics_res.value().uptime_seconds : 0}
            };
            return HttpResponse::json(200, j.dump());
        }

        // POST /api/v1/instances/<id>/power
        if (req.method == "POST" && action == "power") {
            try {
                json body = json::parse(req.body);
                std::string power_action = body.value("action", "");
                Result<void> res = Result<void>::Err("Invalid power action");

                if (power_action == "start") {
                    res = instance_mgr_->start_instance(inst_id);
                } else if (power_action == "stop") {
                    res = instance_mgr_->stop_instance(inst_id, false);
                } else if (power_action == "restart") {
                    res = instance_mgr_->restart_instance(inst_id);
                } else if (power_action == "kill") {
                    res = instance_mgr_->stop_instance(inst_id, true);
                }

                if (res.is_err()) {
                    return HttpResponse::json(400, json({{"error", res.error()}}).dump());
                }
                return HttpResponse::json(200, json({{"success", true}, {"action", power_action}}).dump());
            } catch (const std::exception& e) {
                return HttpResponse::json(400, json({{"error", e.what()}}).dump());
            }
        }

        // POST /api/v1/instances/<id>/command
        if (req.method == "POST" && action == "command") {
            try {
                json body = json::parse(req.body);
                std::string cmd = body.value("command", "");
                auto res = instance_mgr_->send_command(inst_id, cmd);
                if (res.is_err()) {
                    return HttpResponse::json(400, json({{"error", res.error()}}).dump());
                }
                return HttpResponse::json(200, json({{"success", true}}).dump());
            } catch (const std::exception& e) {
                return HttpResponse::json(400, json({{"error", e.what()}}).dump());
            }
        }

        // GET /api/v1/instances/<id>/console
        if (req.method == "GET" && action == "console") {
            if (!console_mgr_) return HttpResponse::json(500, "{\"error\": \"Console manager unavailable\"}");
            auto lines = console_mgr_->get_lines(inst_id, 200);
            return HttpResponse::json(200, json({{"lines", lines}}).dump());
        }

        // POST /api/v1/instances/<id>/cracked
        if (req.method == "POST" && action == "cracked") {
            try {
                json body = json::parse(req.body);
                bool enabled = body.value("enabled", true);
                auto res = instance_mgr_->set_cracked_mode(inst_id, enabled);
                if (res.is_err()) return HttpResponse::json(400, json({{"error", res.error()}}).dump());
                return HttpResponse::json(200, json({{"success", true}, {"cracked_enabled", enabled}}).dump());
            } catch (const std::exception& e) {
                return HttpResponse::json(400, json({{"error", e.what()}}).dump());
            }
        }

        // GET /api/v1/instances/<id>/properties
        if (req.method == "GET" && action == "properties") {
            auto p_res = instance_mgr_->get_server_properties(inst_id);
            if (p_res.is_err()) return HttpResponse::json(400, json({{"error", p_res.error()}}).dump());
            json j = p_res.value().to_map();
            return HttpResponse::json(200, j.dump());
        }

        // POST /api/v1/instances/<id>/properties
        if (req.method == "POST" && action == "properties") {
            try {
                json body = json::parse(req.body);
                std::unordered_map<std::string, std::string> updates;
                for (auto& [k, v] : body.items()) {
                    updates[k] = v.is_string() ? v.get<std::string>() : v.dump();
                }
                auto res = instance_mgr_->update_server_properties(inst_id, updates);
                if (res.is_err()) return HttpResponse::json(400, json({{"error", res.error()}}).dump());
                return HttpResponse::json(200, json({{"success", true}}).dump());
            } catch (const std::exception& e) {
                return HttpResponse::json(400, json({{"error", e.what()}}).dump());
            }
        }

        // POST /api/v1/instances/<id>/backups
        if (req.method == "POST" && action == "backups") {
            if (!backup_mgr_) return HttpResponse::json(500, "{\"error\": \"Backup manager unavailable\"}");
            std::filesystem::path inst_path = std::filesystem::path(config_.storage.instances_dir) / inst_id;
            auto b_res = backup_mgr_->create_backup(inst_id, inst_path);
            if (b_res.is_err()) {
                return HttpResponse::json(500, json({{"error", b_res.error()}}).dump());
            }
            const auto& b = b_res.value();
            json j = {
                {"id", b.id},
                {"filename", b.filename},
                {"size_bytes", b.size_bytes},
                {"sha256", b.sha256_checksum}
            };
            return HttpResponse::json(201, j.dump());
        }
    }

    return HttpResponse::json(404, "{\"error\": \"Endpoint not found\"}");
}

} // namespace synx::agent
