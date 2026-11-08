#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <memory>

namespace synx {

struct AgentDaemonConfig {
    std::string node_id{"b0000000-0000-0000-0000-000000000001"};
    std::string node_name{"localhost-minecraft-server"};
    std::string listen_host{"127.0.0.1"};
    uint16_t listen_port{8443};
    std::string auth_token{"synx_agent_dev_token_default_secret_998244353"};
    bool tls_enabled{false};
    LogLevel log_level{LogLevel::Debug};
    std::string log_file{"logs/synx-agent.log"};
    size_t console_buffer_lines{1000};
};

struct StorageConfig {
    std::string instances_dir{"instances"};
    std::string backups_dir{"backups"};
    std::string temp_dir{"temp"};
};

struct RuntimeConfig {
    std::string java_path{"C:\\Program Files\\Microsoft\\jdk-21.0.12.8-hotspot\\bin\\java.exe"};
    std::vector<std::string> default_java_args;
    bool use_windows_job_objects{true};
    bool enforce_memory_limits{true};
    bool enforce_cpu_limits{true};
};

struct NetworkConfig {
    std::string public_ip{"127.0.0.1"};
    uint16_t port_range_start{25565};
    uint16_t port_range_end{25600};
    uint16_t bedrock_port_range_start{19132};
    uint16_t bedrock_port_range_end{19150};
};

struct SynxAgentConfig {
    AgentDaemonConfig agent;
    StorageConfig storage;
    RuntimeConfig runtime;
    NetworkConfig network;

    static Result<SynxAgentConfig> load_from_file(const std::string& filepath);
    static Result<void> save_to_file(const SynxAgentConfig& config, const std::string& filepath);
    static SynxAgentConfig create_default();

    std::string to_json_string(bool pretty = true) const;
};

} // namespace synx

