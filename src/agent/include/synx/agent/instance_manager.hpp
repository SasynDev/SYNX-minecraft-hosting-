#pragma once

#include "synx/types.hpp"
#include "synx/process.hpp"
#include "synx/filesystem.hpp"
#include "synx/server_properties.hpp"
#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/agent/server_installer.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <filesystem>

namespace synx::agent {

struct ManagedInstance {
    InstanceConfig config;
    InstanceState state{InstanceState::Stopped};
    std::unique_ptr<ManagedProcess> process;
    std::unique_ptr<SandboxedFilesystem> fs;
    uint32_t crash_count{0};
};

class InstanceManager {
public:
    InstanceManager(
        std::filesystem::path instances_root,
        std::shared_ptr<NetworkManager> network_mgr,
        std::shared_ptr<ConsoleManager> console_mgr
    );
    ~InstanceManager();

    Result<void> register_instance(const InstanceConfig& config);
    Result<void> start_instance(const std::string& instance_id);
    Result<void> stop_instance(const std::string& instance_id, bool force = false);
    Result<void> restart_instance(const std::string& instance_id);
    Result<void> send_command(const std::string& instance_id, const std::string& command);

    Result<InstanceState> get_instance_state(const std::string& instance_id);
    Result<InstanceMetrics> get_instance_metrics(const std::string& instance_id);
    Result<InstanceConfig> get_instance_config(const std::string& instance_id);
    std::vector<InstanceConfig> list_all_instances() const;

    Result<void> delete_instance(const std::string& instance_id, bool delete_files = false);

    // Server properties and cracked mode management
    Result<void> set_cracked_mode(const std::string& instance_id, bool cracked_enabled);
    Result<bool> is_cracked_mode(const std::string& instance_id);
    Result<ServerProperties> get_server_properties(const std::string& instance_id);
    Result<void> update_server_properties(const std::string& instance_id, const std::unordered_map<std::string, std::string>& updates);

    // Stop all instances cleanly on shutdown
    void shutdown_all();

private:
    std::filesystem::path instances_root_;
    std::shared_ptr<NetworkManager> network_mgr_;
    std::shared_ptr<ConsoleManager> console_mgr_;
    std::shared_ptr<ServerInstaller> installer_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<ManagedInstance>> instances_;
};

} // namespace synx::agent

