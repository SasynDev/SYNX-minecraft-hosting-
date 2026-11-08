#include "synx/system_info.hpp"
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace synx {

static uint64_t file_time_to_uint64(const FILETIME& ft) {
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

double SystemInfo::calculate_cpu_percent() {
#ifdef _WIN32
    static uint64_t last_idle = 0;
    static uint64_t last_kernel = 0;
    static uint64_t last_user = 0;

    FILETIME idle_time, kernel_time, user_time;
    if (!GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        return 0.0;
    }

    uint64_t idle = file_time_to_uint64(idle_time);
    uint64_t kernel = file_time_to_uint64(kernel_time);
    uint64_t user = file_time_to_uint64(user_time);

    if (last_idle == 0 && last_kernel == 0 && last_user == 0) {
        last_idle = idle;
        last_kernel = kernel;
        last_user = user;
        return 0.0;
    }

    uint64_t delta_idle = idle - last_idle;
    uint64_t delta_kernel = kernel - last_kernel;
    uint64_t delta_user = user - last_user;

    last_idle = idle;
    last_kernel = kernel;
    last_user = user;

    uint64_t total_sys = delta_kernel + delta_user;
    if (total_sys == 0) return 0.0;

    double cpu_usage = (static_cast<double>(total_sys - delta_idle) / static_cast<double>(total_sys)) * 100.0;
    if (cpu_usage < 0.0) cpu_usage = 0.0;
    if (cpu_usage > 100.0) cpu_usage = 100.0;
    return cpu_usage;
#else
    return 0.0;
#endif
}

uint32_t SystemInfo::cpu_core_count() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#else
    return 1;
#endif
}

std::string SystemInfo::os_version_string() {
    return "Windows 11 64-bit (Build 26100+)";
}

uint64_t SystemInfo::uptime_seconds() {
#ifdef _WIN32
    return GetTickCount64() / 1000;
#else
    return 0;
#endif
}

HostMetrics SystemInfo::query_host_metrics(const std::string& drive_path) {
    HostMetrics metrics{};
    metrics.cpu_usage_pct = calculate_cpu_percent();
    metrics.uptime_seconds = uptime_seconds();

#ifdef _WIN32
    // 1. Memory Metrics
    MEMORYSTATUSEX mem_status{};
    mem_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&mem_status)) {
        metrics.ram_total_mb = mem_status.ullTotalPhys / (1024 * 1024);
        metrics.ram_free_mb = mem_status.ullAvailPhys / (1024 * 1024);
        metrics.ram_used_mb = metrics.ram_total_mb - metrics.ram_free_mb;
    }

    // 2. Disk Metrics
    std::wstring drive_w(drive_path.begin(), drive_path.end());
    ULARGE_INTEGER free_bytes_avail, total_bytes, total_free_bytes;
    if (GetDiskFreeSpaceExW(drive_w.c_str(), &free_bytes_avail, &total_bytes, &total_free_bytes)) {
        metrics.disk_total_gb = total_bytes.QuadPart / (1024 * 1024 * 1024);
        metrics.disk_free_gb = total_free_bytes.QuadPart / (1024 * 1024 * 1024);
        metrics.disk_used_gb = metrics.disk_total_gb - metrics.disk_free_gb;
    }
#endif

    return metrics;
}

} // namespace synx

