#include "synx/config.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "[TEST] Running test_config..." << std::endl;

    std::string config_path = "config/synx-agent.json";
    if (!std::filesystem::exists(config_path)) {
        config_path = "../config/synx-agent.json";
    }

    auto res = synx::SynxAgentConfig::load_from_file(config_path);
    assert(res.is_ok());

    const auto& cfg = res.value();
    assert(cfg.agent.node_id == "b0000000-0000-0000-0000-000000000001");
    assert(cfg.agent.listen_port == 8443);
    assert(cfg.agent.auth_token == "synx_agent_dev_token_default_secret_998244353");
    assert(cfg.network.port_range_start == 25565);

    // Test serialization
    std::string json_str = cfg.to_json_string();
    assert(!json_str.empty());
    assert(json_str.find("localhost-minecraft-server") != std::string::npos);

    std::cout << "[TEST] test_config PASSED!" << std::endl;
    return 0;
}
