#include "synx/agent/watchdog.hpp"
#include "synx/logger.hpp"

namespace synx::agent {

InstanceWatchdog::InstanceWatchdog(
    std::shared_ptr<InstanceManager> instance_mgr,
    std::shared_ptr<ConsoleManager> console_mgr,
    uint32_t poll_interval_ms,
    uint32_t max_crashes_in_window,
    uint32_t crash_window_seconds,
    uint32_t restart_backoff_seconds
) : instance_mgr_(std::move(instance_mgr)),
    console_mgr_(std::move(console_mgr)),
    poll_interval_ms_(poll_interval_ms),
    max_crashes_in_window_(max_crashes_in_window),
    crash_window_seconds_(crash_window_seconds),
    restart_backoff_seconds_(restart_backoff_seconds) {}

InstanceWatchdog::~InstanceWatchdog() {
    stop();
}

void InstanceWatchdog::start() {
    if (is_running_.load()) return;
    is_running_ = true;
    worker_thread_ = std::make_unique<std::thread>(&InstanceWatchdog::watchdog_loop, this);
    SYNX_LOG_INFO("Watchdog", "Instance Watchdog started (poll: " + std::to_string(poll_interval_ms_) + "ms, crash window: " + std::to_string(crash_window_seconds_) + "s).");
}

void InstanceWatchdog::stop() {
    if (!is_running_.load()) return;
    is_running_ = false;

    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    SYNX_LOG_INFO("Watchdog", "Instance Watchdog stopped cleanly.");
}

void InstanceWatchdog::watchdog_loop() {
    while (is_running_.load()) {
        check_all_instances();
        std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms_));
    }
}

void InstanceWatchdog::check_all_instances() {
    if (!instance_mgr_) return;

    auto instances = instance_mgr_->list_all_instances();
    auto now = std::chrono::steady_clock::now();

    for (const auto& config : instances) {
        auto state_res = instance_mgr_->get_instance_state(config.id);
        if (state_res.is_err()) continue;

        auto current_state = state_res.value();

        // If the instance crashed
        if (current_state == InstanceState::Crashed) {
            handle_instance_crash(config.id, -1, config.auto_restart);
        } else if (current_state == InstanceState::Running) {
            // Check if running long enough (> 60s) to reset crash penalty
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = start_timestamps_.find(config.id);
            if (it != start_timestamps_.end()) {
                auto duration_sec = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
                if (duration_sec >= 60 && !crash_history_[config.id].empty()) {
                    SYNX_LOG_INFO("Watchdog", "Instance " + config.id + " has been stable for >60s. Resetting crash penalties.");
                    crash_history_[config.id].clear();
                }
            } else {
                start_timestamps_[config.id] = now;
            }
        }
    }
}

void InstanceWatchdog::handle_instance_crash(const std::string& instance_id, int exit_code, bool auto_restart_enabled) {
    total_crashes_++;

    auto now = std::chrono::steady_clock::now();
    bool can_restart = false;
    size_t recent_crashes = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        start_timestamps_.erase(instance_id);

        auto& history = crash_history_[instance_id];
        // Prune records older than window
        while (!history.empty()) {
            auto age_sec = std::chrono::duration_cast<std::chrono::seconds>(now - history.front().timestamp).count();
            if (age_sec > crash_window_seconds_) {
                history.pop_front();
            } else {
                break;
            }
        }

        history.push_back({now, exit_code});
        recent_crashes = history.size();

        if (auto_restart_enabled && recent_crashes <= max_crashes_in_window_) {
            can_restart = true;
        }
    }

    if (console_mgr_) {
        std::string alert = "[SYNX WATCHDOG] Instance crashed! (Exit code: " + std::to_string(exit_code) +
                            ", " + std::to_string(recent_crashes) + "/" + std::to_string(max_crashes_in_window_) +
                            " crashes in window)";
        console_mgr_->append_line(instance_id, alert);
    }

    SYNX_LOG_WARN("Watchdog", "Instance " + instance_id + " crashed (count: " +
                  std::to_string(recent_crashes) + "/" + std::to_string(max_crashes_in_window_) + ")");

    if (can_restart) {
        total_restarts_++;
        if (console_mgr_) {
            console_mgr_->append_line(instance_id, "[SYNX WATCHDOG] Auto-restart triggered. Respawning in " +
                                                   std::to_string(restart_backoff_seconds_) + " seconds...");
        }

        // Spawn detached recovery thread with backoff delay
        auto inst_mgr = instance_mgr_;
        uint32_t delay = restart_backoff_seconds_;
        std::thread([inst_mgr, instance_id, delay]() {
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            SYNX_LOG_INFO("Watchdog", "Executing automated restart for instance: " + instance_id);
            inst_mgr->start_instance(instance_id);
        }).detach();
    } else if (recent_crashes > max_crashes_in_window_) {
        std::string warning = "[SYNX WATCHDOG] Instance crashed too frequently (" + std::to_string(recent_crashes) +
                              " times). Halting automated restarts to prevent system exhaustion.";
        if (console_mgr_) {
            console_mgr_->append_line(instance_id, warning);
        }
        SYNX_LOG_ERROR("Watchdog", warning + " (" + instance_id + ")");
    }
}

} // namespace synx::agent

