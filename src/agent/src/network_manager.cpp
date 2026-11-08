#include "synx/agent/network_manager.hpp"
#include "synx/logger.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace synx::agent {

NetworkManager::NetworkManager(uint16_t java_start, uint16_t java_end,
                               uint16_t bedrock_start, uint16_t bedrock_end)
    : java_start_(java_start), java_end_(java_end),
      bedrock_start_(bedrock_start), bedrock_end_(bedrock_end) {}

bool NetworkManager::is_port_available_on_host(uint16_t port, const std::string& protocol) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }

    int type = (protocol == "udp") ? SOCK_DGRAM : SOCK_STREAM;
    int proto = (protocol == "udp") ? IPPROTO_UDP : IPPROTO_TCP;

    SOCKET s = socket(AF_INET, type, proto);
    if (s == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int bind_res = bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    closesocket(s);
    WSACleanup();

    return (bind_res == 0);
#else
    return true;
#endif
}

Result<uint16_t> NetworkManager::allocate_java_port(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (uint16_t port = java_start_; port <= java_end_; ++port) {
        if (port_to_instance_.find(port) == port_to_instance_.end()) {
            if (is_port_available_on_host(port, "tcp")) {
                port_to_instance_[port] = instance_id;
                instance_to_ports_[instance_id].push_back(port);
                SYNX_LOG_INFO("Network", "Allocated TCP port " + std::to_string(port) + " to instance " + instance_id);
                return Result<uint16_t>::Ok(port);
            }
        }
    }
    return Result<uint16_t>::Err("No available Java Edition ports in configured pool (" +
                                 std::to_string(java_start_) + "-" + std::to_string(java_end_) + ")");
}

Result<uint16_t> NetworkManager::allocate_bedrock_port(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (uint16_t port = bedrock_start_; port <= bedrock_end_; ++port) {
        if (port_to_instance_.find(port) == port_to_instance_.end()) {
            if (is_port_available_on_host(port, "udp")) {
                port_to_instance_[port] = instance_id;
                instance_to_ports_[instance_id].push_back(port);
                SYNX_LOG_INFO("Network", "Allocated UDP port " + std::to_string(port) + " to instance " + instance_id);
                return Result<uint16_t>::Ok(port);
            }
        }
    }
    return Result<uint16_t>::Err("No available Bedrock Edition ports in configured pool (" +
                                 std::to_string(bedrock_start_) + "-" + std::to_string(bedrock_end_) + ")");
}

Result<void> NetworkManager::assign_specific_port(const std::string& instance_id, uint16_t port, const std::string& protocol) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (port_to_instance_.find(port) != port_to_instance_.end()) {
        if (port_to_instance_[port] == instance_id) {
            return Result<void>::Ok(); // Already allocated to this instance
        }
        return Result<void>::Err("Port " + std::to_string(port) + " is already assigned to instance " + port_to_instance_[port]);
    }

    if (!is_port_available_on_host(port, protocol)) {
        return Result<void>::Err("Port " + std::to_string(port) + " is already bound by another process on host");
    }

    port_to_instance_[port] = instance_id;
    instance_to_ports_[instance_id].push_back(port);
    return Result<void>::Ok();
}

void NetworkManager::release_instance_ports(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = instance_to_ports_.find(instance_id);
    if (it != instance_to_ports_.end()) {
        for (uint16_t port : it->second) {
            port_to_instance_.erase(port);
            SYNX_LOG_INFO("Network", "Released port " + std::to_string(port) + " from instance " + instance_id);
        }
        instance_to_ports_.erase(it);
    }
}

std::vector<uint16_t> NetworkManager::get_allocated_ports() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint16_t> ports;
    for (const auto& [port, _] : port_to_instance_) {
        ports.push_back(port);
    }
    return ports;
}

} // namespace synx::agent

