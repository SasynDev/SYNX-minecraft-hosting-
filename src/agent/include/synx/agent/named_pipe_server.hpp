#pragma once

#include "synx/types.hpp"
#include "synx/agent/agent_api.hpp"
#include <string>
#include <memory>
#include <thread>
#include <atomic>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace synx::agent {

class NamedPipeServer {
public:
    explicit NamedPipeServer(
        std::shared_ptr<AgentApiServer> api_server,
        const std::wstring& pipe_name = L"\\\\.\\pipe\\synx_agent_ipc"
    );
    ~NamedPipeServer();

    Result<void> start();
    void stop();
    bool is_running() const { return is_running_.load(); }

    const std::wstring& pipe_name() const { return pipe_name_; }

    // Direct synchronous IPC client call for desktop admin app or tests
    static Result<std::string> send_ipc_message(const std::wstring& pipe_name, const std::string& request_json);

private:
    void pipe_loop();

    std::shared_ptr<AgentApiServer> api_server_;
    std::wstring pipe_name_;
    std::atomic<bool> is_running_{false};
    std::unique_ptr<std::thread> worker_thread_;

#ifdef _WIN32
    HANDLE stop_event_{nullptr};
#endif
};

} // namespace synx::agent

