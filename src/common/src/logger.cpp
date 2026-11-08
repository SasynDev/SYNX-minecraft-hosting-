#include "synx/logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace synx {

static void enable_windows_virtual_terminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(const std::string& log_file_path, LogLevel min_level, size_t ring_buffer_capacity) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = min_level;
    ring_buffer_capacity_ = ring_buffer_capacity;

    enable_windows_virtual_terminal();

    if (!log_file_path.empty()) {
        std::filesystem::path p(log_file_path);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        file_stream_.open(log_file_path, std::ios::out | std::ios::app);
    }
    initialized_ = true;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
    subscribers_.clear();
    initialized_ = false;
}

void Logger::set_min_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

LogLevel Logger::min_level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return min_level_;
}

void Logger::log(LogLevel level, const std::string& tag, const std::string& message) {
    if (level < min_level_) return;

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#ifdef _WIN32
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif

    std::ostringstream ts;
    ts << std::put_time(&bt, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();

    uint32_t thread_id = 0;
#ifdef _WIN32
    thread_id = static_cast<uint32_t>(GetCurrentThreadId());
#endif

    LogEntry entry{
        ts.str(),
        level,
        tag,
        message,
        thread_id
    };

    std::vector<LogSubscriber> current_subscribers;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Write formatted output to console
        std::cout << format_console(entry) << std::endl;

        // Write to log file if open
        if (file_stream_.is_open()) {
            file_stream_ << format_file(entry) << std::endl;
            file_stream_.flush();
        }

        // Store in ring buffer
        if (ring_buffer_.size() >= ring_buffer_capacity_) {
            ring_buffer_.pop_front();
        }
        ring_buffer_.push_back(entry);

        current_subscribers = subscribers_;
    }

    // Notify subscribers outside lock to prevent deadlocks
    for (const auto& sub : current_subscribers) {
        if (sub) {
            sub(entry);
        }
    }
}

std::string Logger::format_console(const LogEntry& entry) const {
    std::string color;
    switch (entry.level) {
        case LogLevel::Trace:   color = "\033[90m"; break;        // Bright Black / Gray
        case LogLevel::Debug:   color = "\033[36m"; break;        // Cyan
        case LogLevel::Info:    color = "\033[32m"; break;        // Green
        case LogLevel::Warning: color = "\033[33m"; break;        // Yellow
        case LogLevel::Error:   color = "\033[31m"; break;        // Red
        case LogLevel::Fatal:   color = "\033[97;41;1m"; break;   // Bold White on Red
    }
    const std::string purple = "\033[38;2;147;51;234m"; // SYNX Purple Accent
    const std::string reset  = "\033[0m";
    const std::string dim    = "\033[2m";

    std::ostringstream ss;
    ss << dim << "[" << entry.timestamp << "]" << reset << " "
       << color << std::setw(5) << std::left << log_level_to_string(entry.level) << reset << " "
       << purple << "[" << entry.tag << "]" << reset << " "
       << entry.message;
    return ss.str();
}

std::string Logger::format_file(const LogEntry& entry) const {
    std::ostringstream ss;
    ss << "[" << entry.timestamp << "] "
       << "[" << std::setw(5) << std::left << log_level_to_string(entry.level) << "] "
       << "[T:" << entry.thread_id << "] "
       << "[" << entry.tag << "] "
       << entry.message;
    return ss.str();
}

std::vector<LogEntry> Logger::get_recent_logs(size_t max_count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> result;
    size_t count = (std::min)(max_count, ring_buffer_.size());
    auto start_it = ring_buffer_.end() - count;
    result.assign(start_it, ring_buffer_.end());
    return result;
}

void Logger::add_subscriber(LogSubscriber subscriber) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_.push_back(std::move(subscriber));
}

void Logger::clear_subscribers() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_.clear();
}

} // namespace synx

