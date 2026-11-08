#include "synx/agent/instance_manager.hpp"
#include "synx/logger.hpp"
#include <sstream>

namespace synx::agent {

InstanceManager::InstanceManager(
    std::filesystem::path instances_root,
    std::shared_ptr<NetworkManager> network_mgr,
    std::shared_ptr<ConsoleManager> console_mgr
) : instances_root_(std::move(instances_root)),
    network_mgr_(std::move(network_mgr)),
    console_mgr_(std::move(console_mgr)) {
    std::error_code ec;
    std::filesystem::create_directories(instances_root_, ec);
    installer_ = std::make_shared<ServerInstaller>();
}

InstanceManager::~InstanceManager() {
    shutdown_all();
}

Result<void> InstanceManager::register_instance(const InstanceConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (config.id.empty()) {
        return Result<void>::Err("Instance ID cannot be empty");
    }

    if (instances_.find(config.id) != instances_.end()) {
        return Result<void>::Err("Instance already registered: " + config.id);
    }

    auto inst = std::make_unique<ManagedInstance>();
    inst->config = config;
    inst->state = InstanceState::Stopped;
    inst->process = std::make_unique<ManagedProcess>();

    std::filesystem::path instance_path = instances_root_ / config.id;
    inst->fs = std::make_unique<SandboxedFilesystem>(instance_path);

    // Install server software and crossplay/cracked configurations
    if (installer_) {
        installer_->install_server(instance_path.string(), config.server_type, config.mc_version, config.crossplay_enabled, config.cracked_enabled);
    }

    // Auto-create eula.txt if not exists
    auto eula_res = inst->fs->read_text_file("eula.txt");
    if (eula_res.is_err()) {
        inst->fs->write_text_file("eula.txt", "# By changing the setting below to TRUE you are indicating your agreement to our EULA (https://aka.ms/MinecraftEULA).\neula=true\n");
    }

    // Auto-create or ensure server.properties has correct port and cracked mode
    auto props_res = inst->fs->read_text_file("server.properties");
    ServerProperties props;
    if (props_res.is_err()) {
        props = ServerProperties::create_default(config.name, config.primary_allocation.port, config.cracked_enabled);
    } else {
        props = ServerProperties::parse(props_res.value());
        props.set("server-port", std::to_string(config.primary_allocation.port));
        props.set_cracked(config.cracked_enabled);
    }
    inst->fs->write_text_file("server.properties", props.to_string());

    // Reserve network port
    if (network_mgr_) {
        network_mgr_->assign_specific_port(config.id, config.primary_allocation.port, "tcp");
        if (config.bedrock_port.has_value()) {
            network_mgr_->assign_specific_port(config.id, config.bedrock_port.value(), "udp");
        }
    }

    instances_[config.id] = std::move(inst);
    SYNX_LOG_INFO("Instances", "Registered instance [" + config.id + "] (" + config.name + ") on port " + std::to_string(config.primary_allocation.port));
    return Result<void>::Ok();
}

Result<void> InstanceManager::start_instance(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<void>::Err("Instance not found: " + instance_id);
    }

    auto& inst = it->second;
    if (inst->process && inst->process->is_running()) {
        return Result<void>::Err("Instance is already running");
    }

    inst->process = std::make_unique<ManagedProcess>();

    inst->state = InstanceState::Starting;
    SYNX_LOG_INFO("Instances", "Starting instance: " + instance_id + " (" + inst->config.name + ")");

    std::filesystem::path work_dir = instances_root_ / instance_id;

    // Build JVM Launch Command
    std::string java_bin = inst->config.java_path.empty() ? "java.exe" : inst->config.java_path;
    std::vector<std::string> args;

    // Memory arguments
    args.push_back("-Xms512M");
    args.push_back("-Xmx" + std::to_string(inst->config.limits.ram_limit_mb) + "M");

    // Standard high-performance G1GC arguments
    args.push_back("-XX:+UseG1GC");
    args.push_back("-XX:+ParallelRefProcEnabled");
    args.push_back("-XX:MaxGCPauseMillis=200");

    // Custom JVM Flags if configured
    for (const auto& flag : inst->config.custom_jvm_flags) {
        args.push_back(flag);
    }

    // Server JAR
    args.push_back("-jar");
    args.push_back("server.jar");
    args.push_back("nogui");

    // Setup stdout/stderr forwarding to ConsoleManager
    auto console_mgr = console_mgr_;
    auto on_stdout = [console_mgr, instance_id](const std::string& line) {
        if (console_mgr) {
            console_mgr->append_line(instance_id, line);
        }
    };

    auto start_res = inst->process->start(
        java_bin,
        args,
        work_dir.string(),
        inst->config.limits,
        on_stdout
    );

    if (start_res.is_err()) {
        inst->state = InstanceState::Error;
        SYNX_LOG_ERROR("Instances", "Failed to start instance " + instance_id + ": " + start_res.error());
        return start_res;
    }

    inst->state = InstanceState::Running;
    SYNX_LOG_INFO("Instances", "Instance " + instance_id + " started successfully with PID " + std::to_string(inst->process->pid()));
    return Result<void>::Ok();
}

Result<void> InstanceManager::stop_instance(const std::string& instance_id, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<void>::Err("Instance not found: " + instance_id);
    }

    auto& inst = it->second;
    if (!inst->process->is_running()) {
        inst->state = InstanceState::Stopped;
        return Result<void>::Ok();
    }

    inst->state = InstanceState::Stopping;
    SYNX_LOG_INFO("Instances", "Stopping instance " + instance_id + (force ? " (FORCED)" : " (graceful)"));

    Result<void> res = force ? inst->process->kill() : inst->process->stop_graceful("stop\n", 15);
    inst->state = InstanceState::Stopped;
    return res;
}

Result<void> InstanceManager::restart_instance(const std::string& instance_id) {
    auto stop_res = stop_instance(instance_id, false);
    if (stop_res.is_err()) {
        stop_instance(instance_id, true);
    }
    return start_instance(instance_id);
}

Result<void> InstanceManager::send_command(const std::string& instance_id, const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<void>::Err("Instance not found: " + instance_id);
    }

    if (!it->second->process->is_running()) {
        return Result<void>::Err("Cannot send command to stopped instance");
    }

    // Echo command to console manager so UI sees what was sent
    if (console_mgr_) {
        console_mgr_->append_line(instance_id, "> " + command);
    }

    return it->second->process->write_stdin(command);
}

Result<InstanceState> InstanceManager::get_instance_state(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<InstanceState>::Err("Instance not found: " + instance_id);
    }

    if (it->second->process) {
        if (!it->second->process->is_running() && it->second->state == InstanceState::Running) {
            // Process terminated unexpectedly
            it->second->state = (it->second->process->exit_code() == 0) ? InstanceState::Stopped : InstanceState::Crashed;
        }
    }

    return Result<InstanceState>::Ok(it->second->state);
}

Result<InstanceMetrics> InstanceManager::get_instance_metrics(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<InstanceMetrics>::Err("Instance not found: " + instance_id);
    }

    const auto& inst = it->second;
    InstanceMetrics metrics{};
    metrics.instance_id = instance_id;
    metrics.state = inst->state;

    if (inst->process && inst->process->is_running()) {
        metrics.pid = inst->process->pid();
        auto p_metrics = inst->process->query_metrics();
        metrics.cpu_usage_pct = p_metrics.cpu_usage_pct;
        metrics.ram_used_mb = p_metrics.working_set_bytes / (1024 * 1024);
        metrics.uptime_seconds = p_metrics.uptime_seconds;
    }

    if (inst->fs) {
        metrics.disk_used_mb = inst->fs->calculate_total_size_bytes() / (1024 * 1024);
    }

    return Result<InstanceMetrics>::Ok(metrics);
}

Result<InstanceConfig> InstanceManager::get_instance_config(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<InstanceConfig>::Err("Instance not found: " + instance_id);
    }
    return Result<InstanceConfig>::Ok(it->second->config);
}

std::vector<InstanceConfig> InstanceManager::list_all_instances() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<InstanceConfig> list;
    list.reserve(instances_.size());
    for (const auto& [id, inst] : instances_) {
        list.push_back(inst->config);
    }
    return list;
}

Result<void> InstanceManager::delete_instance(const std::string& instance_id, bool delete_files) {
    std::unique_ptr<ManagedInstance> removed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = instances_.find(instance_id);
        if (it == instances_.end()) {
            return Result<void>::Err("Instance not found: " + instance_id);
        }
        removed = std::move(it->second);
        instances_.erase(it);
    }

    if (removed->process->is_running()) {
        removed->process->kill();
    }

    if (network_mgr_) {
        network_mgr_->release_instance_ports(instance_id);
    }

    if (console_mgr_) {
        console_mgr_->clear(instance_id);
    }

    if (delete_files) {
        std::error_code ec;
        std::filesystem::remove_all(instances_root_ / instance_id, ec);
    }

    SYNX_LOG_INFO("Instances", "Deleted instance: " + instance_id);
    return Result<void>::Ok();
}

void InstanceManager::shutdown_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    SYNX_LOG_INFO("Instances", "Gracefully shutting down all instances on host...");

    for (auto& [id, inst] : instances_) {
        if (inst->process && inst->process->is_running()) {
            SYNX_LOG_INFO("Instances", "Stopping instance " + id + " before exit");
            inst->process->stop_graceful("stop\n", 5);
            inst->state = InstanceState::Stopped;
        }
    }
}

Result<void> InstanceManager::set_cracked_mode(const std::string& instance_id, bool cracked_enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<void>::Err("Instance not found: " + instance_id);
    }

    auto& inst = it->second;
    inst->config.cracked_enabled = cracked_enabled;

    auto props_res = inst->fs->read_text_file("server.properties");
    ServerProperties props = props_res.is_ok() ? ServerProperties::parse(props_res.value())
                                               : ServerProperties::create_default(inst->config.name, inst->config.primary_allocation.port, cracked_enabled);

    props.set_cracked(cracked_enabled);
    auto write_res = inst->fs->write_text_file("server.properties", props.to_string());
    if (write_res.is_err()) {
        return write_res;
    }

    SYNX_LOG_INFO("Instances", "Cracked mode (offline-mode) set to " + std::string(cracked_enabled ? "ENABLED" : "DISABLED") + " for instance " + instance_id);
    return Result<void>::Ok();
}

Result<bool> InstanceManager::is_cracked_mode(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<bool>::Err("Instance not found: " + instance_id);
    }

    auto props_res = it->second->fs->read_text_file("server.properties");
    if (props_res.is_err()) {
        return Result<bool>::Ok(it->second->config.cracked_enabled);
    }

    ServerProperties props = ServerProperties::parse(props_res.value());
    return Result<bool>::Ok(props.is_cracked());
}

Result<ServerProperties> InstanceManager::get_server_properties(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<ServerProperties>::Err("Instance not found: " + instance_id);
    }

    auto props_res = it->second->fs->read_text_file("server.properties");
    if (props_res.is_err()) {
        return Result<ServerProperties>::Ok(ServerProperties::create_default(it->second->config.name, it->second->config.primary_allocation.port, it->second->config.cracked_enabled));
    }

    return Result<ServerProperties>::Ok(ServerProperties::parse(props_res.value()));
}

Result<void> InstanceManager::update_server_properties(const std::string& instance_id, const std::unordered_map<std::string, std::string>& updates) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        return Result<void>::Err("Instance not found: " + instance_id);
    }

    auto& inst = it->second;
    auto props_res = inst->fs->read_text_file("server.properties");
    ServerProperties props = props_res.is_ok() ? ServerProperties::parse(props_res.value())
                                               : ServerProperties::create_default(inst->config.name, inst->config.primary_allocation.port, inst->config.cracked_enabled);

    props.update_from_map(updates);

    if (props.has("online-mode")) {
        inst->config.cracked_enabled = props.is_cracked();
    }

    auto write_res = inst->fs->write_text_file("server.properties", props.to_string());
    if (write_res.is_err()) {
        return write_res;
    }

    SYNX_LOG_INFO("Instances", "Updated server.properties for instance " + instance_id);
    return Result<void>::Ok();
}

} // namespace synx::agent

