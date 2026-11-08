#pragma once

#include "synx/types.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <chrono>

namespace synx::agent {

struct CrashRecord {
    std::chrono::steady_clock::time_point timestamp;
    int exit_code;
};

class InstanceWatchdog {
public:
    InstanceWatchdog(
        std::shared_ptr<InstanceManager> instance_mgr,
        std::shared_ptr<ConsoleManager> console_mgr,
        uint32_t poll_interval_ms = 500,
        uint32_t max_crashes_in_window = 5,
        uint32_t crash_window_seconds = 600,
        uint32_t restart_backoff_seconds = 3
    );
    ~InstanceWatchdog();

    void start();
    void stop();
    bool is_running() const { return is_running_.load(); }

    // Statistics
    uint32_t total_crashes_detected() const { return total_crashes_.load(); }
    uint32_t total_auto_restarts() const { return total_restarts_.load(); }

    // Manual check for unit tests
    void check_all_instances();

private:
    void watchdog_loop();
    void handle_instance_crash(const std::string& instance_id, int exit_code, bool auto_restart_enabled);

    std::shared_ptr<InstanceManager> instance_mgr_;
    std::shared_ptr<ConsoleManager> console_mgr_;
    uint32_t poll_interval_ms_;
    uint32_t max_crashes_in_window_;
    uint32_t crash_window_seconds_;
    uint32_t restart_backoff_seconds_;

    std::atomic<bool> is_running_{false};
    std::unique_ptr<std::thread> worker_thread_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::deque<CrashRecord>> crash_history_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> start_timestamps_;

    std::atomic<uint32_t> total_crashes_{0};
    std::atomic<uint32_t> total_restarts_{0};
};

} // namespace synx::agent

