#include "synx/agent/agent_service.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string config_path = "config/synx-agent.json";
    bool run_service = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--service") {
            run_service = true;
        } else if (arg == "--install") {
            auto res = synx::agent::AgentService::install_windows_service();
            if (res.is_ok()) {
                std::cout << "[SYNX] Windows Service 'SynxHostAgent' registered successfully.\n";
                return 0;
            } else {
                std::cerr << "[SYNX] Failed to install service: " << res.error() << "\n";
                return 1;
            }
        } else if (arg == "--uninstall") {
            auto res = synx::agent::AgentService::uninstall_windows_service();
            if (res.is_ok()) {
                std::cout << "[SYNX] Windows Service 'SynxHostAgent' removed successfully.\n";
                return 0;
            } else {
                std::cerr << "[SYNX] Failed to uninstall service: " << res.error() << "\n";
                return 1;
            }
        } else if (arg == "--start") {
            auto res = synx::agent::AgentService::start_windows_service();
            if (res.is_ok()) {
                std::cout << "[SYNX] Windows Service start request dispatched successfully.\n";
                return 0;
            } else {
                std::cerr << "[SYNX] Failed to start service: " << res.error() << "\n";
                return 1;
            }
        } else if (arg == "--stop") {
            auto res = synx::agent::AgentService::stop_windows_service();
            if (res.is_ok()) {
                std::cout << "[SYNX] Windows Service stop request dispatched successfully.\n";
                return 0;
            } else {
                std::cerr << "[SYNX] Failed to stop service: " << res.error() << "\n";
                return 1;
            }
        } else if (arg == "--status") {
            auto res = synx::agent::AgentService::query_windows_service_status();
            if (res.is_ok()) {
                std::cout << "[SYNX] Windows Service Status: " << res.value() << "\n";
                return 0;
            } else {
                std::cerr << "[SYNX] Failed to query status: " << res.error() << "\n";
                return 1;
            }
        } else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "SYNX HOSTING Host Agent v1.0.0 (Windows x64 / C++20)\n";
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: synx-agent.exe [options]\n\n"
                      << "Execution Modes:\n"
                      << "  (default)         Run in interactive console mode\n"
                      << "  --service         Run as a background Windows Service (SCM entrypoint)\n\n"
                      << "Service Management (Admin required):\n"
                      << "  --install         Register SynxHostAgent with Windows Service Control Manager\n"
                      << "  --uninstall       Unregister SynxHostAgent from SCM\n"
                      << "  --start           Start the installed Windows Service\n"
                      << "  --stop            Stop the running Windows Service\n"
                      << "  --status          Query the live status of the Windows Service\n\n"
                      << "General Options:\n"
                      << "  --config <file>   Path to configuration JSON file\n"
                      << "  --version, -v     Print version and exit\n"
                      << "  --help, -h        Show this help dialog\n";
            return 0;
        }
    }

    if (run_service) {
        auto res = synx::agent::AgentService::instance().run_as_service();
        return res.is_ok() ? 0 : 1;
    } else {
        auto init_res = synx::agent::AgentService::instance().initialize(config_path);
        if (init_res.is_err()) {
            std::cerr << "[SYNX] Initialization failed: " << init_res.error() << "\n";
            return 1;
        }
        auto run_res = synx::agent::AgentService::instance().run_interactive();
        return run_res.is_ok() ? 0 : 1;
    }
}
