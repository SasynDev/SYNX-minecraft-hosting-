#include "synx/websocket.hpp"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")
#endif

namespace synx {

static const char* WS_MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

std::string WebSocketProtocol::compute_accept_key(const std::string& client_key) {
    std::string combined = client_key + WS_MAGIC_GUID;

#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) return "";

    DWORD cbHash = 20; // SHA-1 is 160 bits (20 bytes)
    std::vector<BYTE> sha1_bytes(cbHash);

    status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
    if (BCRYPT_SUCCESS(status)) {
        BCryptHashData(hHash, reinterpret_cast<PUCHAR>(const_cast<char*>(combined.data())), static_cast<ULONG>(combined.size()), 0);
        BCryptFinishHash(hHash, sha1_bytes.data(), cbHash, 0);
        BCryptDestroyHash(hHash);
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);

    // Convert to Base64 using Windows Crypto API
    DWORD base64_len = 0;
    CryptBinaryToStringA(sha1_bytes.data(), cbHash, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &base64_len);

    std::string base64_result(base64_len, '\0');
    CryptBinaryToStringA(sha1_bytes.data(), cbHash, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64_result.data(), &base64_len);

    // Strip trailing null characters if any
    while (!base64_result.empty() && base64_result.back() == '\0') {
        base64_result.pop_back();
    }
    return base64_result;
#else
    return "";
#endif
}

std::vector<uint8_t> WebSocketProtocol::encode_text_frame(const std::string& text, bool mask) {
    std::vector<uint8_t> frame;
    size_t payload_len = text.size();

    // Byte 0: FIN bit (0x80) | Text OpCode (0x01)
    frame.push_back(0x81);

    // Byte 1+: Payload length encoding
    uint8_t mask_bit = mask ? 0x80 : 0x00;

    if (payload_len <= 125) {
        frame.push_back(mask_bit | static_cast<uint8_t>(payload_len));
    } else if (payload_len <= 0xFFFF) {
        frame.push_back(mask_bit | 126);
        frame.push_back(static_cast<uint8_t>((payload_len >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(payload_len & 0xFF));
    } else {
        frame.push_back(mask_bit | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((payload_len >> (8 * i)) & 0xFF));
        }
    }

    if (mask) {
        uint8_t masking_key[4] = {0x12, 0x34, 0x56, 0x78};
        for (int i = 0; i < 4; ++i) frame.push_back(masking_key[i]);
        for (size_t i = 0; i < payload_len; ++i) {
            frame.push_back(static_cast<uint8_t>(text[i] ^ masking_key[i % 4]));
        }
    } else {
        frame.insert(frame.end(), text.begin(), text.end());
    }

    return frame;
}

std::vector<uint8_t> WebSocketProtocol::encode_pong_frame(const std::string& payload) {
    std::vector<uint8_t> frame;
    // FIN (0x80) | Pong (0x0A)
    frame.push_back(0x8A);

    size_t len = (std::min)(payload.size(), static_cast<size_t>(125));
    frame.push_back(static_cast<uint8_t>(len));
    frame.insert(frame.end(), payload.begin(), payload.begin() + len);
    return frame;
}

std::vector<uint8_t> WebSocketProtocol::encode_close_frame(uint16_t status_code, const std::string& reason) {
    std::vector<uint8_t> frame;
    // FIN (0x80) | Close (0x08)
    frame.push_back(0x88);

    size_t payload_len = 2 + (std::min)(reason.size(), static_cast<size_t>(123));
    frame.push_back(static_cast<uint8_t>(payload_len));

    // Big endian status code
    frame.push_back(static_cast<uint8_t>((status_code >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(status_code & 0xFF));

    frame.insert(frame.end(), reason.begin(), reason.begin() + (payload_len - 2));
    return frame;
}

Result<WebSocketFrame> WebSocketProtocol::decode_frame(const uint8_t* data, size_t length, size_t& bytes_consumed) {
    bytes_consumed = 0;
    if (length < 2) {
        return Result<WebSocketFrame>::Err("Incomplete frame header: need at least 2 bytes");
    }

    WebSocketFrame frame;
    frame.fin = (data[0] & 0x80) != 0;
    frame.opcode = static_cast<WebSocketOpCode>(data[0] & 0x0F);

    frame.is_masked = (data[1] & 0x80) != 0;
    uint8_t basic_len = data[1] & 0x7F;

    size_t header_size = 2;
    uint64_t payload_len = 0;

    if (basic_len <= 125) {
        payload_len = basic_len;
    } else if (basic_len == 126) {
        header_size += 2;
        if (length < header_size) {
            return Result<WebSocketFrame>::Err("Incomplete 16-bit extended length");
        }
        payload_len = (static_cast<uint64_t>(data[2]) << 8) | data[3];
    } else { // 127
        header_size += 8;
        if (length < header_size) {
            return Result<WebSocketFrame>::Err("Incomplete 64-bit extended length");
        }
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | data[2 + i];
        }
    }

    if (frame.is_masked) {
        if (length < header_size + 4) {
            return Result<WebSocketFrame>::Err("Incomplete masking key");
        }
        for (int i = 0; i < 4; ++i) {
            frame.masking_key[i] = data[header_size + i];
        }
        header_size += 4;
    }

    if (length < header_size + payload_len) {
        return Result<WebSocketFrame>::Err("Incomplete payload: waiting for more data");
    }

    const uint8_t* payload_start = data + header_size;
    frame.payload.resize(static_cast<size_t>(payload_len));

    if (frame.is_masked) {
        for (size_t i = 0; i < payload_len; ++i) {
            frame.payload[i] = static_cast<char>(payload_start[i] ^ frame.masking_key[i % 4]);
        }
    } else {
        std::copy(payload_start, payload_start + payload_len, frame.payload.begin());
    }

    bytes_consumed = header_size + static_cast<size_t>(payload_len);
    return Result<WebSocketFrame>::Ok(frame);
}

} // namespace synx

