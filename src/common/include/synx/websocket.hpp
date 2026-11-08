#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace synx {

enum class WebSocketOpCode : uint8_t {
    Continuation = 0x0,
    Text         = 0x1,
    Binary       = 0x2,
    Close        = 0x8,
    Ping         = 0x9,
    Pong         = 0xA
};

struct WebSocketFrame {
    bool fin{true};
    WebSocketOpCode opcode{WebSocketOpCode::Text};
    bool is_masked{false};
    uint8_t masking_key[4]{0, 0, 0, 0};
    std::string payload;
};

class WebSocketProtocol {
public:
    // RFC 6455 Handshake Sec-WebSocket-Accept calculation
    static std::string compute_accept_key(const std::string& client_key);

    // Frame serialization (server-to-client frames are unmasked by default)
    static std::vector<uint8_t> encode_text_frame(const std::string& text, bool mask = false);
    static std::vector<uint8_t> encode_pong_frame(const std::string& payload = "");
    static std::vector<uint8_t> encode_close_frame(uint16_t status_code = 1000, const std::string& reason = "");

    // Frame deserialization
    static Result<WebSocketFrame> decode_frame(const uint8_t* data, size_t length, size_t& bytes_consumed);
};

} // namespace synx

