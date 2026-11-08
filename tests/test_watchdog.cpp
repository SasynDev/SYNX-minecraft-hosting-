#include "synx/agent/watchdog.hpp"
#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/logger.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "[TEST] Running test_watchdog (Crash Detection & Recovery)..." << std::endl;

    synx::Logger::instance().initialize("", synx::LogLevel::Warning);

    std::filesystem::path test_instances = "temp/test_watchdog_instances";
    std::filesystem::remove_all(test_instances);

    auto net_mgr = std::make_shared<synx::agent::NetworkManager>(25580, 25590);
    auto console_mgr = std::make_shared<synx::agent::ConsoleManager>(100);
    auto inst_mgr = std::make_shared<synx::agent::InstanceManager>(test_instances, net_mgr, console_mgr);

    // Register test instance with auto_restart = true
    synx::InstanceConfig cfg;
    cfg.id = "test-crash-instance-1";
    cfg.name = "Crashing Server";
    cfg.auto_restart = true;
    cfg.primary_allocation.port = 25580;
    auto reg_res = inst_mgr->register_instance(cfg);
    assert(reg_res.is_ok());

    // Initialize watchdog with short 1-second backoff
    synx::agent::InstanceWatchdog watchdog(inst_mgr, console_mgr, 100, 3, 60, 1);

    // Verify initial metrics
    assert(watchdog.total_crashes_detected() == 0);
    assert(watchdog.total_auto_restarts() == 0);

    // Trigger simulated crash detection
    watchdog.check_all_instances();

    std::cout << "[TEST] test_watchdog PASSED!" << std::endl;

    std::filesystem::remove_all(test_instances);
    return 0;
}

