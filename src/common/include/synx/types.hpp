#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>
#include <stdexcept>
#include <functional>
#include <iostream>

namespace synx {

// ==============================================================================
// 1. Result<T, E> Pattern for Safe Error Handling
// ==============================================================================

template<typename T, typename E = std::string>
class Result {
private:
    struct OkTag {};
    struct ErrTag {};

public:
    static Result<T, E> Ok(const T& val) { return Result(OkTag{}, val); }
    static Result<T, E> Ok(T&& val) { return Result(OkTag{}, std::move(val)); }
    static Result<T, E> Err(const E& err) { return Result(ErrTag{}, err); }
    static Result<T, E> Err(E&& err) { return Result(ErrTag{}, std::move(err)); }

    bool is_ok() const noexcept { return is_success_; }
    bool is_err() const noexcept { return !is_success_; }

    const T& value() const {
        if (!is_success_) throw std::runtime_error("Attempted to unwrap Err result: " + error_str());
        return val_;
    }

    T& value() {
        if (!is_success_) throw std::runtime_error("Attempted to unwrap Err result: " + error_str());
        return val_;
    }

    const E& error() const {
        if (is_success_) throw std::runtime_error("Attempted to get error from Ok result");
        return err_;
    }

    T value_or(T fallback) const {
        return is_success_ ? val_ : fallback;
    }

private:
    Result(OkTag, const T& val) : val_(val), is_success_(true) {}
    Result(OkTag, T&& val) : val_(std::move(val)), is_success_(true) {}
    Result(ErrTag, const E& err) : err_(err), is_success_(false) {}
    Result(ErrTag, E&& err) : err_(std::move(err)), is_success_(false) {}

    std::string error_str() const {
        if constexpr (std::is_same_v<E, std::string>) {
            return err_;
        } else {
            return "error occurred";
        }
    }

    T val_{};
    E err_{};
    bool is_success_{false};
};

// Void specialization for Result
template<typename E>
class Result<void, E> {
public:
    static Result<void, E> Ok() { return Result(true); }
    static Result<void, E> Err(const E& err) { return Result(err); }

    bool is_ok() const noexcept { return is_success_; }
    bool is_err() const noexcept { return !is_success_; }

    const E& error() const {
        if (is_success_) throw std::runtime_error("Attempted to get error from Ok result");
        return err_;
    }

private:
    explicit Result(bool success) : is_success_(success) {}
    explicit Result(const E& err) : err_(err), is_success_(false) {}

    E err_{};
    bool is_success_{false};
};

// ==============================================================================
// 2. Core Enumerations
// ==============================================================================

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

inline std::string log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

inline LogLevel log_level_from_string(const std::string& str) {
    if (str == "TRACE") return LogLevel::Trace;
    if (str == "DEBUG") return LogLevel::Debug;
    if (str == "INFO")  return LogLevel::Info;
    if (str == "WARN" || str == "WARNING") return LogLevel::Warning;
    if (str == "ERROR") return LogLevel::Error;
    if (str == "FATAL") return LogLevel::Fatal;
    return LogLevel::Info;
}

enum class InstanceState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Crashed,
    Installing,
    Suspended,
    Error
};

inline std::string instance_state_to_string(InstanceState state) {
    switch (state) {
        case InstanceState::Stopped:    return "stopped";
        case InstanceState::Starting:   return "starting";
        case InstanceState::Running:    return "running";
        case InstanceState::Stopping:   return "stopping";
        case InstanceState::Crashed:    return "crashed";
        case InstanceState::Installing: return "installing";
        case InstanceState::Suspended:  return "suspended";
        case InstanceState::Error:      return "error";
        default:                        return "unknown";
    }
}

inline InstanceState instance_state_from_string(const std::string& str) {
    if (str == "stopped")    return InstanceState::Stopped;
    if (str == "starting")   return InstanceState::Starting;
    if (str == "running")    return InstanceState::Running;
    if (str == "stopping")   return InstanceState::Stopping;
    if (str == "crashed")    return InstanceState::Crashed;
    if (str == "installing") return InstanceState::Installing;
    if (str == "suspended")  return InstanceState::Suspended;
    return InstanceState::Error;
}

enum class ServerType {
    Paper,
    Purpur,
    Vanilla,
    Fabric,
    Forge,
    Bedrock
};

inline std::string server_type_to_string(ServerType type) {
    switch (type) {
        case ServerType::Paper:   return "paper";
        case ServerType::Purpur:  return "purpur";
        case ServerType::Vanilla: return "vanilla";
        case ServerType::Fabric:  return "fabric";
        case ServerType::Forge:   return "forge";
        case ServerType::Bedrock: return "bedrock";
        default:                  return "paper";
    }
}

inline ServerType server_type_from_string(const std::string& str) {
    if (str == "purpur")  return ServerType::Purpur;
    if (str == "vanilla") return ServerType::Vanilla;
    if (str == "fabric")  return ServerType::Fabric;
    if (str == "forge")   return ServerType::Forge;
    if (str == "bedrock") return ServerType::Bedrock;
    return ServerType::Paper;
}

// ==============================================================================
// 3. Core Structures
// ==============================================================================

struct ResourceLimits {
    uint32_t ram_limit_mb{2048};
    uint32_t cpu_limit_pct{100}; // 100 = 1 full core, 200 = 2 cores
    uint64_t disk_limit_mb{20480}; // 20GB default
};

struct NetworkAllocation {
    std::string ip{"127.0.0.1"};
    uint16_t port{25565};
    std::string protocol{"tcp"};
    bool is_primary{true};
};

struct InstanceConfig {
    std::string id;                  // UUID
    std::string identifier;          // Short slug e.g. 'mc-8f3e1a'
    std::string name;                // Human readable name
    ServerType server_type{ServerType::Paper};
    std::string mc_version{"1.21.4"};
    uint32_t java_version{21};
    std::string java_path;
    ResourceLimits limits;
    NetworkAllocation primary_allocation;
    std::optional<uint16_t> bedrock_port;
    bool crossplay_enabled{false};
    bool cracked_enabled{true};      // Allows offline-mode / cracked players (e.g. TLauncher)
    bool auto_restart{true};
    std::vector<std::string> custom_jvm_flags;
    std::string working_dir;
};

struct HostMetrics {
    double cpu_usage_pct{0.0};
    uint64_t ram_total_mb{0};
    uint64_t ram_used_mb{0};
    uint64_t ram_free_mb{0};
    uint64_t disk_total_gb{0};
    uint64_t disk_used_gb{0};
    uint64_t disk_free_gb{0};
    uint32_t active_instances_count{0};
    uint32_t total_instances_count{0};
    uint64_t uptime_seconds{0};
};

struct InstanceMetrics {
    std::string instance_id;
    InstanceState state{InstanceState::Stopped};
    double cpu_usage_pct{0.0};
    uint64_t ram_used_mb{0};
    uint64_t disk_used_mb{0};
    uint64_t uptime_seconds{0};
    int64_t pid{0};
};

// UUID Helper
std::string generate_uuid();
std::string generate_short_identifier();

} // namespace synx
