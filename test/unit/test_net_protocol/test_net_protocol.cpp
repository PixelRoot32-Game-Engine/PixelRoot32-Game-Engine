/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the versioned wire protocol (networking/NetProtocol.h) and
 * the module value types (networking/NetTypes.h).
 *
 * The entire networking module is compiled out unless
 * PIXELROOT32_ENABLE_NETWORKING is 1, so this file follows the dialog pattern:
 * the functional branch exists only under the flag, and the flag-off branch
 * runs a single stub so the suite still proves the flags-off contract (a
 * translation unit that never references a networking symbol) under
 * [env:native_test].
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include "networking/NetProtocol.h"
#include "networking/NetTypes.h"

#include <cstdint>
#include <type_traits>

namespace config = pixelroot32::platforms::config;
using namespace pixelroot32::networking;

// Compile-time pins. A header that grew, or an application ceiling that no
// longer leaves room for one ESP-NOW frame, must fail the build rather than
// silently cost flash and break interop.
static_assert(kNetHeaderSize == 16, "The wire header is exactly 16 bytes.");
static_assert(kNetMaxAppPayload == 234, "A 250-byte ESP-NOW frame leaves 234 application bytes.");
static_assert(config::NetMaxPayload <= kNetMaxAppPayload,
              "The configured payload ceiling must fit one ESP-NOW frame.");
static_assert(kNetMagic == 0x5052, "The magic spells 'PR'.");
static_assert(kNetFlagReliable == 0x01, "The reliable flag owns bit 0.");
static_assert(std::is_trivially_copyable<NetAddress>::value,
              "NetAddress is copied by value and must stay trivially copyable.");

namespace {

// Wire offsets, mirrored from the layout table in
// docs/plan-esp-now-networking.md section 4.4.
constexpr uint16_t kOffsetMagic = 0;
constexpr uint16_t kOffsetVersion = 2;
constexpr uint16_t kOffsetType = 3;
constexpr uint16_t kOffsetFlags = 4;
constexpr uint16_t kOffsetSourceNodeId = 5;
constexpr uint16_t kOffsetSourceBootId = 6;
constexpr uint16_t kOffsetSequence = 8;
constexpr uint16_t kOffsetAckSequence = 12;
constexpr uint16_t kOffsetPayloadLength = 14;

// A fully populated header reused by most tests. It carries a 3-byte payload,
// so its complete frame is 19 bytes.
NetPacketHeader makeKnownHeader() {
    NetPacketHeader header;
    header.version = config::NetProtocolVersion;
    header.type = NetPacketType::Data;
    header.flags = kNetFlagReliable;
    header.sourceNodeId = 0x42;
    header.sourceBootId = 0xAABB;
    header.sequence = 0x11223344u;
    header.ackSequence = 0xCCDD;
    header.payloadLength = 3;
    return header;
}

// Writes the known header into `frame` (capacity >= 16) and returns the byte
// count reported by encodeHeader so callers can assert on it.
uint16_t writeKnownHeader(uint8_t* frame, uint16_t capacity) {
    return encodeHeader(makeKnownHeader(), frame, capacity);
}

// Patches the payloadLength field in place, so tests can build frames that
// encodeHeader would refuse to produce.
void patchPayloadLength(uint8_t* frame, uint16_t payloadLength) {
    frame[kOffsetPayloadLength] = static_cast<uint8_t>(payloadLength & 0xFFu);
    frame[kOffsetPayloadLength + 1] = static_cast<uint8_t>(payloadLength >> 8);
}

}  // namespace

// =============================================================================
// 1. Byte-exact layout
// =============================================================================

void test_net_protocol_header_byte_layout(void) {
    uint8_t frame[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kNetHeaderSize));

    // Magic spells "PR" in reading order: 0x50 then 0x52.
    TEST_ASSERT_EQUAL_HEX8(0x50, frame[kOffsetMagic]);
    TEST_ASSERT_EQUAL_HEX8(0x52, frame[kOffsetMagic + 1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, frame[kOffsetVersion]);
    TEST_ASSERT_EQUAL_HEX8(0x01, frame[kOffsetType]);
    TEST_ASSERT_EQUAL_HEX8(0x01, frame[kOffsetFlags]);
    TEST_ASSERT_EQUAL_HEX8(0x42, frame[kOffsetSourceNodeId]);
    // bootId 0xAABB -> BB AA (little-endian).
    TEST_ASSERT_EQUAL_HEX8(0xBB, frame[kOffsetSourceBootId]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, frame[kOffsetSourceBootId + 1]);
    // sequence 0x11223344 -> 44 33 22 11 (little-endian).
    TEST_ASSERT_EQUAL_HEX8(0x44, frame[kOffsetSequence]);
    TEST_ASSERT_EQUAL_HEX8(0x33, frame[kOffsetSequence + 1]);
    TEST_ASSERT_EQUAL_HEX8(0x22, frame[kOffsetSequence + 2]);
    TEST_ASSERT_EQUAL_HEX8(0x11, frame[kOffsetSequence + 3]);
    // ackSequence 0xCCDD -> DD CC (little-endian).
    TEST_ASSERT_EQUAL_HEX8(0xDD, frame[kOffsetAckSequence]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, frame[kOffsetAckSequence + 1]);
    // payloadLength 3 -> 03 00 (little-endian).
    TEST_ASSERT_EQUAL_HEX8(0x03, frame[kOffsetPayloadLength]);
    TEST_ASSERT_EQUAL_HEX8(0x00, frame[kOffsetPayloadLength + 1]);
}

// =============================================================================
// 2. Header round-trip
// =============================================================================

void test_net_protocol_header_round_trip(void) {
    // decodeHeader validates the whole declared frame shape, so the buffer
    // carries the 3 payload bytes its header announces.
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kFrameLength));

    NetPacketHeader decoded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::Ok),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    TEST_ASSERT_EQUAL_UINT8(config::NetProtocolVersion, decoded.version);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetPacketType::Data),
                            static_cast<uint8_t>(decoded.type));
    TEST_ASSERT_EQUAL_HEX8(kNetFlagReliable, decoded.flags);
    TEST_ASSERT_EQUAL_HEX8(0x42, decoded.sourceNodeId);
    TEST_ASSERT_EQUAL_UINT16(0xAABB, decoded.sourceBootId);
    TEST_ASSERT_EQUAL_UINT32(0x11223344u, decoded.sequence);
    TEST_ASSERT_EQUAL_UINT16(0xCCDD, decoded.ackSequence);
    TEST_ASSERT_EQUAL_UINT16(3, decoded.payloadLength);
}

// =============================================================================
// 3. encodeHeader capacity boundary
// =============================================================================

void test_net_protocol_encode_header_capacity_boundary(void) {
    uint8_t frame[kNetHeaderSize]{};

    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kNetHeaderSize));
    TEST_ASSERT_EQUAL_UINT16(0, writeKnownHeader(frame, kNetHeaderSize - 1));
    TEST_ASSERT_EQUAL_UINT16(0, writeKnownHeader(frame, 0));
}

// =============================================================================
// 4. Frame round-trip (zero-copy decode)
// =============================================================================

void test_net_protocol_frame_round_trip(void) {
    const uint8_t payload[3] = {0xDE, 0xAD, 0xBE};
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};

    TEST_ASSERT_EQUAL_UINT16(kFrameLength,
                             encodeFrame(makeKnownHeader(), payload, frame, kFrameLength));

    uint8_t snapshot[kFrameLength]{};
    for (uint16_t i = 0; i < kFrameLength; ++i) {
        snapshot[i] = frame[i];
    }

    NetPacketHeader decoded{};
    const uint8_t* decodedPayload = nullptr;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::Ok),
                            static_cast<uint8_t>(decodeFrame(frame, kFrameLength, decoded,
                                                             &decodedPayload)));

    TEST_ASSERT_EQUAL_PTR(frame + kNetHeaderSize, decodedPayload);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(payload, decodedPayload, 3);
    TEST_ASSERT_EQUAL_UINT16(3, decoded.payloadLength);

    // Decode is zero-copy: it must not rewrite the input frame.
    TEST_ASSERT_EQUAL_HEX8_ARRAY(snapshot, frame, kFrameLength);
}

void test_net_protocol_frame_without_payload_yields_null_pointer(void) {
    NetPacketHeader header = makeKnownHeader();
    header.payloadLength = 0;

    uint8_t frame[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, encodeFrame(header, nullptr, frame, kNetHeaderSize));

    NetPacketHeader decoded{};
    const uint8_t* decodedPayload = frame;  // Poisoned on purpose.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::Ok),
                            static_cast<uint8_t>(decodeFrame(frame, kNetHeaderSize, decoded,
                                                             &decodedPayload)));
    TEST_ASSERT_NULL(decodedPayload);
    TEST_ASSERT_EQUAL_UINT16(0, decoded.payloadLength);
}

// =============================================================================
// 5. encodeFrame capacity boundary
// =============================================================================

void test_net_protocol_encode_frame_capacity_boundary(void) {
    const uint8_t payload[3] = {0x01, 0x02, 0x03};
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};

    TEST_ASSERT_EQUAL_UINT16(0,
                             encodeFrame(makeKnownHeader(), payload, frame, kFrameLength - 1));
    TEST_ASSERT_EQUAL_UINT16(kFrameLength,
                             encodeFrame(makeKnownHeader(), payload, frame, kFrameLength));
}

// =============================================================================
// 6. Configured payload ceiling
// =============================================================================

void test_net_protocol_max_configured_payload_accepted(void) {
    constexpr uint16_t kFrameLength = kNetHeaderSize + config::NetMaxPayload;
    uint8_t payload[config::NetMaxPayload]{};
    for (uint16_t i = 0; i < config::NetMaxPayload; ++i) {
        payload[i] = static_cast<uint8_t>(i);
    }

    NetPacketHeader header = makeKnownHeader();
    header.payloadLength = config::NetMaxPayload;

    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kFrameLength,
                             encodeFrame(header, payload, frame, kFrameLength));

    NetPacketHeader decoded{};
    const uint8_t* decodedPayload = nullptr;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::Ok),
                            static_cast<uint8_t>(decodeFrame(frame, kFrameLength, decoded,
                                                             &decodedPayload)));
    TEST_ASSERT_EQUAL_UINT16(config::NetMaxPayload, decoded.payloadLength);
    TEST_ASSERT_EQUAL_PTR(frame + kNetHeaderSize, decodedPayload);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(payload, decodedPayload, config::NetMaxPayload);
}

void test_net_protocol_payload_over_configured_max_rejected(void) {
    constexpr uint16_t kTooLargePayload = config::NetMaxPayload + 1;
    constexpr uint16_t kFrameLength = kNetHeaderSize + kTooLargePayload;

    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kNetHeaderSize));
    patchPayloadLength(frame, kTooLargePayload);

    NetPacketHeader decoded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::PayloadTooLarge),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    const uint8_t* decodedPayload = frame;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::PayloadTooLarge),
                            static_cast<uint8_t>(decodeFrame(frame, kFrameLength, decoded,
                                                             &decodedPayload)));

    // Encode refuses to produce the oversized header in the first place.
    NetPacketHeader oversized = makeKnownHeader();
    oversized.payloadLength = kTooLargePayload;
    uint8_t out[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(0, encodeHeader(oversized, out, kNetHeaderSize));
}

// =============================================================================
// 7. Malformed frames
// =============================================================================

void test_net_protocol_truncated_frame_rejected(void) {
    uint8_t frame[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kNetHeaderSize));

    NetPacketHeader decoded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::TooShort),
                            static_cast<uint8_t>(decodeHeader(frame, kNetHeaderSize - 1, decoded)));

    const uint8_t* decodedPayload = nullptr;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::TooShort),
                            static_cast<uint8_t>(decodeFrame(frame, kNetHeaderSize - 1, decoded,
                                                             &decodedPayload)));
}

void test_net_protocol_bad_magic_rejected(void) {
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kFrameLength));

    frame[kOffsetMagic] = 0x51;
    NetPacketHeader decoded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadMagic),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    frame[kOffsetMagic] = 0x50;
    frame[kOffsetMagic + 1] = 0x53;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadMagic),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));
}

void test_net_protocol_bad_version_rejected(void) {
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kFrameLength));

    frame[kOffsetVersion] = static_cast<uint8_t>(config::NetProtocolVersion + 1);
    NetPacketHeader decoded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadVersion),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    // Encode validates the version too: only the build's wire version goes out.
    NetPacketHeader header = makeKnownHeader();
    header.version = static_cast<uint8_t>(config::NetProtocolVersion + 1);
    uint8_t out[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(0, encodeHeader(header, out, kNetHeaderSize));
}

void test_net_protocol_invalid_type_rejected(void) {
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kFrameLength));

    NetPacketHeader decoded{};
    frame[kOffsetType] = 0;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadType),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    frame[kOffsetType] = 6;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadType),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    // Every valid type is accepted.
    for (uint8_t type = 1; type <= 5; ++type) {
        frame[kOffsetType] = type;
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::Ok),
                                static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));
    }

    NetPacketHeader header = makeKnownHeader();
    header.type = static_cast<NetPacketType>(0);
    uint8_t out[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(0, encodeHeader(header, out, kNetHeaderSize));
    header.type = static_cast<NetPacketType>(6);
    TEST_ASSERT_EQUAL_UINT16(0, encodeHeader(header, out, kNetHeaderSize));
}

void test_net_protocol_reserved_flag_bit_rejected(void) {
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kFrameLength));

    NetPacketHeader decoded{};
    frame[kOffsetFlags] = 0x02;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadFlags),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    frame[kOffsetFlags] = 0x03;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::BadFlags),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    // 0x00 (unreliable) and 0x01 (reliable) are the only legal values.
    frame[kOffsetFlags] = 0x00;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::Ok),
                            static_cast<uint8_t>(decodeHeader(frame, kFrameLength, decoded)));

    NetPacketHeader header = makeKnownHeader();
    header.flags = 0x02;
    uint8_t out[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(0, encodeHeader(header, out, kNetHeaderSize));
}

void test_net_protocol_length_mismatch_rejected(void) {
    NetPacketHeader decoded{};
    const uint8_t* decodedPayload = nullptr;

    // Declares no payload but carries one extra byte.
    uint8_t zeroPayloadFrame[kNetHeaderSize + 1]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(zeroPayloadFrame, kNetHeaderSize));
    patchPayloadLength(zeroPayloadFrame, 0);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::LengthMismatch),
                            static_cast<uint8_t>(decodeHeader(zeroPayloadFrame,
                                                              kNetHeaderSize + 1, decoded)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::LengthMismatch),
                            static_cast<uint8_t>(decodeFrame(zeroPayloadFrame,
                                                             kNetHeaderSize + 1, decoded,
                                                             &decodedPayload)));

    // Declares a 2-byte payload inside a 19-byte frame, so one byte is stray.
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t strayByteFrame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(strayByteFrame, kNetHeaderSize));
    patchPayloadLength(strayByteFrame, 2);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::LengthMismatch),
                            static_cast<uint8_t>(decodeHeader(strayByteFrame, kFrameLength,
                                                              decoded)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::LengthMismatch),
                            static_cast<uint8_t>(decodeFrame(strayByteFrame, kFrameLength,
                                                             decoded, &decodedPayload)));
}

void test_net_protocol_null_arguments_rejected(void) {
    constexpr uint16_t kFrameLength = kNetHeaderSize + 3;
    uint8_t frame[kFrameLength]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize, writeKnownHeader(frame, kFrameLength));

    NetPacketHeader decoded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::InvalidArgument),
                            static_cast<uint8_t>(decodeHeader(nullptr, kFrameLength, decoded)));

    const uint8_t* decodedPayload = nullptr;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::InvalidArgument),
                            static_cast<uint8_t>(decodeFrame(nullptr, kFrameLength, decoded,
                                                             &decodedPayload)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetParseResult::InvalidArgument),
                            static_cast<uint8_t>(decodeFrame(frame, kFrameLength, decoded,
                                                             nullptr)));

    uint8_t out[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(0, encodeHeader(makeKnownHeader(), nullptr, kNetHeaderSize));
    TEST_ASSERT_EQUAL_UINT16(0, encodeFrame(makeKnownHeader(), frame, nullptr, kNetHeaderSize));
}

void test_net_protocol_encode_frame_requires_payload_pointer(void) {
    uint8_t frame[kNetHeaderSize + 3]{};
    // payloadLength is 3, so a null payload is a programming error, not an
    // empty frame.
    TEST_ASSERT_EQUAL_UINT16(0,
                             encodeFrame(makeKnownHeader(), nullptr, frame,
                                         static_cast<uint16_t>(sizeof(frame))));
}

// =============================================================================
// 8. NetAddress value semantics
// =============================================================================

void test_net_protocol_net_address_equality(void) {
    NetAddress a{};
    NetAddress b{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        a.bytes[i] = static_cast<uint8_t>(0x10 + i);
        b.bytes[i] = static_cast<uint8_t>(0x10 + i);
    }

    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_FALSE(a != b);

    b.bytes[kNetAddressSize - 1] = 0x00;
    TEST_ASSERT_FALSE(a == b);
    TEST_ASSERT_TRUE(a != b);
}

void test_net_protocol_net_address_predicates(void) {
    NetAddress zero{};
    TEST_ASSERT_TRUE(zero.isZero());
    TEST_ASSERT_FALSE(zero.isBroadcast());

    NetAddress broadcast{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        broadcast.bytes[i] = 0xFF;
    }
    TEST_ASSERT_TRUE(broadcast.isBroadcast());
    TEST_ASSERT_FALSE(broadcast.isZero());

    // Neither predicate may accept a partially matching address.
    zero.bytes[kNetAddressSize - 1] = 0x01;
    TEST_ASSERT_FALSE(zero.isZero());
    TEST_ASSERT_FALSE(zero.isBroadcast());

    NetAddress almostBroadcast = broadcast;
    almostBroadcast.bytes[0] = 0xFE;
    TEST_ASSERT_FALSE(almostBroadcast.isBroadcast());
    TEST_ASSERT_FALSE(almostBroadcast.isZero());
}

// =============================================================================
// 9. Compile-time pins, mirrored at runtime
// =============================================================================

void test_net_protocol_size_pins(void) {
    TEST_ASSERT_EQUAL_UINT16(16, kNetHeaderSize);
    TEST_ASSERT_EQUAL_UINT16(234, kNetMaxAppPayload);
    TEST_ASSERT_EQUAL_UINT16(250, kNetMaxFrameSize);
    TEST_ASSERT_TRUE(config::NetMaxPayload <= kNetMaxAppPayload);
    TEST_ASSERT_EQUAL_HEX16(0x5052, kNetMagic);
    TEST_ASSERT_EQUAL_HEX16(kNetMagic, static_cast<uint16_t>((0x50u << 8) | 0x52u));
}

#else  // !PIXELROOT32_ENABLE_NETWORKING

void test_net_protocol_flag_off_stub(void) {
    // With the flag off, NetTypes.h and NetProtocol.h declare nothing at all.
    // This translation unit compiling and passing without referencing any of
    // their symbols is the flags-off contract every existing game relies on.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_NETWORKING=0: the networking wire protocol is not "
        "compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_NETWORKING

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_NETWORKING
    RUN_TEST(test_net_protocol_header_byte_layout);
    RUN_TEST(test_net_protocol_header_round_trip);
    RUN_TEST(test_net_protocol_encode_header_capacity_boundary);
    RUN_TEST(test_net_protocol_frame_round_trip);
    RUN_TEST(test_net_protocol_frame_without_payload_yields_null_pointer);
    RUN_TEST(test_net_protocol_encode_frame_capacity_boundary);
    RUN_TEST(test_net_protocol_max_configured_payload_accepted);
    RUN_TEST(test_net_protocol_payload_over_configured_max_rejected);
    RUN_TEST(test_net_protocol_truncated_frame_rejected);
    RUN_TEST(test_net_protocol_bad_magic_rejected);
    RUN_TEST(test_net_protocol_bad_version_rejected);
    RUN_TEST(test_net_protocol_invalid_type_rejected);
    RUN_TEST(test_net_protocol_reserved_flag_bit_rejected);
    RUN_TEST(test_net_protocol_length_mismatch_rejected);
    RUN_TEST(test_net_protocol_null_arguments_rejected);
    RUN_TEST(test_net_protocol_encode_frame_requires_payload_pointer);
    RUN_TEST(test_net_protocol_net_address_equality);
    RUN_TEST(test_net_protocol_net_address_predicates);
    RUN_TEST(test_net_protocol_size_pins);
#else
    RUN_TEST(test_net_protocol_flag_off_stub);
#endif

    return UNITY_END();
}
