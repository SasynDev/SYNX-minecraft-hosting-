#include "synx/websocket.hpp"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "[TEST] Running test_websocket (RFC 6455 Specification Compliance)..." << std::endl;

    // 1. RFC 6455 Official Handshake Test Vector
    // Client nonce from RFC 6455 section 4.2.2
    std::string client_key = "dGhlIHNhbXBsZSBub25jZQ==";
    std::string expected_accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";

    std::string actual_accept = synx::WebSocketProtocol::compute_accept_key(client_key);
    std::cout << "  Computed Accept Key: " << actual_accept << std::endl;
    assert(actual_accept == expected_accept);
    std::cout << "  [PASS] RFC 6455 Handshake Sec-WebSocket-Accept verified!" << std::endl;

    // 2. Encode and Decode Unmasked Text Frame
    std::string test_message = "SYNX Live Console Stream Message #12345";
    auto encoded = synx::WebSocketProtocol::encode_text_frame(test_message, false);
    assert(!encoded.empty());

    size_t consumed = 0;
    auto decode_res = synx::WebSocketProtocol::decode_frame(encoded.data(), encoded.size(), consumed);
    assert(decode_res.is_ok());
    assert(consumed == encoded.size());
    assert(decode_res.value().payload == test_message);
    assert(decode_res.value().opcode == synx::WebSocketOpCode::Text);
    std::cout << "  [PASS] Unmasked text frame encode/decode verified!" << std::endl;

    // 3. Encode and Decode Masked Frame (Simulating Client-to-Server)
    auto masked_frame = synx::WebSocketProtocol::encode_text_frame("list", true);
    size_t masked_consumed = 0;
    auto masked_res = synx::WebSocketProtocol::decode_frame(masked_frame.data(), masked_frame.size(), masked_consumed);
    assert(masked_res.is_ok());
    assert(masked_res.value().is_masked);
    assert(masked_res.value().payload == "list");
    std::cout << "  [PASS] Masked client frame decoding verified!" << std::endl;

    // 4. Ping / Pong Frame
    auto pong = synx::WebSocketProtocol::encode_pong_frame("heartbeat");
    size_t pong_consumed = 0;
    auto pong_res = synx::WebSocketProtocol::decode_frame(pong.data(), pong.size(), pong_consumed);
    assert(pong_res.is_ok());
    assert(pong_res.value().opcode == synx::WebSocketOpCode::Pong);
    assert(pong_res.value().payload == "heartbeat");
    std::cout << "  [PASS] Ping/Pong frame encode/decode verified!" << std::endl;

    std::cout << "[TEST] test_websocket PASSED!" << std::endl;
    return 0;
}

