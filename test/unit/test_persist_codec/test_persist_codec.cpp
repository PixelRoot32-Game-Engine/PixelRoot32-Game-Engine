/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the persistence result codes and key rules
 * (persistence/StoreTypes.h) and the record envelope with its CRC-32
 * (persistence/RecordCodec.h).
 *
 * The entire persistence module is compiled out unless
 * PIXELROOT32_ENABLE_PERSISTENCE is 1, so this file follows the networking
 * pattern: the functional branch exists only under the flag, and the flag-off
 * branch runs a single stub so the suite still proves the flags-off contract
 * (a translation unit that never references a persistence symbol) under
 * [env:native_test].
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include "persistence/RecordCodec.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

using namespace pixelroot32::persistence;

namespace {

constexpr uint8_t kGuard = 0x5A;

// Encodes a record into `out` and asserts success; returns the written size.
size_t encodeOk(uint16_t schema, const uint8_t* payload, size_t size,
                uint8_t* out, size_t capacity) {
    size_t written = 12345;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(StoreResult::Ok),
                            static_cast<uint8_t>(RecordCodec::encode(
                                schema, payload, size, out, capacity, written)));
    TEST_ASSERT_EQUAL_size_t(RecordCodec::kHeaderSize + size, written);
    return written;
}

void assertResult(StoreResult expected, StoreResult actual) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),
                            static_cast<uint8_t>(actual));
}

// Decodes `in` and asserts it is rejected as Corrupt without touching the
// caller's outputs.
void assertDecodeCorrupt(const uint8_t* in, size_t size) {
    const uint8_t sentinel = 0;
    const uint8_t* payload = &sentinel;
    RecordInfo info{0xBEEF, 777};
    assertResult(StoreResult::Corrupt, RecordCodec::decode(in, size, payload, info));
    TEST_ASSERT_EQUAL_PTR(&sentinel, payload);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, info.schemaVersion);
    TEST_ASSERT_EQUAL_size_t(777, info.payloadSize);
}

// A small valid record used as the starting point of every corruption case.
const uint8_t kPayload[3] = {0xAA, 0xBB, 0xCC};
constexpr size_t kRecordSize = RecordCodec::kHeaderSize + sizeof(kPayload);

void makeRecord(uint8_t (&out)[kRecordSize]) {
    encodeOk(0x1234, kPayload, sizeof(kPayload), out, sizeof(out));
}

}  // namespace

// --- S2: result codes --------------------------------------------------------

void test_persist_store_result_values_are_distinct(void) {
    const StoreResult all[] = {
        StoreResult::Ok,          StoreResult::NotFound,  StoreResult::TooLarge,
        StoreResult::NoSpace,     StoreResult::Corrupt,   StoreResult::IoError,
        StoreResult::Unavailable, StoreResult::InvalidKey,
        StoreResult::InvalidArgument};
    const size_t count = sizeof(all) / sizeof(all[0]);
    TEST_ASSERT_EQUAL_size_t(9, count);
    TEST_ASSERT_EQUAL_size_t(1, sizeof(StoreResult));
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            TEST_ASSERT_NOT_EQUAL(static_cast<int>(all[i]), static_cast<int>(all[j]));
        }
    }
}

// --- S3: key rules -----------------------------------------------------------

void test_persist_key_valid_examples(void) {
    TEST_ASSERT_EQUAL_size_t(15, kMaxKeyLength);
    TEST_ASSERT_TRUE(isValidKey("cfg"));
    TEST_ASSERT_TRUE(isValidKey("doom"));
    TEST_ASSERT_TRUE(isValidKey("a"));
    TEST_ASSERT_TRUE(isValidKey("Slot_0-B"));
    TEST_ASSERT_TRUE(isValidKey("AZaz09_-"));
    TEST_ASSERT_TRUE(isValidKey("abcdefghijklmno"));  // exactly 15 bytes
}

void test_persist_key_invalid_examples(void) {
    TEST_ASSERT_FALSE(isValidKey(nullptr));
    TEST_ASSERT_FALSE(isValidKey(""));
    TEST_ASSERT_FALSE(isValidKey("abcdefghijklmnop"));  // 16 bytes
    TEST_ASSERT_FALSE(isValidKey("a b"));
    TEST_ASSERT_FALSE(isValidKey("a.b"));
    TEST_ASSERT_FALSE(isValidKey("a/b"));
    TEST_ASSERT_FALSE(isValidKey("a\\b"));
    TEST_ASSERT_FALSE(isValidKey("a:b"));
    TEST_ASSERT_FALSE(isValidKey("a*b"));
    TEST_ASSERT_FALSE(isValidKey("caf\xC3\xA9"));  // non-ASCII bytes
    TEST_ASSERT_FALSE(isValidKey("\x80"));
}

void test_persist_key_scan_stops_after_limit(void) {
    // Sixteen valid characters and no terminator: the validator must reject
    // the key by looking at the 16th byte, never reading the 17th.
    char unterminated[16];
    std::memset(unterminated, 'k', sizeof(unterminated));
    TEST_ASSERT_FALSE(isValidKey(unterminated));
}

// --- S8: CRC-32 --------------------------------------------------------------

void test_persist_crc32_check_value(void) {
    const char* check = "123456789";
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u,
                            crc32(reinterpret_cast<const uint8_t*>(check), 9));
}

void test_persist_crc32_empty_input(void) {
    TEST_ASSERT_EQUAL_HEX32(0u, crc32(nullptr, 0));
    const uint8_t byte = 0x42;
    TEST_ASSERT_EQUAL_HEX32(0u, crc32(&byte, 0));
    TEST_ASSERT_EQUAL_HEX32(0x1234ABCDu, crc32(&byte, 0, 0x1234ABCDu));
}

void test_persist_crc32_chaining_equals_one_shot(void) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>("123456789");
    for (size_t split = 0; split <= 9; ++split) {
        const uint32_t partial = crc32(data, split);
        TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32(data + split, 9 - split, partial));
    }
}

// --- S8: encode / decode -----------------------------------------------------

void test_persist_codec_constants(void) {
    TEST_ASSERT_EQUAL_size_t(16, RecordCodec::kHeaderSize);
    TEST_ASSERT_EQUAL_UINT8(1, RecordCodec::kEnvelopeVersion);
}

void test_persist_codec_known_record_byte_layout(void) {
    uint8_t out[kRecordSize];
    makeRecord(out);
    // Reference CRC from zlib.crc32 over header bytes 0..11 + payload.
    const uint8_t expected[kRecordSize] = {
        'P',  'R',  'S',  'T',   // magic
        0x01,                    // envelope version
        0x00,                    // reserved
        0x34, 0x12,              // schema version 0x1234, little-endian
        0x03, 0x00, 0x00, 0x00,  // payload length 3, little-endian
        0xA2, 0x91, 0x3B, 0xE8,  // CRC32 0xE83B91A2, little-endian
        0xAA, 0xBB, 0xCC};       // payload
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, kRecordSize);
}

void test_persist_codec_round_trip(void) {
    uint8_t payload[40];
    for (size_t i = 0; i < sizeof(payload); ++i) {
        payload[i] = static_cast<uint8_t>(i * 7 + 1);
    }
    uint8_t buffer[RecordCodec::kHeaderSize + sizeof(payload)];
    const size_t written = encodeOk(7, payload, sizeof(payload), buffer, sizeof(buffer));

    const uint8_t* decoded = nullptr;
    RecordInfo info{};
    assertResult(StoreResult::Ok, RecordCodec::decode(buffer, written, decoded, info));
    TEST_ASSERT_EQUAL_PTR(buffer + RecordCodec::kHeaderSize, decoded);
    TEST_ASSERT_EQUAL_HEX16(7, info.schemaVersion);
    TEST_ASSERT_EQUAL_size_t(sizeof(payload), info.payloadSize);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(payload, decoded, sizeof(payload));
}

void test_persist_codec_round_trip_empty_payload(void) {
    uint8_t buffer[RecordCodec::kHeaderSize];
    const size_t written = encodeOk(0, nullptr, 0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_size_t(RecordCodec::kHeaderSize, written);

    const uint8_t* decoded = nullptr;
    RecordInfo info{0xAAAA, 99};
    assertResult(StoreResult::Ok, RecordCodec::decode(buffer, written, decoded, info));
    TEST_ASSERT_EQUAL_PTR(buffer + RecordCodec::kHeaderSize, decoded);
    TEST_ASSERT_EQUAL_HEX16(0, info.schemaVersion);
    TEST_ASSERT_EQUAL_size_t(0, info.payloadSize);
}

void test_persist_codec_reports_any_schema_version(void) {
    uint8_t buffer[kRecordSize];
    encodeOk(0xFFFF, kPayload, sizeof(kPayload), buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[6]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[7]);

    const uint8_t* decoded = nullptr;
    RecordInfo info{};
    assertResult(StoreResult::Ok, RecordCodec::decode(buffer, sizeof(buffer), decoded, info));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, info.schemaVersion);
}

void test_persist_codec_encode_exact_capacity_succeeds(void) {
    uint8_t buffer[kRecordSize + 1];
    buffer[kRecordSize] = kGuard;
    encodeOk(1, kPayload, sizeof(kPayload), buffer, kRecordSize);
    TEST_ASSERT_EQUAL_HEX8(kGuard, buffer[kRecordSize]);
}

void test_persist_codec_encode_capacity_one_short_is_too_large(void) {
    uint8_t buffer[kRecordSize];
    std::memset(buffer, kGuard, sizeof(buffer));
    size_t written = 12345;
    assertResult(StoreResult::TooLarge,
                 RecordCodec::encode(1, kPayload, sizeof(kPayload), buffer,
                                     kRecordSize - 1, written));
    TEST_ASSERT_EQUAL_size_t(0, written);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(kGuard, buffer[i]);
    }

    // Too small even for the header of an empty record.
    written = 12345;
    assertResult(StoreResult::TooLarge,
                 RecordCodec::encode(1, nullptr, 0, buffer,
                                     RecordCodec::kHeaderSize - 1, written));
    TEST_ASSERT_EQUAL_size_t(0, written);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(kGuard, buffer[i]);
    }
}

void test_persist_codec_encode_invalid_arguments(void) {
    uint8_t buffer[kRecordSize];
    size_t written = 12345;
    assertResult(StoreResult::InvalidArgument,
                 RecordCodec::encode(1, kPayload, sizeof(kPayload), nullptr,
                                     kRecordSize, written));
    TEST_ASSERT_EQUAL_size_t(0, written);

    std::memset(buffer, kGuard, sizeof(buffer));
    written = 12345;
    assertResult(StoreResult::InvalidArgument,
                 RecordCodec::encode(1, nullptr, 3, buffer, sizeof(buffer), written));
    TEST_ASSERT_EQUAL_size_t(0, written);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(kGuard, buffer[i]);
    }

#if SIZE_MAX > UINT32_MAX
    // A payload whose length does not fit the u32 field. The pointer is never
    // dereferenced: the size check rejects the call first.
    written = 12345;
    assertResult(StoreResult::InvalidArgument,
                 RecordCodec::encode(1, kPayload,
                                     static_cast<size_t>(UINT32_MAX) + 1u, buffer,
                                     SIZE_MAX, written));
    TEST_ASSERT_EQUAL_size_t(0, written);
#endif
}

void test_persist_codec_decode_null_input_is_invalid_argument(void) {
    const uint8_t sentinel = 0;
    const uint8_t* payload = &sentinel;
    RecordInfo info{0xBEEF, 777};
    assertResult(StoreResult::InvalidArgument,
                 RecordCodec::decode(nullptr, kRecordSize, payload, info));
    TEST_ASSERT_EQUAL_PTR(&sentinel, payload);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, info.schemaVersion);
    TEST_ASSERT_EQUAL_size_t(777, info.payloadSize);
}

void test_persist_codec_decode_truncated(void) {
    uint8_t record[kRecordSize];
    makeRecord(record);
    assertDecodeCorrupt(record, 0);
    assertDecodeCorrupt(record, RecordCodec::kHeaderSize - 1);
    assertDecodeCorrupt(record, kRecordSize - 1);  // header intact, payload cut
}

void test_persist_codec_decode_bad_magic(void) {
    for (size_t i = 0; i < 4; ++i) {
        uint8_t record[kRecordSize];
        makeRecord(record);
        record[i] ^= 0x20;
        assertDecodeCorrupt(record, sizeof(record));
    }
}

void test_persist_codec_decode_bad_envelope_version(void) {
    uint8_t record[kRecordSize];
    makeRecord(record);
    record[4] = 2;
    assertDecodeCorrupt(record, sizeof(record));
    record[4] = 0;
    assertDecodeCorrupt(record, sizeof(record));
}

void test_persist_codec_decode_nonzero_reserved(void) {
    uint8_t record[kRecordSize];
    makeRecord(record);
    record[5] = 1;
    assertDecodeCorrupt(record, sizeof(record));
}

void test_persist_codec_decode_length_field_larger_than_buffer(void) {
    uint8_t record[kRecordSize];
    makeRecord(record);
    record[8] = 4;  // claims 4 payload bytes, only 3 present
    assertDecodeCorrupt(record, sizeof(record));
}

void test_persist_codec_decode_length_field_smaller_than_buffer(void) {
    uint8_t record[kRecordSize];
    makeRecord(record);
    record[8] = 2;  // claims 2 payload bytes, 3 present
    assertDecodeCorrupt(record, sizeof(record));

    // A valid record followed by one trailing byte is also rejected.
    uint8_t padded[kRecordSize + 1];
    encodeOk(0x1234, kPayload, sizeof(kPayload), padded, sizeof(padded));
    padded[kRecordSize] = 0;
    assertDecodeCorrupt(padded, sizeof(padded));
}

void test_persist_codec_decode_flipped_payload_bit(void) {
    for (size_t i = RecordCodec::kHeaderSize; i < kRecordSize; ++i) {
        uint8_t record[kRecordSize];
        makeRecord(record);
        record[i] ^= 0x01;
        assertDecodeCorrupt(record, sizeof(record));
    }
}

void test_persist_codec_decode_flipped_schema_byte(void) {
    // The schema version is not validated by itself; only the CRC over the
    // header catches a flip there.
    for (size_t i = 6; i <= 7; ++i) {
        uint8_t record[kRecordSize];
        makeRecord(record);
        record[i] ^= 0x80;
        assertDecodeCorrupt(record, sizeof(record));
    }
}

void test_persist_codec_decode_flipped_crc_byte(void) {
    for (size_t i = 12; i < RecordCodec::kHeaderSize; ++i) {
        uint8_t record[kRecordSize];
        makeRecord(record);
        record[i] ^= 0x04;
        assertDecodeCorrupt(record, sizeof(record));
    }
}

#else  // !PIXELROOT32_ENABLE_PERSISTENCE

void test_persist_codec_flag_off_stub(void) {
    // With the flag off, StoreTypes.h and RecordCodec.h declare nothing at
    // all. This translation unit compiling and passing without referencing any
    // of their symbols is the flags-off contract every existing game relies on.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PERSISTENCE=0: the persistence record codec is not "
        "compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_PERSISTENCE

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

#if PIXELROOT32_ENABLE_PERSISTENCE
    RUN_TEST(test_persist_store_result_values_are_distinct);
    RUN_TEST(test_persist_key_valid_examples);
    RUN_TEST(test_persist_key_invalid_examples);
    RUN_TEST(test_persist_key_scan_stops_after_limit);
    RUN_TEST(test_persist_crc32_check_value);
    RUN_TEST(test_persist_crc32_empty_input);
    RUN_TEST(test_persist_crc32_chaining_equals_one_shot);
    RUN_TEST(test_persist_codec_constants);
    RUN_TEST(test_persist_codec_known_record_byte_layout);
    RUN_TEST(test_persist_codec_round_trip);
    RUN_TEST(test_persist_codec_round_trip_empty_payload);
    RUN_TEST(test_persist_codec_reports_any_schema_version);
    RUN_TEST(test_persist_codec_encode_exact_capacity_succeeds);
    RUN_TEST(test_persist_codec_encode_capacity_one_short_is_too_large);
    RUN_TEST(test_persist_codec_encode_invalid_arguments);
    RUN_TEST(test_persist_codec_decode_null_input_is_invalid_argument);
    RUN_TEST(test_persist_codec_decode_truncated);
    RUN_TEST(test_persist_codec_decode_bad_magic);
    RUN_TEST(test_persist_codec_decode_bad_envelope_version);
    RUN_TEST(test_persist_codec_decode_nonzero_reserved);
    RUN_TEST(test_persist_codec_decode_length_field_larger_than_buffer);
    RUN_TEST(test_persist_codec_decode_length_field_smaller_than_buffer);
    RUN_TEST(test_persist_codec_decode_flipped_payload_bit);
    RUN_TEST(test_persist_codec_decode_flipped_schema_byte);
    RUN_TEST(test_persist_codec_decode_flipped_crc_byte);
#else
    RUN_TEST(test_persist_codec_flag_off_stub);
#endif

    return UNITY_END();
}
