#pragma once

#include "synx/types.hpp"
#include "synx/config.hpp"
#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/agent/backup_manager.hpp"
#include "synx/agent/agent_api.hpp"
#include "synx/agent/watchdog.hpp"
#include "synx/agent/named_pipe_server.hpp"
#include <memory>
#include <atomic>

namespace synx::agent {

class AgentService {
public:
    static AgentService& instance();

    Result<void> initialize(const std::string& config_file = "config/synx-agent.json");
    Result<void> run_interactive();
    Result<void> run_as_service();
    void stop();

    bool is_running() const { return running_.load(); }

    std::shared_ptr<InstanceManager> instance_manager() { return instance_mgr_; }
    std::shared_ptr<NetworkManager> network_manager() { return network_mgr_; }
    std::shared_ptr<ConsoleManager> console_manager() { return console_mgr_; }
    std::shared_ptr<BackupManager> backup_manager() { return backup_mgr_; }
    std::shared_ptr<AgentApiServer> api_server() { return api_server_; }
    std::shared_ptr<InstanceWatchdog> watchdog() { return watchdog_; }
    std::shared_ptr<NamedPipeServer> named_pipe_server() { return pipe_server_; }
    const SynxAgentConfig& config() const { return config_; }

    static Result<void> install_windows_service();
    static Result<void> uninstall_windows_service();
    static Result<void> start_windows_service();
    static Result<void> stop_windows_service();
    static Result<std::string> query_windows_service_status();

private:
    AgentService() = default;
    ~AgentService();

    SynxAgentConfig config_;
    std::shared_ptr<NetworkManager> network_mgr_;
    std::shared_ptr<ConsoleManager> console_mgr_;
    std::shared_ptr<InstanceManager> instance_mgr_;
    std::shared_ptr<BackupManager> backup_mgr_;
    std::shared_ptr<AgentApiServer> api_server_;
    std::shared_ptr<InstanceWatchdog> watchdog_;
    std::shared_ptr<NamedPipeServer> pipe_server_;

    std::atomic<bool> running_{false};
};

} // namespace synx::agent

