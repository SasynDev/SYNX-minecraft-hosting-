#include "synx/agent/named_pipe_server.hpp"
#include "synx/logger.hpp"
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

namespace synx::agent {

NamedPipeServer::NamedPipeServer(
    std::shared_ptr<AgentApiServer> api_server,
    const std::wstring& pipe_name
) : api_server_(std::move(api_server)),
    pipe_name_(pipe_name) {}

NamedPipeServer::~NamedPipeServer() {
    stop();
}

Result<void> NamedPipeServer::start() {
    if (is_running_.load()) return Result<void>::Ok();

#ifdef _WIN32
    stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stop_event_) {
        return Result<void>::Err("Failed to create stop event for named pipe");
    }

    is_running_ = true;
    worker_thread_ = std::make_unique<std::thread>(&NamedPipeServer::pipe_loop, this);

    std::string pipe_name_s(pipe_name_.begin(), pipe_name_.end());
    SYNX_LOG_INFO("NamedPipe", "SYNX IPC Named Pipe server listening at " + pipe_name_s);
    return Result<void>::Ok();
#else
    return Result<void>::Err("Named pipes are only supported on Windows.");
#endif
}

void NamedPipeServer::stop() {
    if (!is_running_.load()) return;
    is_running_ = false;

#ifdef _WIN32
    if (stop_event_) {
        SetEvent(stop_event_);
    }
#endif

    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }

#ifdef _WIN32
    if (stop_event_) {
        CloseHandle(stop_event_);
        stop_event_ = nullptr;
    }
#endif
    SYNX_LOG_INFO("NamedPipe", "SYNX IPC Named Pipe server stopped cleanly.");
}

void NamedPipeServer::pipe_loop() {
#ifdef _WIN32
    while (is_running_.load()) {
        HANDLE hPipe = CreateNamedPipeW(
            pipe_name_.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            65536, // Out buffer
            65536, // In buffer
            0,     // Default timeout
            nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            if (!is_running_.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        OVERLAPPED ol{};
        ol.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        BOOL connected = ConnectNamedPipe(hPipe, &ol);
        if (!connected) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                HANDLE events[2] = { stop_event_, ol.hEvent };
                DWORD wait_res = WaitForMultipleObjects(2, events, FALSE, INFINITE);
                if (wait_res == WAIT_OBJECT_0) {
                    // Stop signaled
                    CancelIo(hPipe);
                    CloseHandle(ol.hEvent);
                    CloseHandle(hPipe);
                    break;
                }
            } else if (err != ERROR_PIPE_CONNECTED) {
                CloseHandle(ol.hEvent);
                CloseHandle(hPipe);
                continue;
            }
        }

        CloseHandle(ol.hEvent);

        if (!is_running_.load()) {
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            break;
        }

        // Read client request message
        char buffer[65536];
        DWORD bytes_read = 0;
        BOOL read_ok = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr);

        if (read_ok && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string req_str(buffer, bytes_read);

            HttpRequest req;
            try {
                // Try parsing as JSON RPC format
                json j = json::parse(req_str);
                req.method = j.value("method", "GET");
                req.path = j.value("path", "/api/v1/health");
                if (j.contains("headers") && j["headers"].is_object()) {
                    for (auto& [k, v] : j["headers"].items()) {
                        req.headers[k] = v.get<std::string>();
                    }
                }
                if (j.contains("body")) {
                    req.body = j["body"].is_string() ? j["body"].get<std::string>() : j["body"].dump();
                }
            } catch (...) {
                // Fallback default
                req.method = "GET";
                req.path = "/api/v1/health";
            }

            HttpResponse res;
            if (api_server_) {
                res = api_server_->handle_request(req);
            } else {
                res = HttpResponse::json(500, "{\"error\": \"API server unavailable\"}");
            }

            json response_json = {
                {"status_code", res.status_code},
                {"body", res.body}
            };
            std::string response_str = response_json.dump();

            DWORD bytes_written = 0;
            WriteFile(hPipe, response_str.data(), static_cast<DWORD>(response_str.size()), &bytes_written, nullptr);
            FlushFileBuffers(hPipe);
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
#endif
}

Result<std::string> NamedPipeServer::send_ipc_message(const std::wstring& pipe_name, const std::string& request_json) {
#ifdef _WIN32
    HANDLE hPipe = CreateFileW(
        pipe_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        return Result<std::string>::Err("Failed to connect to named pipe: error " + std::to_string(GetLastError()));
    }

    DWORD dwMode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &dwMode, nullptr, nullptr);

    DWORD bytes_written = 0;
    BOOL write_ok = WriteFile(hPipe, request_json.data(), static_cast<DWORD>(request_json.size()), &bytes_written, nullptr);
    if (!write_ok) {
        CloseHandle(hPipe);
        return Result<std::string>::Err("Failed to write to named pipe: error " + std::to_string(GetLastError()));
    }

    char buffer[65536];
    DWORD bytes_read = 0;
    BOOL read_ok = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr);
    CloseHandle(hPipe);

    if (!read_ok || bytes_read == 0) {
        return Result<std::string>::Err("Failed to read response from named pipe: error " + std::to_string(GetLastError()));
    }

    buffer[bytes_read] = '\0';
    return Result<std::string>::Ok(std::string(buffer, bytes_read));
#else
    return Result<std::string>::Err("Named pipes are not supported on this platform");
#endif
}

} // namespace synx::agent

