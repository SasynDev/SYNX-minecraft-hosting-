#pragma once

#include "synx/types.hpp"
#include <string>

namespace synx {

class SystemInfo {
public:
    static HostMetrics query_host_metrics(const std::string& drive_path = "C:\\");
    static uint32_t cpu_core_count();
    static std::string os_version_string();
    static uint64_t uptime_seconds();

private:
    static double calculate_cpu_percent();
};

} // namespace synx

