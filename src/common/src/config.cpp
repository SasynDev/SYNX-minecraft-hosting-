#include "synx/config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>

using json = nlohmann::json;

namespace synx {

static std::string get_env_var(const char* name, const std::string& default_val = "") {
#ifdef _WIN32
    char buffer[1024];
    size_t required_size = 0;
    if (getenv_s(&required_size, buffer, sizeof(buffer), name) == 0 && required_size > 0) {
        return std::string(buffer);
    }
    return default_val;
#else
    const char* val = std::getenv(name);
    return val ? std::string(val) : default_val;
#endif
}

SynxAgentConfig SynxAgentConfig::create_default() {
    SynxAgentConfig cfg;
    cfg.runtime.default_java_args = {
        "-XX:+UseG1GC",
        "-XX:+ParallelRefProcEnabled",
        "-XX:MaxGCPauseMillis=200",
        "-XX:+UnlockExperimentalVMOptions",
        "-XX:+DisableExplicitGC",
        "-XX:+AlwaysPreTouch"
    };
    return cfg;
}

Result<SynxAgentConfig> SynxAgentConfig::load_from_file(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) {
        return Result<SynxAgentConfig>::Err("Could not open configuration file: " + filepath);
    }

    try {
        json j;
        f >> j;

        SynxAgentConfig cfg = create_default();

        if (j.contains("agent") && j["agent"].is_object()) {
            const auto& a = j["agent"];
            if (a.contains("node_id")) cfg.agent.node_id = a["node_id"].get<std::string>();
            if (a.contains("node_name")) cfg.agent.node_name = a["node_name"].get<std::string>();
            if (a.contains("listen_host")) cfg.agent.listen_host = a["listen_host"].get<std::string>();
            if (a.contains("listen_port")) cfg.agent.listen_port = a["listen_port"].get<uint16_t>();
            if (a.contains("auth_token")) cfg.agent.auth_token = a["auth_token"].get<std::string>();
            if (a.contains("tls_enabled")) cfg.agent.tls_enabled = a["tls_enabled"].get<bool>();
            if (a.contains("log_level")) cfg.agent.log_level = log_level_from_string(a["log_level"].get<std::string>());
            if (a.contains("log_file")) cfg.agent.log_file = a["log_file"].get<std::string>();
            if (a.contains("console_buffer_lines")) cfg.agent.console_buffer_lines = a["console_buffer_lines"].get<size_t>();
        }

        if (j.contains("storage") && j["storage"].is_object()) {
            const auto& s = j["storage"];
            if (s.contains("instances_dir")) cfg.storage.instances_dir = s["instances_dir"].get<std::string>();
            if (s.contains("backups_dir")) cfg.storage.backups_dir = s["backups_dir"].get<std::string>();
            if (s.contains("temp_dir")) cfg.storage.temp_dir = s["temp_dir"].get<std::string>();
        }

        if (j.contains("runtime") && j["runtime"].is_object()) {
            const auto& r = j["runtime"];
            if (r.contains("java_path")) cfg.runtime.java_path = r["java_path"].get<std::string>();
            if (r.contains("use_windows_job_objects")) cfg.runtime.use_windows_job_objects = r["use_windows_job_objects"].get<bool>();
            if (r.contains("enforce_memory_limits")) cfg.runtime.enforce_memory_limits = r["enforce_memory_limits"].get<bool>();
            if (r.contains("enforce_cpu_limits")) cfg.runtime.enforce_cpu_limits = r["enforce_cpu_limits"].get<bool>();
            if (r.contains("default_java_args") && r["default_java_args"].is_array()) {
                cfg.runtime.default_java_args = r["default_java_args"].get<std::vector<std::string>>();
            }
        }

        if (j.contains("network") && j["network"].is_object()) {
            const auto& n = j["network"];
            if (n.contains("public_ip")) cfg.network.public_ip = n["public_ip"].get<std::string>();
            if (n.contains("port_range_start")) cfg.network.port_range_start = n["port_range_start"].get<uint16_t>();
            if (n.contains("port_range_end")) cfg.network.port_range_end = n["port_range_end"].get<uint16_t>();
            if (n.contains("bedrock_port_range_start")) cfg.network.bedrock_port_range_start = n["bedrock_port_range_start"].get<uint16_t>();
            if (n.contains("bedrock_port_range_end")) cfg.network.bedrock_port_range_end = n["bedrock_port_range_end"].get<uint16_t>();
        }

        // Apply environment variable overrides if present
        std::string env_host = get_env_var("SYNX_AGENT_LISTEN_HOST");
        if (!env_host.empty()) cfg.agent.listen_host = env_host;

        std::string env_port = get_env_var("SYNX_AGENT_LISTEN_PORT");
        if (!env_port.empty()) cfg.agent.listen_port = static_cast<uint16_t>(std::stoi(env_port));

        std::string env_token = get_env_var("SYNX_AGENT_AUTH_TOKEN");
        if (!env_token.empty()) cfg.agent.auth_token = env_token;

        std::string env_java = get_env_var("SYNX_JAVA_PATH");
        if (!env_java.empty()) cfg.runtime.java_path = env_java;

        return Result<SynxAgentConfig>::Ok(cfg);
    } catch (const std::exception& e) {
        return Result<SynxAgentConfig>::Err(std::string("JSON parsing error: ") + e.what());
    }
}

Result<void> SynxAgentConfig::save_to_file(const SynxAgentConfig& config, const std::string& filepath) {
    try {
        std::ofstream f(filepath);
        if (!f.is_open()) {
            return Result<void>::Err("Could not open file for writing: " + filepath);
        }
        f << config.to_json_string(true);
        return Result<void>::Ok();
    } catch (const std::exception& e) {
        return Result<void>::Err(std::string("Failed to save config: ") + e.what());
    }
}

std::string SynxAgentConfig::to_json_string(bool pretty) const {
    json j;
    j["agent"] = {
        {"node_id", agent.node_id},
        {"node_name", agent.node_name},
        {"listen_host", agent.listen_host},
        {"listen_port", agent.listen_port},
        {"auth_token", agent.auth_token},
        {"tls_enabled", agent.tls_enabled},
        {"log_level", log_level_to_string(agent.log_level)},
        {"log_file", agent.log_file},
        {"console_buffer_lines", agent.console_buffer_lines}
    };
    j["storage"] = {
        {"instances_dir", storage.instances_dir},
        {"backups_dir", storage.backups_dir},
        {"temp_dir", storage.temp_dir}
    };
    j["runtime"] = {
        {"java_path", runtime.java_path},
        {"default_java_args", runtime.default_java_args},
        {"use_windows_job_objects", runtime.use_windows_job_objects},
        {"enforce_memory_limits", runtime.enforce_memory_limits},
        {"enforce_cpu_limits", runtime.enforce_cpu_limits}
    };
    j["network"] = {
        {"public_ip", network.public_ip},
        {"port_range_start", network.port_range_start},
        {"port_range_end", network.port_range_end},
        {"bedrock_port_range_start", network.bedrock_port_range_start},
        {"bedrock_port_range_end", network.bedrock_port_range_end}
    };

    return pretty ? j.dump(2) : j.dump();
}

} // namespace synx

