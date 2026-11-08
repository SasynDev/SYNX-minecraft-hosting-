#include "synx/types.hpp"
#include <random>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")
#endif

namespace synx {

std::string generate_uuid() {
#ifdef _WIN32
    UUID uuid;
    RPC_STATUS status = UuidCreate(&uuid);
    if (status == RPC_S_OK) {
        RPC_CSTR rpc_str = nullptr;
        if (UuidToStringA(&uuid, &rpc_str) == RPC_S_OK && rpc_str != nullptr) {
            std::string result(reinterpret_cast<char*>(rpc_str));
            RpcStringFreeA(&rpc_str);
            return result;
        }
    }
#endif

    // Fallback UUID v4 generator
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t high = dis(gen);
    uint64_t low = dis(gen);

    // Set UUID v4 variant and version bits
    high = (high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    low = (low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (high >> 32) << "-"
       << std::setw(4) << ((high >> 16) & 0xFFFF) << "-"
       << std::setw(4) << (high & 0xFFFF) << "-"
       << std::setw(4) << (low >> 48) << "-"
       << std::setw(12) << (low & 0xFFFFFFFFFFFFULL);

    return ss.str();
}

std::string generate_short_identifier() {
    static const char charset[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, sizeof(charset) - 2);

    std::string id = "mc-";
    for (int i = 0; i < 6; ++i) {
        id += charset[dis(gen)];
    }
    return id;
}

} // namespace synx
