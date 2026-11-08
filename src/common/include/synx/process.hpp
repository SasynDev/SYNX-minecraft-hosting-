#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace synx {

using StdoutCallback = std::function<void(const std::string& line)>;

struct ProcessMetrics {
    uint64_t working_set_bytes{0};
    uint64_t commit_bytes{0};
    double cpu_usage_pct{0.0};
    uint64_t uptime_seconds{0};
    uint32_t active_processes{0};
};

class ManagedProcess {
public:
    ManagedProcess();
    ~ManagedProcess();

    // Prevent copy
    ManagedProcess(const ManagedProcess&) = delete;
    ManagedProcess& operator=(const ManagedProcess&) = delete;

    // Launch process with optional Job Object resource limits
    Result<void> start(
        const std::string& executable,
        const std::vector<std::string>& arguments,
        const std::string& working_directory,
        const ResourceLimits& limits,
        StdoutCallback on_stdout = nullptr
    );

    // Command dispatching to child stdin
    Result<void> write_stdin(const std::string& command);

    // Termination
    Result<void> stop_graceful(const std::string& stop_command = "stop\n", uint32_t timeout_seconds = 15);
    Result<void> kill();

    // Query status & metrics
    bool is_running();
    int64_t pid() const { return pid_; }
    ProcessMetrics query_metrics();
    int exit_code() const { return exit_code_; }

private:
    void cleanup_handles();
    void stdout_reader_loop();

#ifdef _WIN32
    HANDLE process_handle_{nullptr};
    HANDLE thread_handle_{nullptr};
    HANDLE job_object_{nullptr};
    HANDLE stdin_write_{nullptr};
    HANDLE stdout_read_{nullptr};
#endif

    int64_t pid_{0};
    int exit_code_{-1};
    std::atomic<bool> is_running_{false};
    std::atomic<bool> stop_requested_{false};
    StdoutCallback stdout_callback_{nullptr};
    std::unique_ptr<std::thread> reader_thread_;
    std::chrono::steady_clock::time_point start_time_;

    // CPU measurement tracking
    uint64_t last_kernel_time_{0};
    uint64_t last_user_time_{0};
    std::chrono::steady_clock::time_point last_measure_time_;

    mutable std::mutex process_mutex_;
};

} // namespace synx

