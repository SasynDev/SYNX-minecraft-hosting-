#include "synx/server_properties.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/logger.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "[TEST] Running test_server_properties (Cracked & Offline Player Support)..." << std::endl;

    synx::Logger::instance().initialize("", synx::LogLevel::Warning);

    // 1. Test ServerProperties default creation with cracked enabled
    auto props = synx::ServerProperties::create_default("SyNx Survival", 25565, true);
    assert(props.is_cracked());
    assert(props.get("online-mode") == "false");
    std::cout << "  [PASS] Default properties created with cracked mode enabled (online-mode=false)!" << std::endl;

    // 2. Test toggling cracked mode to false (premium only)
    props.set_cracked(false);
    assert(!props.is_cracked());
    assert(props.get("online-mode") == "true");
    std::cout << "  [PASS] Toggled cracked mode to premium only (online-mode=true)!" << std::endl;

    // 3. Test toggling cracked mode back to true
    props.set_cracked(true);
    assert(props.is_cracked());
    assert(props.get("online-mode") == "false");
    std::cout << "  [PASS] Toggled cracked mode back to enabled (online-mode=false)!" << std::endl;

    // 4. Test parsing server.properties with comments
    std::string mock_file = 
        "# Custom Minecraft Properties\n"
        "server-port=25570\n"
        "motd=Welcome\n"
        "online-mode=false\n"
        "max-players=50\n";

    auto parsed = synx::ServerProperties::parse(mock_file);
    assert(parsed.get("server-port") == "25570");
    assert(parsed.get("max-players") == "50");
    assert(parsed.is_cracked());

    // 5. Integration test with InstanceManager
    std::filesystem::path test_instances = "temp/test_cracked_instances";
    std::filesystem::remove_all(test_instances);

    auto net_mgr = std::make_shared<synx::agent::NetworkManager>(25565, 25570);
    auto console_mgr = std::make_shared<synx::agent::ConsoleManager>(50);
    auto inst_mgr = std::make_shared<synx::agent::InstanceManager>(test_instances, net_mgr, console_mgr);

    synx::InstanceConfig cfg;
    cfg.id = "test-cracked-server";
    cfg.name = "Cracked SMP";
    cfg.primary_allocation.port = 25565;
    cfg.cracked_enabled = true; // Cracked players can join!

    auto reg_res = inst_mgr->register_instance(cfg);
    assert(reg_res.is_ok());

    auto is_cracked_res = inst_mgr->is_cracked_mode("test-cracked-server");
    assert(is_cracked_res.is_ok());
    assert(is_cracked_res.value() == true);
    std::cout << "  [PASS] InstanceManager verified cracked mode enabled on disk!" << std::endl;

    // Toggle via InstanceManager API
    auto set_prem_res = inst_mgr->set_cracked_mode("test-cracked-server", false);
    assert(set_prem_res.is_ok());
    assert(inst_mgr->is_cracked_mode("test-cracked-server").value() == false);
    std::cout << "  [PASS] InstanceManager verified cracked mode switched to false!" << std::endl;

    auto set_crack_again = inst_mgr->set_cracked_mode("test-cracked-server", true);
    assert(set_crack_again.is_ok());
    assert(inst_mgr->is_cracked_mode("test-cracked-server").value() == true);
    std::cout << "  [PASS] InstanceManager verified cracked mode switched back to true!" << std::endl;

    std::filesystem::remove_all(test_instances);

    std::cout << "[TEST] test_server_properties PASSED!" << std::endl;
    return 0;
}

