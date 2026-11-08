#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>

namespace synx::agent {

class NetworkManager {
public:
    NetworkManager(uint16_t java_start = 25565, uint16_t java_end = 25600,
                   uint16_t bedrock_start = 19132, uint16_t bedrock_end = 19150);

    // Allocates next free port in range
    Result<uint16_t> allocate_java_port(const std::string& instance_id);
    Result<uint16_t> allocate_bedrock_port(const std::string& instance_id);

    // Manually register or release a port
    Result<void> assign_specific_port(const std::string& instance_id, uint16_t port, const std::string& protocol = "tcp");
    void release_instance_ports(const std::string& instance_id);

    // Tests whether a port is physically available on the host using WinSock2
    static bool is_port_available_on_host(uint16_t port, const std::string& protocol = "tcp");

    std::vector<uint16_t> get_allocated_ports() const;

private:
    uint16_t java_start_;
    uint16_t java_end_;
    uint16_t bedrock_start_;
    uint16_t bedrock_end_;

    mutable std::mutex mutex_;
    std::unordered_map<uint16_t, std::string> port_to_instance_;
    std::unordered_map<std::string, std::vector<uint16_t>> instance_to_ports_;
};

} // namespace synx::agent

