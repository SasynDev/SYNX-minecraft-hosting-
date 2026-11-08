#include "synx/agent/network_manager.hpp"
#include "synx/agent/console_manager.hpp"
#include "synx/agent/instance_manager.hpp"
#include "synx/agent/backup_manager.hpp"
#include "synx/agent/agent_api.hpp"
#include "synx/logger.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::cout << "[TEST] Running test_agent_skeleton..." << std::endl;

    synx::Logger::instance().initialize("", synx::LogLevel::Warning);

    std::filesystem::path test_instances = "temp/test_instances";
    std::filesystem::path test_backups = "temp/test_backups";
    std::filesystem::remove_all(test_instances);
    std::filesystem::remove_all(test_backups);

    auto net_mgr = std::make_shared<synx::agent::NetworkManager>(25565, 25575, 19132, 19140);
    auto console_mgr = std::make_shared<synx::agent::ConsoleManager>(100);
    auto inst_mgr = std::make_shared<synx::agent::InstanceManager>(test_instances, net_mgr, console_mgr);
    auto backup_mgr = std::make_shared<synx::agent::BackupManager>(test_backups);

    // 1. Test Network Port Allocation
    std::string inst_id = "test-instance-uuid-1";
    auto port_res = net_mgr->allocate_java_port(inst_id);
    assert(port_res.is_ok());
    assert(port_res.value() >= 25565 && port_res.value() <= 25575);

    auto bedrock_res = net_mgr->allocate_bedrock_port(inst_id);
    assert(bedrock_res.is_ok());
    assert(bedrock_res.value() >= 19132 && bedrock_res.value() <= 19140);

    // 2. Test Instance Manager Registration
    synx::InstanceConfig cfg;
    cfg.id = inst_id;
    cfg.name = "Test Survival Server";
    cfg.server_type = synx::ServerType::Paper;
    cfg.mc_version = "1.21.4";
    cfg.primary_allocation.port = port_res.value();
    cfg.limits.ram_limit_mb = 2048;

    auto reg_res = inst_mgr->register_instance(cfg);
    assert(reg_res.is_ok());

    auto state_res = inst_mgr->get_instance_state(inst_id);
    assert(state_res.is_ok());
    assert(state_res.value() == synx::InstanceState::Stopped);

    // Check files generated in instance directory
    assert(std::filesystem::exists(test_instances / inst_id / "eula.txt"));
    assert(std::filesystem::exists(test_instances / inst_id / "server.properties"));

    // 3. Test Agent API In-Memory Request Dispatching
    synx::SynxAgentConfig api_cfg = synx::SynxAgentConfig::create_default();
    api_cfg.agent.auth_token = "test_secret_token_12345";

    synx::agent::AgentApiServer api_server(api_cfg, inst_mgr, net_mgr, console_mgr, backup_mgr);

    // Health Check (Unauthenticated)
    synx::agent::HttpRequest req_health;
    req_health.method = "GET";
    req_health.path = "/api/v1/health";
    auto res_health = api_server.handle_request(req_health);
    assert(res_health.status_code == 200);
    assert(res_health.body.find("healthy") != std::string::npos);

    // Metrics without token (Must be rejected)
    synx::agent::HttpRequest req_metrics_noauth;
    req_metrics_noauth.method = "GET";
    req_metrics_noauth.path = "/api/v1/host/metrics";
    auto res_metrics_noauth = api_server.handle_request(req_metrics_noauth);
    assert(res_metrics_noauth.status_code == 401);

    // Metrics with valid token (Must succeed)
    synx::agent::HttpRequest req_metrics;
    req_metrics.method = "GET";
    req_metrics.path = "/api/v1/host/metrics";
    req_metrics.headers["x-synx-agent-token"] = "test_secret_token_12345";
    auto res_metrics = api_server.handle_request(req_metrics);
    assert(res_metrics.status_code == 200);
    assert(res_metrics.body.find("ram_total_mb") != std::string::npos);

    // List Instances
    synx::agent::HttpRequest req_list;
    req_list.method = "GET";
    req_list.path = "/api/v1/instances";
    req_list.headers["x-synx-agent-token"] = "test_secret_token_12345";
    auto res_list = api_server.handle_request(req_list);
    assert(res_list.status_code == 200);
    assert(res_list.body.find(inst_id) != std::string::npos);

    // Cleanup
    std::filesystem::remove_all(test_instances);
    std::filesystem::remove_all(test_backups);

    std::cout << "[TEST] test_agent_skeleton PASSED!" << std::endl;
    return 0;
}

