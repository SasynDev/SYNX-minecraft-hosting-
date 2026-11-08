#include "synx/agent/named_pipe_server.hpp"
#include "synx/agent/agent_api.hpp"
#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/agent/backup_manager.hpp"
#include "synx/logger.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::cout << "[TEST] Running test_named_pipe (Windows Native IPC Engine)..." << std::endl;

    synx::Logger::instance().initialize("", synx::LogLevel::Warning);

    std::filesystem::path test_instances = "temp/test_pipe_instances";
    std::filesystem::path test_backups = "temp/test_pipe_backups";
    std::filesystem::remove_all(test_instances);
    std::filesystem::remove_all(test_backups);

    auto net_mgr = std::make_shared<synx::agent::NetworkManager>(25590, 25595);
    auto console_mgr = std::make_shared<synx::agent::ConsoleManager>(100);
    auto inst_mgr = std::make_shared<synx::agent::InstanceManager>(test_instances, net_mgr, console_mgr);
    auto backup_mgr = std::make_shared<synx::agent::BackupManager>(test_backups);

    synx::SynxAgentConfig cfg = synx::SynxAgentConfig::create_default();
    auto api_server = std::make_shared<synx::agent::AgentApiServer>(
        cfg, inst_mgr, net_mgr, console_mgr, backup_mgr
    );

    std::wstring test_pipe_name = L"\\\\.\\pipe\\synx_test_pipe_ipc";
    synx::agent::NamedPipeServer pipe_server(api_server, test_pipe_name);

    auto start_res = pipe_server.start();
    assert(start_res.is_ok());
    assert(pipe_server.is_running());

    // Give server thread a moment to open pipe
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send IPC Health Request
    json req = {
        {"method", "GET"},
        {"path", "/api/v1/health"}
    };

    auto ipc_res = synx::agent::NamedPipeServer::send_ipc_message(test_pipe_name, req.dump());
    assert(ipc_res.is_ok());

    std::cout << "  IPC Response received: " << ipc_res.value() << std::endl;
    json resp = json::parse(ipc_res.value());
    assert(resp["status_code"] == 200);
    assert(resp["body"].get<std::string>().find("healthy") != std::string::npos);

    pipe_server.stop();
    assert(!pipe_server.is_running());

    std::filesystem::remove_all(test_instances);
    std::filesystem::remove_all(test_backups);

    std::cout << "[TEST] test_named_pipe PASSED!" << std::endl;
    return 0;
}

