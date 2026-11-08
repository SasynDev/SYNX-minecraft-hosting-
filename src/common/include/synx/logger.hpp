#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>

namespace synx {

struct LogEntry {
    std::string timestamp;
    LogLevel level;
    std::string tag;
    std::string message;
    uint32_t thread_id;
};

class Logger {
public:
    using LogSubscriber = std::function<void(const LogEntry&)>;

    static Logger& instance();

    void initialize(const std::string& log_file_path = "", LogLevel min_level = LogLevel::Debug, size_t ring_buffer_capacity = 1000);
    void shutdown();

    void set_min_level(LogLevel level);
    LogLevel min_level() const;

    void log(LogLevel level, const std::string& tag, const std::string& message);

    void trace(const std::string& tag, const std::string& message) { log(LogLevel::Trace, tag, message); }
    void debug(const std::string& tag, const std::string& message) { log(LogLevel::Debug, tag, message); }
    void info(const std::string& tag, const std::string& message)  { log(LogLevel::Info, tag, message); }
    void warn(const std::string& tag, const std::string& message)  { log(LogLevel::Warning, tag, message); }
    void error(const std::string& tag, const std::string& message) { log(LogLevel::Error, tag, message); }
    void fatal(const std::string& tag, const std::string& message) { log(LogLevel::Fatal, tag, message); }

    // Ring buffer access for UI / WebSockets
    std::vector<LogEntry> get_recent_logs(size_t max_count = 100) const;

    // Subscriber for real-time live events
    void add_subscriber(LogSubscriber subscriber);
    void clear_subscribers();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string format_console(const LogEntry& entry) const;
    std::string format_file(const LogEntry& entry) const;

    mutable std::mutex mutex_;
    LogLevel min_level_{LogLevel::Info};
    std::ofstream file_stream_;
    std::deque<LogEntry> ring_buffer_;
    size_t ring_buffer_capacity_{1000};
    std::vector<LogSubscriber> subscribers_;
    bool initialized_{false};
};

} // namespace synx

#define SYNX_LOG_TRACE(tag, msg) ::synx::Logger::instance().trace(tag, msg)
#define SYNX_LOG_DEBUG(tag, msg) ::synx::Logger::instance().debug(tag, msg)
#define SYNX_LOG_INFO(tag, msg)  ::synx::Logger::instance().info(tag, msg)
#define SYNX_LOG_WARN(tag, msg)  ::synx::Logger::instance().warn(tag, msg)
#define SYNX_LOG_ERROR(tag, msg) ::synx::Logger::instance().error(tag, msg)
#define SYNX_LOG_FATAL(tag, msg) ::synx::Logger::instance().fatal(tag, msg)

