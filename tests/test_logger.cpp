#include "synx/logger.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "[TEST] Running test_logger..." << std::endl;

    std::string test_log_file = "logs/test_logger.log";
    std::filesystem::remove(test_log_file);

    synx::Logger::instance().initialize(test_log_file, synx::LogLevel::Trace, 50);

    bool subscriber_called = false;
    std::string received_msg;
    synx::Logger::instance().add_subscriber([&](const synx::LogEntry& entry) {
        subscriber_called = true;
        received_msg = entry.message;
    });

    SYNX_LOG_INFO("TestModule", "Testing info message");
    assert(subscriber_called);
    assert(received_msg == "Testing info message");

    SYNX_LOG_DEBUG("TestModule", "Testing debug message");
    SYNX_LOG_WARN("TestModule", "Testing warn message");
    SYNX_LOG_ERROR("TestModule", "Testing error message");

    auto logs = synx::Logger::instance().get_recent_logs(10);
    assert(logs.size() >= 4);

    synx::Logger::instance().shutdown();

    // Verify log file was written
    assert(std::filesystem::exists(test_log_file));
    assert(std::filesystem::file_size(test_log_file) > 0);

    std::cout << "[TEST] test_logger PASSED!" << std::endl;
    return 0;
}

