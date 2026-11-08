#include "synx/process.hpp"
#include "synx/logger.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace synx {

ManagedProcess::ManagedProcess() {
    start_time_ = std::chrono::steady_clock::now();
    last_measure_time_ = std::chrono::steady_clock::now();
}

ManagedProcess::~ManagedProcess() {
    stop_requested_ = true;
    if (is_running()) {
        kill();
    }
    if (reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
    cleanup_handles();
}

void ManagedProcess::cleanup_handles() {
#ifdef _WIN32
    if (stdin_write_) {
        CloseHandle(stdin_write_);
        stdin_write_ = nullptr;
    }
    if (stdout_read_) {
        CloseHandle(stdout_read_);
        stdout_read_ = nullptr;
    }
    if (thread_handle_) {
        CloseHandle(thread_handle_);
        thread_handle_ = nullptr;
    }
    if (process_handle_) {
        CloseHandle(process_handle_);
        process_handle_ = nullptr;
    }
    if (job_object_) {
        CloseHandle(job_object_);
        job_object_ = nullptr;
    }
#endif
}

Result<void> ManagedProcess::start(
    const std::string& executable,
    const std::vector<std::string>& arguments,
    const std::string& working_directory,
    const ResourceLimits& limits,
    StdoutCallback on_stdout
) {
    std::lock_guard<std::mutex> lock(process_mutex_);

    if (is_running_.load()) {
        return Result<void>::Err("Process is already running");
    }

    // Clean up any previous run's background reader thread and handles safely
    stop_requested_ = true;
    if (reader_thread_) {
        if (reader_thread_->joinable()) {
            reader_thread_->join();
        }
        reader_thread_.reset();
    }

#ifdef _WIN32
    if (stdin_write_) { CloseHandle(stdin_write_); stdin_write_ = nullptr; }
    if (stdout_read_) { CloseHandle(stdout_read_); stdout_read_ = nullptr; }
    if (thread_handle_) { CloseHandle(thread_handle_); thread_handle_ = nullptr; }
    if (process_handle_) { CloseHandle(process_handle_); process_handle_ = nullptr; }
    if (job_object_) { CloseHandle(job_object_); job_object_ = nullptr; }
#endif

    stdout_callback_ = std::move(on_stdout);
    stop_requested_ = false;

#ifdef _WIN32
    // 1. Create Pipes for stdin and stdout/stderr redirection
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdin_read = nullptr;
    if (!CreatePipe(&stdin_read, &stdin_write_, &sa, 0)) {
        return Result<void>::Err("Failed to create stdin pipe: error " + std::to_string(GetLastError()));
    }
    SetHandleInformation(stdin_write_, HANDLE_FLAG_INHERIT, 0);

    HANDLE stdout_write = nullptr;
    if (!CreatePipe(&stdout_read_, &stdout_write, &sa, 0)) {
        CloseHandle(stdin_read);
        CloseHandle(stdin_write_);
        stdin_write_ = nullptr;
        return Result<void>::Err("Failed to create stdout pipe: error " + std::to_string(GetLastError()));
    }
    SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0);

    // 2. Initialize Windows Job Object for strict hardware resource isolation
    job_object_ = CreateJobObjectW(nullptr, nullptr);
    if (!job_object_) {
        CloseHandle(stdin_read);
        CloseHandle(stdout_write);
        cleanup_handles();
        return Result<void>::Err("Failed to create Windows Job Object: error " + std::to_string(GetLastError()));
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
    jeli.BasicLimitInformation.ActiveProcessLimit = 16; // Prevent fork bombs

    if (limits.ram_limit_mb > 0) {
        jeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
        // Set hard committed memory limit (with small 256MB OS buffer for JVM metadata)
        uint64_t ram_bytes = (static_cast<uint64_t>(limits.ram_limit_mb) + 256) * 1024 * 1024;
        jeli.JobMemoryLimit = static_cast<SIZE_T>(ram_bytes);
    }

    if (!SetInformationJobObject(job_object_, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
        SYNX_LOG_WARN("Process", "Could not set Job Object memory limit (non-fatal): " + std::to_string(GetLastError()));
    }

    // Configure CPU rate limiting if specified
    if (limits.cpu_limit_pct > 0 && limits.cpu_limit_pct <= 100) {
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info{};
        cpu_info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        // Rate is specified in 1/100ths of a percent: 100% = 10,000
        cpu_info.CpuRate = static_cast<DWORD>(limits.cpu_limit_pct * 100);
        SetInformationJobObject(job_object_, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info));
    }

    // 3. Prepare Command Line
    std::wstring cmdline_w;
    // Quote executable if it has spaces
    if (executable.find(' ') != std::string::npos) {
        cmdline_w += L"\"" + std::wstring(executable.begin(), executable.end()) + L"\"";
    } else {
        cmdline_w += std::wstring(executable.begin(), executable.end());
    }

    for (const auto& arg : arguments) {
        cmdline_w += L" ";
        if (arg.find(' ') != std::string::npos) {
            cmdline_w += L"\"" + std::wstring(arg.begin(), arg.end()) + L"\"";
        } else {
            cmdline_w += std::wstring(arg.begin(), arg.end());
        }
    }

    std::wstring workdir_w(working_directory.begin(), working_directory.end());

    // 4. Setup STARTUPINFO with pipe handles
    STARTUPINFOW si{};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdin_read;
    si.hStdOutput = stdout_write;
    si.hStdError = stdout_write;

    PROCESS_INFORMATION pi{};

    // Launch process suspended so we can safely attach the Job Object before execution
    DWORD creation_flags = CREATE_SUSPENDED | CREATE_NO_WINDOW;

    BOOL ok = CreateProcessW(
        nullptr,
        cmdline_w.data(),
        nullptr,
        nullptr,
        TRUE, // Inherit handles
        creation_flags,
        nullptr,
        workdir_w.empty() ? nullptr : workdir_w.c_str(),
        &si,
        &pi
    );

    // Close pipe handles in agent that are now owned by child
    CloseHandle(stdin_read);
    CloseHandle(stdout_write);

    if (!ok) {
        DWORD err = GetLastError();
        cleanup_handles();
        return Result<void>::Err("CreateProcessW failed with error " + std::to_string(err));
    }

    process_handle_ = pi.hProcess;
    thread_handle_ = pi.hThread;
    pid_ = static_cast<int64_t>(pi.dwProcessId);

    // Assign process to Windows Job Object
    if (!AssignProcessToJobObject(job_object_, process_handle_)) {
        SYNX_LOG_WARN("Process", "AssignProcessToJobObject failed: " + std::to_string(GetLastError()));
    }

    // Resume execution
    ResumeThread(thread_handle_);
    is_running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    last_measure_time_ = start_time_;

    // Start background reader thread for stdout/stderr
    reader_thread_ = std::make_unique<std::thread>(&ManagedProcess::stdout_reader_loop, this);

    SYNX_LOG_INFO("Process", "Successfully spawned process [PID: " + std::to_string(pid_) + "] in Job Object.");
    return Result<void>::Ok();
#else
    return Result<void>::Err("Platform not supported; Windows 11 required.");
#endif
}

void ManagedProcess::stdout_reader_loop() {
#ifdef _WIN32
    char buffer[4096];
    std::string line_accumulator;

    while (!stop_requested_.load()) {
        DWORD bytes_read = 0;
        BOOL success = ReadFile(stdout_read_, buffer, sizeof(buffer) - 1, &bytes_read, nullptr);
        if (!success || bytes_read == 0) {
            break;
        }

        buffer[bytes_read] = '\0';
        for (DWORD i = 0; i < bytes_read; ++i) {
            char c = buffer[i];
            if (c == '\r') continue;
            if (c == '\n') {
                if (stdout_callback_) {
                    stdout_callback_(line_accumulator);
                }
                line_accumulator.clear();
            } else {
                line_accumulator += c;
            }
        }
    }

    if (!line_accumulator.empty() && stdout_callback_) {
        stdout_callback_(line_accumulator);
    }
#endif
    is_running_ = false;
}

Result<void> ManagedProcess::write_stdin(const std::string& command) {
#ifdef _WIN32
    if (!stdin_write_ || !is_running()) {
        return Result<void>::Err("Process is not running or stdin pipe is unavailable");
    }

    std::string cmd = command;
    if (cmd.empty() || cmd.back() != '\n') {
        cmd += "\n";
    }

    DWORD bytes_written = 0;
    BOOL ok = WriteFile(stdin_write_, cmd.data(), static_cast<DWORD>(cmd.size()), &bytes_written, nullptr);
    if (!ok) {
        return Result<void>::Err("Failed to write to process stdin: error " + std::to_string(GetLastError()));
    }
    return Result<void>::Ok();
#else
    return Result<void>::Err("Platform not supported");
#endif
}

bool ManagedProcess::is_running() {
#ifdef _WIN32
    if (!process_handle_) return false;
    DWORD exit_code = 0;
    if (GetExitCodeProcess(process_handle_, &exit_code)) {
        if (exit_code == STILL_ACTIVE) {
            return true;
        }
        exit_code_ = static_cast<int>(exit_code);
        is_running_ = false;
        return false;
    }
    return false;
#else
    return false;
#endif
}

Result<void> ManagedProcess::stop_graceful(const std::string& stop_command, uint32_t timeout_seconds) {
    if (!is_running()) {
        return Result<void>::Ok();
    }

    SYNX_LOG_INFO("Process", "Issuing graceful stop command to PID " + std::to_string(pid_));
    auto res = write_stdin(stop_command);
    if (res.is_err()) {
        SYNX_LOG_WARN("Process", "Failed to write stop command: " + res.error() + ", killing immediately.");
        return kill();
    }

#ifdef _WIN32
    DWORD wait_res = WaitForSingleObject(process_handle_, timeout_seconds * 1000);
    if (wait_res == WAIT_OBJECT_0) {
        GetExitCodeProcess(process_handle_, reinterpret_cast<LPDWORD>(&exit_code_));
        is_running_ = false;
        SYNX_LOG_INFO("Process", "Process " + std::to_string(pid_) + " exited gracefully with code " + std::to_string(exit_code_));
        return Result<void>::Ok();
    }

    SYNX_LOG_WARN("Process", "Process " + std::to_string(pid_) + " did not terminate within " + std::to_string(timeout_seconds) + "s, killing.");
    return kill();
#else
    return Result<void>::Ok();
#endif
}

Result<void> ManagedProcess::kill() {
    stop_requested_ = true;
#ifdef _WIN32
    if (job_object_) {
        TerminateJobObject(job_object_, 1);
    }
    if (process_handle_) {
        TerminateProcess(process_handle_, 1);
        WaitForSingleObject(process_handle_, 1000);
        GetExitCodeProcess(process_handle_, reinterpret_cast<LPDWORD>(&exit_code_));
    }
    is_running_ = false;
    return Result<void>::Ok();
#else
    return Result<void>::Ok();
#endif
}

ProcessMetrics ManagedProcess::query_metrics() {
    ProcessMetrics metrics{};
    if (!is_running()) return metrics;

    auto now = std::chrono::steady_clock::now();
    metrics.uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

#ifdef _WIN32
    if (job_object_) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
        if (QueryInformationJobObject(job_object_, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli), nullptr)) {
            metrics.commit_bytes = jeli.PeakJobMemoryUsed;
            metrics.working_set_bytes = jeli.PeakProcessMemoryUsed;
        }

        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jbai{};
        if (QueryInformationJobObject(job_object_, JobObjectBasicAccountingInformation, &jbai, sizeof(jbai), nullptr)) {
            metrics.active_processes = jbai.ActiveProcesses;

            // Calculate CPU % over elapsed time
            uint64_t current_k = static_cast<uint64_t>(jbai.TotalKernelTime.QuadPart);
            uint64_t current_u = static_cast<uint64_t>(jbai.TotalUserTime.QuadPart);

            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_measure_time_).count();
            if (elapsed_ms > 0 && last_measure_time_ != start_time_) {
                uint64_t delta_time = (current_k - last_kernel_time_) + (current_u - last_user_time_);
                // 100-nanosecond units to milliseconds = delta_time / 10000
                double cpu_time_ms = static_cast<double>(delta_time) / 10000.0;
                metrics.cpu_usage_pct = (cpu_time_ms / static_cast<double>(elapsed_ms)) * 100.0;
            }

            last_kernel_time_ = current_k;
            last_user_time_ = current_u;
            last_measure_time_ = now;
        }
    }
#endif
    return metrics;
}

} // namespace synx
