/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the ping/pong codec (examples/esp_now_ping/src/
 * PingPongProtocol.h). The header is pure arithmetic over fixed-width
 * integers, so the suite runs under [env:native_test] and
 * [env:native_test_networking]: no networking, no Arduino, no display.
 *
 * The tests pin the whole wire contract: the 7-byte little-endian layout, the
 * two application tags, capacity and null rejection, exact-length decode, the
 * 32-bit millisecond wrap, and the fact that a ping and a pong with the same
 * fields differ only in the tag byte.
 */
#include <unity.h>

#include <cstdint>

#include "../../../examples/esp_now_ping/src/PingPongProtocol.h"

namespace {

using esp_now_ping::decodePing;
using esp_now_ping::decodePong;
using esp_now_ping::elapsedMs;
using esp_now_ping::encodePing;
using esp_now_ping::encodePong;
using esp_now_ping::kPingPongPayloadSize;
using esp_now_ping::kPingTag;
using esp_now_ping::kPongTag;
using esp_now_ping::PingPongMessage;

// Fill a scratch buffer with a byte no encoded field produces, so a rejected
// encode that wrote anything at all is visible.
void fillCanary(uint8_t *bytes, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        bytes[i] = 0xA5;
    }
}

void assertBytesEqual(const uint8_t *actual, const uint8_t *expected,
                      size_t count) {
    for (size_t i = 0; i < count; ++i) {
        TEST_ASSERT_EQUAL_UINT8(expected[i], actual[i]);
    }
}

}  // namespace

void setUp(void) {}
void tearDown(void) {}

// 1. Ping: exactly 7 bytes, tag 16, little-endian sequence and stamp. The
//    sample values have four distinct bytes so a byte-swap cannot pass.
void test_ping_round_trip_and_byte_layout(void) {
    PingPongMessage message;
    message.sequence = 0x1234u;
    message.stampMs = 0x0A0B0C0Du;

    uint8_t payload[kPingPongPayloadSize] = {};
    const uint16_t length = encodePing(payload, sizeof(payload), message);

    TEST_ASSERT_EQUAL_UINT8(7u, kPingPongPayloadSize);
    TEST_ASSERT_EQUAL_UINT8(16u, kPingTag);
    TEST_ASSERT_EQUAL_UINT16(7u, length);

    const uint8_t expected[kPingPongPayloadSize] = {
        16u,    // tag
        0x34u,  // sequence low byte
        0x12u,  // sequence high byte
        0x0Du,  // stamp byte 0 (least significant)
        0x0Cu,  // stamp byte 1
        0x0Bu,  // stamp byte 2
        0x0Au,  // stamp byte 3 (most significant)
    };
    assertBytesEqual(payload, expected, kPingPongPayloadSize);

    PingPongMessage decoded;
    TEST_ASSERT_TRUE(decodePing(payload, length, decoded));
    TEST_ASSERT_EQUAL_UINT16(0x1234u, decoded.sequence);
    TEST_ASSERT_EQUAL_UINT32(0x0A0B0C0Du, decoded.stampMs);
}

// 2. Pong: same layout, tag 17.
void test_pong_round_trip_and_byte_layout(void) {
    PingPongMessage message;
    message.sequence = 0xBEEFu;
    message.stampMs = 0x11223344u;

    uint8_t payload[kPingPongPayloadSize] = {};
    const uint16_t length = encodePong(payload, sizeof(payload), message);

    TEST_ASSERT_EQUAL_UINT8(17u, kPongTag);
    TEST_ASSERT_EQUAL_UINT16(7u, length);

    const uint8_t expected[kPingPongPayloadSize] = {
        17u,    // tag
        0xEFu,  // sequence low byte
        0xBEu,  // sequence high byte
        0x44u,  // stamp byte 0
        0x33u,  // stamp byte 1
        0x22u,  // stamp byte 2
        0x11u,  // stamp byte 3
    };
    assertBytesEqual(payload, expected, kPingPongPayloadSize);

    PingPongMessage decoded;
    TEST_ASSERT_TRUE(decodePong(payload, length, decoded));
    TEST_ASSERT_EQUAL_UINT16(0xBEEFu, decoded.sequence);
    TEST_ASSERT_EQUAL_UINT32(0x11223344u, decoded.stampMs);
}

// 3. Capacity boundary: 7 bytes succeeds, 6 returns 0 and writes nothing; a
//    null buffer returns 0.
void test_encode_rejects_short_capacity_and_null_buffer(void) {
    PingPongMessage message;
    message.sequence = 0x0102u;
    message.stampMs = 0x03040506u;

    uint8_t exact[kPingPongPayloadSize] = {};
    TEST_ASSERT_EQUAL_UINT16(7u, encodePing(exact, 7u, message));
    TEST_ASSERT_EQUAL_UINT16(7u, encodePong(exact, 7u, message));

    uint8_t shortBuffer[kPingPongPayloadSize] = {};
    fillCanary(shortBuffer, sizeof(shortBuffer));
    TEST_ASSERT_EQUAL_UINT16(0u, encodePing(shortBuffer, 6u, message));
    TEST_ASSERT_EQUAL_UINT16(0u, encodePong(shortBuffer, 6u, message));
    for (size_t i = 0; i < sizeof(shortBuffer); ++i) {
        TEST_ASSERT_EQUAL_UINT8(0xA5u, shortBuffer[i]);
    }

    TEST_ASSERT_EQUAL_UINT16(0u, encodePing(nullptr, 7u, message));
    TEST_ASSERT_EQUAL_UINT16(0u, encodePong(nullptr, kPingPongPayloadSize,
                                           message));
}

// 4. Decode rejects a wrong tag and every length other than exactly 7. A
//    rejected decode leaves the output untouched.
void test_decode_rejects_wrong_tag_and_wrong_length(void) {
    const uint8_t ping[kPingPongPayloadSize] = {16u,   0x34u, 0x12u, 0x0Du,
                                                0x0Cu, 0x0Bu, 0x0Au};
    PingPongMessage out;
    out.sequence = 0xAAAAu;
    out.stampMs = 0xBBBBBBBBu;

    uint8_t wrongTag[kPingPongPayloadSize];
    for (size_t i = 0; i < kPingPongPayloadSize; ++i) {
        wrongTag[i] = ping[i];
    }
    wrongTag[0] = 17u;
    TEST_ASSERT_FALSE(decodePing(wrongTag, kPingPongPayloadSize, out));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAu, out.sequence);
    TEST_ASSERT_EQUAL_UINT32(0xBBBBBBBBu, out.stampMs);

    wrongTag[0] = 16u;
    TEST_ASSERT_FALSE(decodePong(wrongTag, kPingPongPayloadSize, out));

    TEST_ASSERT_FALSE(decodePing(ping, 0u, out));
    TEST_ASSERT_FALSE(decodePing(ping, 6u, out));
    TEST_ASSERT_FALSE(decodePong(ping, 0u, out));
    TEST_ASSERT_FALSE(decodePong(ping, 6u, out));

    // A length-8 view of an 8-byte buffer: the extra byte is never read as a
    // field because the length check fails first.
    uint8_t longer[kPingPongPayloadSize + 1u] = {16u,   0x34u, 0x12u, 0x0Du,
                                                 0x0Cu, 0x0Bu, 0x0Au, 0xA5u};
    TEST_ASSERT_FALSE(decodePing(longer, 8u, out));
    longer[0] = 17u;
    TEST_ASSERT_FALSE(decodePong(longer, 8u, out));
}

// 4. A null payload pointer is rejected by both decoders.
void test_decode_rejects_null_pointer(void) {
    PingPongMessage out;
    TEST_ASSERT_FALSE(decodePing(nullptr, kPingPongPayloadSize, out));
    TEST_ASSERT_FALSE(decodePong(nullptr, kPingPongPayloadSize, out));
}

// 5. Every field at its maximum round-trips.
void test_maximum_values_round_trip(void) {
    PingPongMessage message;
    message.sequence = 0xFFFFu;
    message.stampMs = 0xFFFFFFFFu;

    const uint8_t expected[kPingPongPayloadSize] = {16u,   0xFFu, 0xFFu, 0xFFu,
                                                    0xFFu, 0xFFu, 0xFFu};

    uint8_t ping[kPingPongPayloadSize] = {};
    TEST_ASSERT_EQUAL_UINT16(7u, encodePing(ping, sizeof(ping), message));
    assertBytesEqual(ping, expected, kPingPongPayloadSize);

    PingPongMessage decoded;
    TEST_ASSERT_TRUE(decodePing(ping, 7u, decoded));
    TEST_ASSERT_EQUAL_UINT16(0xFFFFu, decoded.sequence);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, decoded.stampMs);

    uint8_t pong[kPingPongPayloadSize] = {};
    TEST_ASSERT_EQUAL_UINT16(7u, encodePong(pong, sizeof(pong), message));
    TEST_ASSERT_TRUE(decodePong(pong, 7u, decoded));
    TEST_ASSERT_EQUAL_UINT16(0xFFFFu, decoded.sequence);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, decoded.stampMs);
}

// 6. elapsedMs is an interval: unsigned subtraction gives the right answer
//    across the 32-bit millisecond wrap.
void test_elapsed_ms_handles_wrap(void) {
    TEST_ASSERT_EQUAL_UINT32(100u, elapsedMs(1000u, 900u));
    TEST_ASSERT_EQUAL_UINT32(21u, elapsedMs(5u, 0xFFFFFFF0u));
    TEST_ASSERT_EQUAL_UINT32(0u, elapsedMs(0xFFFFFFFFu, 0xFFFFFFFFu));
}

// 7. A ping and a pong with identical fields differ only in the tag byte.
void test_ping_and_pong_differ_only_in_tag(void) {
    PingPongMessage message;
    message.sequence = 0x0F0Eu;
    message.stampMs = 0xCAFEBABEu;

    uint8_t ping[kPingPongPayloadSize] = {};
    uint8_t pong[kPingPongPayloadSize] = {};
    TEST_ASSERT_EQUAL_UINT16(7u, encodePing(ping, sizeof(ping), message));
    TEST_ASSERT_EQUAL_UINT16(7u, encodePong(pong, sizeof(pong), message));

    TEST_ASSERT_EQUAL_UINT8(kPingTag, ping[0]);
    TEST_ASSERT_EQUAL_UINT8(kPongTag, pong[0]);
    TEST_ASSERT_NOT_EQUAL(ping[0], pong[0]);

    for (size_t i = 1; i < kPingPongPayloadSize; ++i) {
        TEST_ASSERT_EQUAL_UINT8(ping[i], pong[i]);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_ping_round_trip_and_byte_layout);
    RUN_TEST(test_pong_round_trip_and_byte_layout);
    RUN_TEST(test_encode_rejects_short_capacity_and_null_buffer);
    RUN_TEST(test_decode_rejects_wrong_tag_and_wrong_length);
    RUN_TEST(test_decode_rejects_null_pointer);
    RUN_TEST(test_maximum_values_round_trip);
    RUN_TEST(test_elapsed_ms_handles_wrap);
    RUN_TEST(test_ping_and_pong_differ_only_in_tag);
    return UNITY_END();
}
