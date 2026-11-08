#include "synx/crypto.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace synx {

std::string Crypto::generate_random_token(size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string token;
    token.resize(length);

#ifdef _WIN32
    std::vector<unsigned char> random_bytes(length);
    NTSTATUS status = BCryptGenRandom(
        nullptr,
        random_bytes.data(),
        static_cast<ULONG>(length),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    if (BCRYPT_SUCCESS(status)) {
        for (size_t i = 0; i < length; ++i) {
            token[i] = charset[random_bytes[i] % (sizeof(charset) - 1)];
        }
        return token;
    }
#endif

    // Fallback pseudo-random if BCrypt fails
    for (size_t i = 0; i < length; ++i) {
        token[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    return token;
}

std::string Crypto::sha256_string(const std::string& input) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) return "";

    DWORD cbHash = 32;
    std::vector<BYTE> hash_bytes(cbHash);

    status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
    if (BCRYPT_SUCCESS(status)) {
        BCryptHashData(hHash, reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())), static_cast<ULONG>(input.size()), 0);
        BCryptFinishHash(hHash, hash_bytes.data(), cbHash, 0);
        BCryptDestroyHash(hHash);
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);

    std::ostringstream ss;
    for (BYTE b : hash_bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
#else
    return "";
#endif
}

Result<std::string> Crypto::sha256_file(const std::string& filepath) {
#ifdef _WIN32
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return Result<std::string>::Err("Could not open file: " + filepath);
    }

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        return Result<std::string>::Err("Failed to open BCrypt SHA256 provider");
    }

    DWORD cbHash = 32;
    std::vector<BYTE> hash_bytes(cbHash);

    status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return Result<std::string>::Err("Failed to create hash object");
    }

    char buffer[65536];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        BCryptHashData(hHash, reinterpret_cast<PUCHAR>(buffer), static_cast<ULONG>(file.gcount()), 0);
    }

    BCryptFinishHash(hHash, hash_bytes.data(), cbHash, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    std::ostringstream ss;
    for (BYTE b : hash_bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return Result<std::string>::Ok(ss.str());
#else
    return Result<std::string>::Err("Platform not supported");
#endif
}

bool Crypto::constant_time_compare(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        result |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return result == 0;
}

} // namespace synx
