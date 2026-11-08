#include "synx/agent/agent_service.hpp"
#include "synx/logger.hpp"
#include "synx/system_info.hpp"
#include <iostream>
#include <chrono>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace synx::agent {

static const wchar_t* SERVICE_NAME = L"SynxHostAgent";
static const wchar_t* SERVICE_DISPLAY_NAME = L"SYNX HOSTING - Host Orchestration Agent";
static SERVICE_STATUS g_service_status{};
static SERVICE_STATUS_HANDLE g_status_handle = nullptr;

#ifdef _WIN32
static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            SYNX_LOG_INFO("Agent", "Termination signal received, initiating graceful shutdown...");
            AgentService::instance().stop();
            return TRUE;
        default:
            return FALSE;
    }
}

static VOID WINAPI ServiceCtrlHandler(DWORD request) {
    switch (request) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            g_service_status.dwWin32ExitCode = 0;
            g_service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(g_status_handle, &g_service_status);
            AgentService::instance().stop();
            g_service_status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(g_status_handle, &g_service_status);
            break;
        default:
            break;
    }
}

static VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    (void)argc;
    (void)argv;
    g_status_handle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
    if (!g_status_handle) return;

    g_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_service_status.dwCurrentState = SERVICE_START_PENDING;
    g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus(g_status_handle, &g_service_status);

    auto init_res = AgentService::instance().initialize();
    if (init_res.is_err()) {
        g_service_status.dwCurrentState = SERVICE_STOPPED;
        g_service_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        SetServiceStatus(g_status_handle, &g_service_status);
        return;
    }

    g_service_status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_status_handle, &g_service_status);

    AgentService::instance().run_interactive();

    g_service_status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_status_handle, &g_service_status);
}
#endif

AgentService& AgentService::instance() {
    static AgentService s;
    return s;
}

AgentService::~AgentService() {
    stop();
}

Result<void> AgentService::initialize(const std::string& config_file) {
    // 1. Load Configuration
    auto cfg_res = SynxAgentConfig::load_from_file(config_file);
    if (cfg_res.is_err()) {
        SYNX_LOG_WARN("Agent", "Could not load " + config_file + " (" + cfg_res.error() + "), using default configuration.");
        config_ = SynxAgentConfig::create_default();
    } else {
        config_ = cfg_res.value();
    }

    // 2. Initialize Logger
    Logger::instance().initialize(config_.agent.log_file, config_.agent.log_level, config_.agent.console_buffer_lines);

    SYNX_LOG_INFO("Agent", "============================================================");
    SYNX_LOG_INFO("Agent", "   SYNX HOSTING — Independent Host Agent v1.0.0            ");
    SYNX_LOG_INFO("Agent", "   Host Node: " + config_.agent.node_name + " [" + config_.agent.node_id + "]");
    SYNX_LOG_INFO("Agent", "   OS: " + SystemInfo::os_version_string() + " | CPU Cores: " + std::to_string(SystemInfo::cpu_core_count()));
    SYNX_LOG_INFO("Agent", "============================================================");

    // 3. Initialize Subsystems
    network_mgr_ = std::make_shared<NetworkManager>(
        config_.network.port_range_start, config_.network.port_range_end,
        config_.network.bedrock_port_range_start, config_.network.bedrock_port_range_end
    );

    console_mgr_ = std::make_shared<ConsoleManager>(config_.agent.console_buffer_lines);

    instance_mgr_ = std::make_shared<InstanceManager>(
        config_.storage.instances_dir,
        network_mgr_,
        console_mgr_
    );

    backup_mgr_ = std::make_shared<BackupManager>(config_.storage.backups_dir);

    api_server_ = std::make_shared<AgentApiServer>(
        config_,
        instance_mgr_,
        network_mgr_,
        console_mgr_,
        backup_mgr_
    );

    watchdog_ = std::make_shared<InstanceWatchdog>(
        instance_mgr_,
        console_mgr_
    );

    pipe_server_ = std::make_shared<NamedPipeServer>(
        api_server_
    );

    return Result<void>::Ok();
}

Result<void> AgentService::run_interactive() {
    if (running_.load()) return Result<void>::Ok();

#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#endif

    running_ = true;

    // Start subsystems
    auto start_api_res = api_server_->start();
    if (start_api_res.is_err()) {
        SYNX_LOG_ERROR("Agent", "Failed to start API server: " + start_api_res.error());
        running_ = false;
        return start_api_res;
    }

    if (pipe_server_) {
        pipe_server_->start();
    }

    if (watchdog_) {
        watchdog_->start();
    }

    SYNX_LOG_INFO("Agent", "SYNX Host Agent running in interactive mode. Press Ctrl+C to terminate.");

    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return Result<void>::Ok();
}

Result<void> AgentService::run_as_service() {
#ifdef _WIN32
    SERVICE_TABLE_ENTRYW dispatch_table[] = {
        { const_cast<LPWSTR>(SERVICE_NAME), ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(dispatch_table)) {
        return Result<void>::Err("StartServiceCtrlDispatcher failed: " + std::to_string(GetLastError()));
    }
    return Result<void>::Ok();
#else
    return Result<void>::Err("Windows Service mode only supported on Windows.");
#endif
}

void AgentService::stop() {
    if (!running_.load()) return;
    running_ = false;

    SYNX_LOG_INFO("Agent", "Stopping Agent subsystems...");

    if (watchdog_) {
        watchdog_->stop();
    }

    if (pipe_server_) {
        pipe_server_->stop();
    }

    if (api_server_) {
        api_server_->stop();
    }

    if (instance_mgr_) {
        instance_mgr_->shutdown_all();
    }

    SYNX_LOG_INFO("Agent", "SYNX Host Agent terminated cleanly.");
}

Result<void> AgentService::install_windows_service() {
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        return Result<void>::Err("Cannot retrieve current module file name");
    }

    std::wstring bin_path = L"\"" + std::wstring(path) + L"\" --service";

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return Result<void>::Err("OpenSCManager failed: " + std::to_string(GetLastError()));

    SC_HANDLE service = CreateServiceW(
        scm,
        SERVICE_NAME,
        SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        bin_path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    if (!service) {
        DWORD err = GetLastError();
        CloseServiceHandle(scm);
        return Result<void>::Err("CreateService failed: " + std::to_string(err));
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return Result<void>::Ok();
#else
    return Result<void>::Err("Not supported");
#endif
}

Result<void> AgentService::uninstall_windows_service() {
#ifdef _WIN32
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return Result<void>::Err("OpenSCManager failed: " + std::to_string(GetLastError()));

    SC_HANDLE service = OpenServiceW(scm, SERVICE_NAME, DELETE);
    if (!service) {
        DWORD err = GetLastError();
        CloseServiceHandle(scm);
        return Result<void>::Err("OpenService failed: " + std::to_string(err));
    }

    BOOL ok = DeleteService(service);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    if (!ok) return Result<void>::Err("DeleteService failed: " + std::to_string(GetLastError()));
    return Result<void>::Ok();
#else
    return Result<void>::Err("Not supported");
#endif
}

Result<void> AgentService::start_windows_service() {
#ifdef _WIN32
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return Result<void>::Err("OpenSCManager failed: " + std::to_string(GetLastError()));

    SC_HANDLE service = OpenServiceW(scm, SERVICE_NAME, SERVICE_START);
    if (!service) {
        DWORD err = GetLastError();
        CloseServiceHandle(scm);
        return Result<void>::Err("OpenService failed: " + std::to_string(err));
    }

    BOOL ok = StartServiceW(service, 0, nullptr);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    if (!ok) return Result<void>::Err("StartService failed: " + std::to_string(GetLastError()));
    return Result<void>::Ok();
#else
    return Result<void>::Err("Not supported");
#endif
}

Result<void> AgentService::stop_windows_service() {
#ifdef _WIN32
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) return Result<void>::Err("OpenSCManager failed: " + std::to_string(GetLastError()));

    SC_HANDLE service = OpenServiceW(scm, SERVICE_NAME, SERVICE_STOP);
    if (!service) {
        DWORD err = GetLastError();
        CloseServiceHandle(scm);
        return Result<void>::Err("OpenService failed: " + std::to_string(err));
    }

    SERVICE_STATUS status{};
    BOOL ok = ControlService(service, SERVICE_CONTROL_STOP, &status);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    if (!ok) return Result<void>::Err("ControlService STOP failed: " + std::to_string(GetLastError()));
    return Result<void>::Ok();
#else
    return Result<void>::Err("Not supported");
#endif
}

Result<std::string> AgentService::query_windows_service_status() {
#ifdef _WIN32
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return Result<std::string>::Err("OpenSCManager failed: " + std::to_string(GetLastError()));

    SC_HANDLE service = OpenServiceW(scm, SERVICE_NAME, SERVICE_QUERY_STATUS);
    if (!service) {
        DWORD err = GetLastError();
        CloseServiceHandle(scm);
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            return Result<std::string>::Ok("NOT_INSTALLED");
        }
        return Result<std::string>::Err("OpenService failed: " + std::to_string(err));
    }

    SERVICE_STATUS_PROCESS ssp{};
    DWORD bytes_needed = 0;
    BOOL ok = QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytes_needed);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    if (!ok) return Result<std::string>::Err("QueryServiceStatusEx failed: " + std::to_string(GetLastError()));

    switch (ssp.dwCurrentState) {
        case SERVICE_STOPPED:          return Result<std::string>::Ok("STOPPED");
        case SERVICE_START_PENDING:    return Result<std::string>::Ok("START_PENDING");
        case SERVICE_STOP_PENDING:     return Result<std::string>::Ok("STOP_PENDING");
        case SERVICE_RUNNING:          return Result<std::string>::Ok("RUNNING");
        case SERVICE_CONTINUE_PENDING: return Result<std::string>::Ok("CONTINUE_PENDING");
        case SERVICE_PAUSE_PENDING:    return Result<std::string>::Ok("PAUSE_PENDING");
        case SERVICE_PAUSED:           return Result<std::string>::Ok("PAUSED");
        default:                       return Result<std::string>::Ok("UNKNOWN");
    }
#else
    return Result<std::string>::Err("Not supported");
#endif
}

} // namespace synx::agent
