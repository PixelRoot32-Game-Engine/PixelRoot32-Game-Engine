/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Adapter contract suite for the persistence store interfaces
 * (persistence/KeyValueStore.h, persistence/SlotStore.h).
 *
 * Every contract test reaches the store only through IKeyValueStore or
 * ISlotStore, so the same tests run unchanged against every adapter that can
 * run on the host. Adapters are registered in kKvAdapters / kSlotAdapters
 * below: each entry names the adapter, gives a factory that returns a fresh,
 * empty store, and states the limits the tests need. main() runs the whole
 * contract once per registered adapter, pointing gKv / gSlot at the current
 * entry; failure messages carry the adapter name.
 *
 * The memory-only tests at the end exercise the fault-injection hooks of
 * MemoryKeyValueStore / MemorySlotStore (failed writes, counters, byte
 * corruption), which are not part of the interfaces.
 *
 * The persistence module is compiled out unless PIXELROOT32_ENABLE_PERSISTENCE
 * is 1; under the flag off this file runs a single stub, as the codec suite
 * does.
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include "persistence/KeyValueStore.h"
#include "persistence/MemoryStores.h"
#include "persistence/SlotStore.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

using namespace pixelroot32::persistence;

namespace {

// ---------------------------------------------------------------------------
// Adapter registry
// ---------------------------------------------------------------------------

/** A key-value adapter under test. */
struct KvAdapter {
    const char* name;
    IKeyValueStore& (*fresh)();  ///< Returns an empty store; earlier contents are gone.
    size_t maxBlobSize;          ///< Largest blob setBlob() accepts.
    size_t entryLimit;           ///< Keys the store can hold; 0 = no fixed limit.
};

/** A slot adapter under test. */
struct SlotAdapter {
    const char* name;
    ISlotStore& (*fresh)();      ///< Returns a store with every slot empty.
    uint16_t slotCount;          ///< Expected slotCount().
    size_t maxRecordSize;        ///< Expected maxRecordSize().
};

constexpr size_t kMemEntries = 4;
constexpr size_t kMemBlobBytes = 32;
constexpr uint16_t kMemSlots = 3;
constexpr size_t kMemRecordBytes = 32;

using MemKv = MemoryKeyValueStore<kMemEntries, kMemBlobBytes>;
using MemSlots = MemorySlotStore<kMemSlots, kMemRecordBytes>;

MemKv gMemKv;
MemSlots gMemSlots;

IKeyValueStore& freshMemoryKv() {
    gMemKv = MemKv{};
    return gMemKv;
}

ISlotStore& freshMemorySlots() {
    gMemSlots = MemSlots{};
    return gMemSlots;
}

// To add an adapter: write a factory returning a fresh store and append one
// row here. No test needs to change.
const KvAdapter kKvAdapters[] = {
    {"MemoryKeyValueStore", &freshMemoryKv, kMemBlobBytes, kMemEntries},
};

const SlotAdapter kSlotAdapters[] = {
    {"MemorySlotStore", &freshMemorySlots, kMemSlots, kMemRecordBytes},
};

const KvAdapter* gKv = nullptr;
const SlotAdapter* gSlot = nullptr;
const char* gAdapterName = "";

constexpr uint8_t kGuard = 0x5A;
constexpr size_t kMaxTestBytes = 64;

#define CHECK_RESULT(expected, actual)                                          \
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),             \
                                    static_cast<uint8_t>(actual), gAdapterName)
#define CHECK_SIZE(expected, actual)                                            \
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(static_cast<uint32_t>(expected),           \
                                     static_cast<uint32_t>(actual), gAdapterName)

void fillPattern(uint8_t* data, size_t size, uint8_t seed) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(seed + i * 7u);
    }
}

void assertAllGuard(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        TEST_ASSERT_EQUAL_HEX8_MESSAGE(kGuard, data[i], gAdapterName);
    }
}

// Reads a blob into a guarded buffer and checks bytes, size and guards.
void assertBlobEquals(IKeyValueStore& kv, const char* key, const uint8_t* expected,
                      size_t size) {
    uint8_t buf[kMaxTestBytes + 1];
    TEST_ASSERT_TRUE(size <= kMaxTestBytes);
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 9999;
    CHECK_RESULT(StoreResult::Ok, kv.getBlob(key, buf, size, stored));
    CHECK_SIZE(size, stored);
    if (size > 0) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(expected, buf, size, gAdapterName);
    }
    assertAllGuard(buf + size, sizeof(buf) - size);
}

void assertSlotEquals(ISlotStore& s, uint16_t slot, const uint8_t* expected, size_t size) {
    uint8_t buf[kMaxTestBytes + 1];
    TEST_ASSERT_TRUE(size <= kMaxTestBytes);
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 9999;
    CHECK_RESULT(StoreResult::Ok, s.read(slot, buf, size, stored));
    CHECK_SIZE(size, stored);
    if (size > 0) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(expected, buf, size, gAdapterName);
    }
    assertAllGuard(buf + size, sizeof(buf) - size);
}

const char* const kInvalidKeys[] = {
    nullptr, "", "abcdefghijklmnop" /* 16 bytes */, "a b", "a/b", "a.b", "a:b",
};

}  // namespace

// ---------------------------------------------------------------------------
// IKeyValueStore contract (S4, S5)
// ---------------------------------------------------------------------------

void test_kv_i32_round_trip(void) {
    IKeyValueStore& kv = gKv->fresh();
    const int32_t values[] = {0, -1, 123456, INT32_MIN, INT32_MAX};
    for (int32_t v : values) {
        CHECK_RESULT(StoreResult::Ok, kv.setI32("i", v));
        int32_t out = 7;
        CHECK_RESULT(StoreResult::Ok, kv.getI32("i", out));
        TEST_ASSERT_EQUAL_INT32_MESSAGE(v, out, gAdapterName);
    }
}

void test_kv_u32_round_trip(void) {
    IKeyValueStore& kv = gKv->fresh();
    const uint32_t values[] = {0u, 1u, 0x80000000u, 0xFFFFFFFFu};
    for (uint32_t v : values) {
        CHECK_RESULT(StoreResult::Ok, kv.setU32("u", v));
        uint32_t out = 7;
        CHECK_RESULT(StoreResult::Ok, kv.getU32("u", out));
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(v, out, gAdapterName);
    }
}

void test_kv_blob_round_trip(void) {
    IKeyValueStore& kv = gKv->fresh();
    uint8_t data[kMaxTestBytes];
    fillPattern(data, gKv->maxBlobSize, 3);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("full", data, gKv->maxBlobSize));
    assertBlobEquals(kv, "full", data, gKv->maxBlobSize);

    CHECK_RESULT(StoreResult::Ok, kv.setBlob("one", data, 1));
    assertBlobEquals(kv, "one", data, 1);
}

void test_kv_blob_round_trip_empty(void) {
    IKeyValueStore& kv = gKv->fresh();
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("e1", nullptr, 0));
    const uint8_t any = 1;
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("e2", &any, 0));

    size_t stored = 9999;
    CHECK_RESULT(StoreResult::Ok, kv.getBlob("e1", nullptr, 0, stored));
    CHECK_SIZE(0, stored);
    assertBlobEquals(kv, "e2", nullptr, 0);
}

void test_kv_blob_read_into_larger_buffer(void) {
    IKeyValueStore& kv = gKv->fresh();
    uint8_t data[8];
    fillPattern(data, sizeof(data), 11);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", data, sizeof(data)));

    uint8_t buf[20];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 9999;
    CHECK_RESULT(StoreResult::Ok, kv.getBlob("b", buf, 16, stored));
    CHECK_SIZE(sizeof(data), stored);
    TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(data, buf, sizeof(data), gAdapterName);
    // Nothing past the stored size is written, even inside the capacity.
    assertAllGuard(buf + sizeof(data), sizeof(buf) - sizeof(data));
}

void test_kv_overwrite_same_type(void) {
    IKeyValueStore& kv = gKv->fresh();
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 1));
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", -2));
    int32_t i = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getI32("k", i));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(-2, i, gAdapterName);

    CHECK_RESULT(StoreResult::Ok, kv.setU32("u", 5));
    CHECK_RESULT(StoreResult::Ok, kv.setU32("u", 6));
    uint32_t u = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getU32("u", u));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(6u, u, gAdapterName);

    uint8_t big[12];
    uint8_t small[3];
    fillPattern(big, sizeof(big), 1);
    fillPattern(small, sizeof(small), 90);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", big, sizeof(big)));
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", small, sizeof(small)));
    assertBlobEquals(kv, "b", small, sizeof(small));
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", big, sizeof(big)));
    assertBlobEquals(kv, "b", big, sizeof(big));
}

void test_kv_overwrite_other_type_last_write_wins(void) {
    IKeyValueStore& kv = gKv->fresh();
    const uint8_t blob[3] = {9, 8, 7};
    int32_t i = 42;
    uint32_t u = 43;
    uint8_t buf[4] = {kGuard, kGuard, kGuard, kGuard};
    size_t stored = 77;

    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", -5));
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("k", blob, sizeof(blob)));
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("k", i));
    CHECK_RESULT(StoreResult::NotFound, kv.getU32("k", u));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(42, i, gAdapterName);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(43u, u, gAdapterName);
    assertBlobEquals(kv, "k", blob, sizeof(blob));

    CHECK_RESULT(StoreResult::Ok, kv.setU32("k", 0xDEADBEEFu));
    CHECK_RESULT(StoreResult::NotFound, kv.getBlob("k", buf, sizeof(buf), stored));
    CHECK_SIZE(77, stored);
    assertAllGuard(buf, sizeof(buf));
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("k", i));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(42, i, gAdapterName);
    CHECK_RESULT(StoreResult::Ok, kv.getU32("k", u));
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(0xDEADBEEFu, u, gAdapterName);

    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 17));
    u = 43;
    CHECK_RESULT(StoreResult::NotFound, kv.getU32("k", u));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(43u, u, gAdapterName);
    CHECK_RESULT(StoreResult::Ok, kv.getI32("k", i));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(17, i, gAdapterName);
}

void test_kv_independent_keys(void) {
    IKeyValueStore& kv = gKv->fresh();
    CHECK_RESULT(StoreResult::Ok, kv.setI32("a", 1));
    CHECK_RESULT(StoreResult::Ok, kv.setI32("b", 2));
    CHECK_RESULT(StoreResult::Ok, kv.setI32("ab", 3));
    int32_t v = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getI32("a", v));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(1, v, gAdapterName);
    CHECK_RESULT(StoreResult::Ok, kv.getI32("b", v));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(2, v, gAdapterName);
    CHECK_RESULT(StoreResult::Ok, kv.getI32("ab", v));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(3, v, gAdapterName);
}

void test_kv_keys_are_case_sensitive_and_use_full_length(void) {
    IKeyValueStore& kv = gKv->fresh();
    CHECK_RESULT(StoreResult::Ok, kv.setI32("abcdefghijklmno", 1));  // 15 bytes
    CHECK_RESULT(StoreResult::Ok, kv.setI32("abcdefghijklmnO", 2));
    int32_t v = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getI32("abcdefghijklmno", v));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(1, v, gAdapterName);
    CHECK_RESULT(StoreResult::Ok, kv.getI32("abcdefghijklmnO", v));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(2, v, gAdapterName);
}

void test_kv_missing_key_not_found_outputs_untouched(void) {
    IKeyValueStore& kv = gKv->fresh();
    CHECK_RESULT(StoreResult::Ok, kv.setI32("other", 1));
    int32_t i = 1234;
    uint32_t u = 5678;
    uint8_t buf[8];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 4321;

    CHECK_RESULT(StoreResult::NotFound, kv.getI32("missing", i));
    CHECK_RESULT(StoreResult::NotFound, kv.getU32("missing", u));
    CHECK_RESULT(StoreResult::NotFound, kv.getBlob("missing", buf, sizeof(buf), stored));
    CHECK_RESULT(StoreResult::NotFound, kv.getBlob("missing", nullptr, 0, stored));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(1234, i, gAdapterName);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5678u, u, gAdapterName);
    CHECK_SIZE(4321, stored);
    assertAllGuard(buf, sizeof(buf));
}

void test_kv_invalid_key_every_method(void) {
    IKeyValueStore& kv = gKv->fresh();
    const uint8_t data[2] = {1, 2};
    for (const char* key : kInvalidKeys) {
        int32_t i = 11;
        uint32_t u = 12;
        uint8_t buf[4];
        memset(buf, kGuard, sizeof(buf));
        size_t stored = 13;
        CHECK_RESULT(StoreResult::InvalidKey, kv.setI32(key, 1));
        CHECK_RESULT(StoreResult::InvalidKey, kv.getI32(key, i));
        CHECK_RESULT(StoreResult::InvalidKey, kv.setU32(key, 1));
        CHECK_RESULT(StoreResult::InvalidKey, kv.getU32(key, u));
        CHECK_RESULT(StoreResult::InvalidKey, kv.setBlob(key, data, sizeof(data)));
        CHECK_RESULT(StoreResult::InvalidKey, kv.getBlob(key, buf, sizeof(buf), stored));
        CHECK_RESULT(StoreResult::InvalidKey, kv.erase(key));
        TEST_ASSERT_EQUAL_INT32_MESSAGE(11, i, gAdapterName);
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(12u, u, gAdapterName);
        CHECK_SIZE(13, stored);
        assertAllGuard(buf, sizeof(buf));
    }
    // The 15-byte prefix of the overlong key was not stored either.
    int32_t i = 0;
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("abcdefghijklmno", i));
}

void test_kv_blob_too_large_for_capacity_writes_nothing(void) {
    IKeyValueStore& kv = gKv->fresh();
    uint8_t data[8];
    fillPattern(data, sizeof(data), 5);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", data, sizeof(data)));

    uint8_t buf[sizeof(data)];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 0;
    CHECK_RESULT(StoreResult::TooLarge, kv.getBlob("b", buf, sizeof(data) - 1, stored));
    CHECK_SIZE(sizeof(data), stored);
    assertAllGuard(buf, sizeof(buf));

    // Exact capacity succeeds; the byte after it stays a guard.
    assertBlobEquals(kv, "b", data, sizeof(data));
}

void test_kv_blob_size_query(void) {
    IKeyValueStore& kv = gKv->fresh();
    uint8_t data[6];
    fillPattern(data, sizeof(data), 2);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", data, sizeof(data)));

    size_t stored = 0;
    CHECK_RESULT(StoreResult::TooLarge, kv.getBlob("b", nullptr, 0, stored));
    CHECK_SIZE(sizeof(data), stored);

    uint8_t buf[4];
    memset(buf, kGuard, sizeof(buf));
    stored = 0;
    CHECK_RESULT(StoreResult::TooLarge, kv.getBlob("b", buf, 0, stored));
    CHECK_SIZE(sizeof(data), stored);
    assertAllGuard(buf, sizeof(buf));
}

void test_kv_blob_invalid_arguments(void) {
    IKeyValueStore& kv = gKv->fresh();
    const uint8_t data[4] = {1, 2, 3, 4};
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", data, sizeof(data)));

    size_t stored = 99;
    CHECK_RESULT(StoreResult::InvalidArgument, kv.getBlob("b", nullptr, 4, stored));
    CHECK_SIZE(99, stored);

    CHECK_RESULT(StoreResult::InvalidArgument, kv.setBlob("b", nullptr, 2));
    CHECK_RESULT(StoreResult::InvalidArgument, kv.setBlob("new", nullptr, 1));
    assertBlobEquals(kv, "b", data, sizeof(data));
    CHECK_RESULT(StoreResult::NotFound, kv.getBlob("new", nullptr, 0, stored));
}

void test_kv_blob_above_store_max_is_too_large(void) {
    IKeyValueStore& kv = gKv->fresh();
    uint8_t data[kMaxTestBytes + 1];
    TEST_ASSERT_TRUE(gKv->maxBlobSize < sizeof(data));
    fillPattern(data, sizeof(data), 4);
    const uint8_t old[2] = {7, 7};
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", old, sizeof(old)));

    CHECK_RESULT(StoreResult::TooLarge, kv.setBlob("b", data, gKv->maxBlobSize + 1));
    CHECK_RESULT(StoreResult::TooLarge, kv.setBlob("new", data, gKv->maxBlobSize + 1));
    assertBlobEquals(kv, "b", old, sizeof(old));
    size_t stored = 0;
    CHECK_RESULT(StoreResult::NotFound, kv.getBlob("new", nullptr, 0, stored));
}

void test_kv_erase(void) {
    IKeyValueStore& kv = gKv->fresh();
    const uint8_t data[1] = {1};
    CHECK_RESULT(StoreResult::Ok, kv.setI32("i", 1));
    CHECK_RESULT(StoreResult::Ok, kv.setU32("u", 2));
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", data, sizeof(data)));

    CHECK_RESULT(StoreResult::Ok, kv.erase("i"));
    CHECK_RESULT(StoreResult::Ok, kv.erase("u"));
    CHECK_RESULT(StoreResult::Ok, kv.erase("b"));

    int32_t i = 5;
    uint32_t u = 6;
    size_t stored = 7;
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("i", i));
    CHECK_RESULT(StoreResult::NotFound, kv.getU32("u", u));
    CHECK_RESULT(StoreResult::NotFound, kv.getBlob("b", nullptr, 0, stored));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(5, i, gAdapterName);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(6u, u, gAdapterName);
    CHECK_SIZE(7, stored);

    CHECK_RESULT(StoreResult::NotFound, kv.erase("i"));
    CHECK_RESULT(StoreResult::NotFound, kv.erase("never"));
}

void test_kv_no_space_when_full(void) {
    if (gKv->entryLimit == 0) {
        return;  // This adapter has no fixed key limit; nothing to fill.
    }
    IKeyValueStore& kv = gKv->fresh();
    char key[4] = {'k', '0', '0', '\0'};
    for (size_t n = 0; n < gKv->entryLimit; ++n) {
        key[1] = static_cast<char>('0' + n / 10);
        key[2] = static_cast<char>('0' + n % 10);
        CHECK_RESULT(StoreResult::Ok, kv.setI32(key, static_cast<int32_t>(n)));
    }
    const uint8_t data[2] = {1, 2};
    CHECK_RESULT(StoreResult::NoSpace, kv.setI32("extra", 1));
    CHECK_RESULT(StoreResult::NoSpace, kv.setU32("extra", 1));
    CHECK_RESULT(StoreResult::NoSpace, kv.setBlob("extra", data, sizeof(data)));
    int32_t i = 0;
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("extra", i));

    // Updating an existing key, even to another type, still works when full.
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k00", 100));
    CHECK_RESULT(StoreResult::Ok, kv.getI32("k00", i));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(100, i, gAdapterName);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("k01", data, sizeof(data)));
    assertBlobEquals(kv, "k01", data, sizeof(data));

    // Erasing frees an entry for a new key.
    CHECK_RESULT(StoreResult::Ok, kv.erase("k00"));
    CHECK_RESULT(StoreResult::Ok, kv.setI32("extra", 7));
    CHECK_RESULT(StoreResult::Ok, kv.getI32("extra", i));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(7, i, gAdapterName);
}

void test_kv_commit_ok_and_data_kept(void) {
    IKeyValueStore& kv = gKv->fresh();
    CHECK_RESULT(StoreResult::Ok, kv.commit());
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 3));
    CHECK_RESULT(StoreResult::Ok, kv.commit());
    int32_t v = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getI32("k", v));
    TEST_ASSERT_EQUAL_INT32_MESSAGE(3, v, gAdapterName);
}

// ---------------------------------------------------------------------------
// ISlotStore contract (S5, S6)
// ---------------------------------------------------------------------------

void test_slot_geometry(void) {
    ISlotStore& s = gSlot->fresh();
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(gSlot->slotCount, s.slotCount(), gAdapterName);
    CHECK_SIZE(gSlot->maxRecordSize, s.maxRecordSize());
}

void test_slot_empty_is_not_found_with_zero_size(void) {
    ISlotStore& s = gSlot->fresh();
    for (uint16_t slot = 0; slot < s.slotCount(); ++slot) {
        size_t stored = 99;
        CHECK_RESULT(StoreResult::NotFound, s.query(slot, stored));
        CHECK_SIZE(0, stored);

        uint8_t buf[8];
        memset(buf, kGuard, sizeof(buf));
        stored = 99;
        CHECK_RESULT(StoreResult::NotFound, s.read(slot, buf, sizeof(buf), stored));
        CHECK_SIZE(0, stored);
        assertAllGuard(buf, sizeof(buf));

        stored = 99;
        CHECK_RESULT(StoreResult::NotFound, s.read(slot, nullptr, 0, stored));
        CHECK_SIZE(0, stored);
    }
}

void test_slot_round_trip(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t data[kMaxTestBytes];
    const size_t max = s.maxRecordSize();
    fillPattern(data, max, 21);
    CHECK_RESULT(StoreResult::Ok, s.write(0, data, max));
    assertSlotEquals(s, 0, data, max);

    const uint16_t last = static_cast<uint16_t>(s.slotCount() - 1);
    CHECK_RESULT(StoreResult::Ok, s.write(last, data, 5));
    assertSlotEquals(s, last, data, 5);
}

void test_slot_round_trip_empty_record(void) {
    ISlotStore& s = gSlot->fresh();
    CHECK_RESULT(StoreResult::Ok, s.write(0, nullptr, 0));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::Ok, s.query(0, stored));
    CHECK_SIZE(0, stored);
    stored = 99;
    CHECK_RESULT(StoreResult::Ok, s.read(0, nullptr, 0, stored));
    CHECK_SIZE(0, stored);
    assertSlotEquals(s, 0, nullptr, 0);
}

void test_slot_query_reports_size(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t data[9];
    fillPattern(data, sizeof(data), 1);
    CHECK_RESULT(StoreResult::Ok, s.write(1, data, sizeof(data)));
    size_t stored = 0;
    CHECK_RESULT(StoreResult::Ok, s.query(1, stored));
    CHECK_SIZE(sizeof(data), stored);
}

void test_slot_out_of_range_every_method(void) {
    ISlotStore& s = gSlot->fresh();
    const uint8_t data[2] = {1, 2};
    const uint16_t bad[] = {s.slotCount(), static_cast<uint16_t>(s.slotCount() + 1), 0xFFFF};
    for (uint16_t slot : bad) {
        size_t stored = 55;
        uint8_t buf[4];
        memset(buf, kGuard, sizeof(buf));
        CHECK_RESULT(StoreResult::InvalidArgument, s.query(slot, stored));
        CHECK_RESULT(StoreResult::InvalidArgument, s.read(slot, buf, sizeof(buf), stored));
        CHECK_RESULT(StoreResult::InvalidArgument, s.write(slot, data, sizeof(data)));
        CHECK_RESULT(StoreResult::InvalidArgument, s.erase(slot));
        CHECK_SIZE(55, stored);
        assertAllGuard(buf, sizeof(buf));
    }
}

void test_slot_too_large_for_capacity_writes_nothing(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t data[10];
    fillPattern(data, sizeof(data), 8);
    CHECK_RESULT(StoreResult::Ok, s.write(0, data, sizeof(data)));

    uint8_t buf[sizeof(data)];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 0;
    CHECK_RESULT(StoreResult::TooLarge, s.read(0, buf, sizeof(data) - 1, stored));
    CHECK_SIZE(sizeof(data), stored);
    assertAllGuard(buf, sizeof(buf));

    assertSlotEquals(s, 0, data, sizeof(data));
}

void test_slot_size_query(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t data[7];
    fillPattern(data, sizeof(data), 8);
    CHECK_RESULT(StoreResult::Ok, s.write(2, data, sizeof(data)));

    size_t stored = 0;
    CHECK_RESULT(StoreResult::TooLarge, s.read(2, nullptr, 0, stored));
    CHECK_SIZE(sizeof(data), stored);

    uint8_t buf[3];
    memset(buf, kGuard, sizeof(buf));
    stored = 0;
    CHECK_RESULT(StoreResult::TooLarge, s.read(2, buf, 0, stored));
    CHECK_SIZE(sizeof(data), stored);
    assertAllGuard(buf, sizeof(buf));
}

void test_slot_invalid_arguments(void) {
    ISlotStore& s = gSlot->fresh();
    const uint8_t data[3] = {1, 2, 3};
    CHECK_RESULT(StoreResult::Ok, s.write(0, data, sizeof(data)));

    size_t stored = 99;
    CHECK_RESULT(StoreResult::InvalidArgument, s.read(0, nullptr, 3, stored));
    CHECK_SIZE(99, stored);
    CHECK_RESULT(StoreResult::InvalidArgument, s.write(0, nullptr, 1));
    CHECK_RESULT(StoreResult::InvalidArgument, s.write(1, nullptr, 1));
    assertSlotEquals(s, 0, data, sizeof(data));
    CHECK_RESULT(StoreResult::NotFound, s.query(1, stored));
}

void test_slot_above_max_record_is_too_large(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t data[kMaxTestBytes + 1];
    TEST_ASSERT_TRUE(s.maxRecordSize() < sizeof(data));
    fillPattern(data, sizeof(data), 3);
    const uint8_t old[2] = {4, 4};
    CHECK_RESULT(StoreResult::Ok, s.write(0, old, sizeof(old)));

    CHECK_RESULT(StoreResult::TooLarge, s.write(0, data, s.maxRecordSize() + 1));
    CHECK_RESULT(StoreResult::TooLarge, s.write(1, data, s.maxRecordSize() + 1));
    assertSlotEquals(s, 0, old, sizeof(old));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::NotFound, s.query(1, stored));
}

void test_slot_overwrite_replaces_record(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t big[16];
    uint8_t small[4];
    fillPattern(big, sizeof(big), 1);
    fillPattern(small, sizeof(small), 200);
    CHECK_RESULT(StoreResult::Ok, s.write(0, big, sizeof(big)));
    CHECK_RESULT(StoreResult::Ok, s.write(0, small, sizeof(small)));
    assertSlotEquals(s, 0, small, sizeof(small));
    CHECK_RESULT(StoreResult::Ok, s.write(0, big, sizeof(big)));
    assertSlotEquals(s, 0, big, sizeof(big));
}

void test_slot_slots_are_independent(void) {
    ISlotStore& s = gSlot->fresh();
    uint8_t data[8];
    for (uint16_t slot = 0; slot < s.slotCount(); ++slot) {
        fillPattern(data, sizeof(data), static_cast<uint8_t>(slot * 40));
        CHECK_RESULT(StoreResult::Ok, s.write(slot, data, sizeof(data) - slot % sizeof(data)));
    }
    for (uint16_t slot = 0; slot < s.slotCount(); ++slot) {
        fillPattern(data, sizeof(data), static_cast<uint8_t>(slot * 40));
        assertSlotEquals(s, slot, data, sizeof(data) - slot % sizeof(data));
    }
    CHECK_RESULT(StoreResult::Ok, s.erase(0));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::NotFound, s.query(0, stored));
    fillPattern(data, sizeof(data), 40);
    assertSlotEquals(s, 1, data, sizeof(data) - 1);
}

void test_slot_erase(void) {
    ISlotStore& s = gSlot->fresh();
    const uint8_t data[2] = {1, 2};
    CHECK_RESULT(StoreResult::Ok, s.write(0, data, sizeof(data)));
    CHECK_RESULT(StoreResult::Ok, s.erase(0));
    size_t stored = 99;
    uint8_t buf[2] = {kGuard, kGuard};
    CHECK_RESULT(StoreResult::NotFound, s.read(0, buf, sizeof(buf), stored));
    CHECK_SIZE(0, stored);
    assertAllGuard(buf, sizeof(buf));
    CHECK_RESULT(StoreResult::NotFound, s.erase(0));
    CHECK_RESULT(StoreResult::NotFound, s.erase(1));
}

// ---------------------------------------------------------------------------
// Memory adapter hooks (S7, S10)
// ---------------------------------------------------------------------------

void test_mem_kv_failed_writes_keep_previous_value(void) {
    gAdapterName = "MemoryKeyValueStore";
    MemKv kv;
    const uint8_t blob[3] = {1, 2, 3};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(StoreResult::Ok),
                            static_cast<uint8_t>(kv.setI32("i", 1)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(StoreResult::Ok),
                            static_cast<uint8_t>(kv.setBlob("b", blob, sizeof(blob))));
    const uint32_t writesBefore = kv.writes();
    const uint32_t commitsBefore = kv.commits();

    kv.setFailWrites(true);
    const uint8_t other[2] = {9, 9};
    CHECK_RESULT(StoreResult::IoError, kv.setI32("i", 2));
    CHECK_RESULT(StoreResult::IoError, kv.setU32("i", 2));
    CHECK_RESULT(StoreResult::IoError, kv.setBlob("i", other, sizeof(other)));
    CHECK_RESULT(StoreResult::IoError, kv.setBlob("b", other, sizeof(other)));
    CHECK_RESULT(StoreResult::IoError, kv.setI32("new", 5));
    CHECK_RESULT(StoreResult::IoError, kv.erase("b"));
    CHECK_RESULT(StoreResult::IoError, kv.commit());
    // Argument checks still come first.
    CHECK_RESULT(StoreResult::InvalidKey, kv.setI32("a b", 1));
    // Reads keep working while writes fail.
    int32_t i = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getI32("i", i));
    TEST_ASSERT_EQUAL_INT32(1, i);
    assertBlobEquals(kv, "b", blob, sizeof(blob));
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("new", i));
    TEST_ASSERT_EQUAL_UINT32(writesBefore, kv.writes());
    TEST_ASSERT_EQUAL_UINT32(commitsBefore, kv.commits());

    kv.setFailWrites(false);
    CHECK_RESULT(StoreResult::Ok, kv.setI32("i", 2));
    CHECK_RESULT(StoreResult::Ok, kv.getI32("i", i));
    TEST_ASSERT_EQUAL_INT32(2, i);
}

void test_mem_kv_counters(void) {
    MemKv kv;
    TEST_ASSERT_EQUAL_UINT32(0u, kv.writes());
    TEST_ASSERT_EQUAL_UINT32(0u, kv.commits());
    const uint8_t blob[1] = {1};
    kv.setI32("a", 1);
    kv.setU32("b", 2);
    kv.setBlob("c", blob, sizeof(blob));
    kv.erase("a");
    TEST_ASSERT_EQUAL_UINT32(4u, kv.writes());
    // Reads, rejected calls and erasing a missing key are not writes.
    int32_t i = 0;
    kv.getI32("b", i);
    kv.erase("a");
    kv.setI32("a b", 1);
    kv.setBlob("big", blob, kMemBlobBytes + 1);
    TEST_ASSERT_EQUAL_UINT32(4u, kv.writes());
    kv.commit();
    kv.commit();
    TEST_ASSERT_EQUAL_UINT32(2u, kv.commits());
}

void test_mem_kv_corrupt_byte(void) {
    gAdapterName = "MemoryKeyValueStore";
    MemKv kv;
    uint8_t data[5] = {10, 20, 30, 40, 50};
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", data, sizeof(data)));
    CHECK_RESULT(StoreResult::Ok, kv.corruptByte("b", 2));
    data[2] = static_cast<uint8_t>(data[2] ^ 0xFFu);
    assertBlobEquals(kv, "b", data, sizeof(data));

    CHECK_RESULT(StoreResult::Ok, kv.setU32("u", 0x01020304u));
    CHECK_RESULT(StoreResult::Ok, kv.corruptByte("u", 0));  // ints are little-endian
    uint32_t u = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getU32("u", u));
    TEST_ASSERT_EQUAL_HEX32(0x010203FBu, u);

    CHECK_RESULT(StoreResult::InvalidArgument, kv.corruptByte("b", sizeof(data)));
    CHECK_RESULT(StoreResult::InvalidArgument, kv.corruptByte("u", 4));
    CHECK_RESULT(StoreResult::NotFound, kv.corruptByte("missing", 0));
    CHECK_RESULT(StoreResult::InvalidKey, kv.corruptByte("a b", 0));
}

void test_mem_slot_failed_writes_keep_previous_record(void) {
    gAdapterName = "MemorySlotStore";
    MemSlots s;
    const uint8_t old[4] = {1, 2, 3, 4};
    const uint8_t neu[6] = {9, 9, 9, 9, 9, 9};
    CHECK_RESULT(StoreResult::Ok, s.write(0, old, sizeof(old)));
    const uint32_t writesBefore = s.writes();

    s.setFailWrites(true);
    CHECK_RESULT(StoreResult::IoError, s.write(0, neu, sizeof(neu)));
    CHECK_RESULT(StoreResult::IoError, s.write(1, neu, sizeof(neu)));
    CHECK_RESULT(StoreResult::IoError, s.erase(0));
    CHECK_RESULT(StoreResult::InvalidArgument, s.write(kMemSlots, neu, sizeof(neu)));
    assertSlotEquals(s, 0, old, sizeof(old));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::NotFound, s.query(1, stored));
    TEST_ASSERT_EQUAL_UINT32(writesBefore, s.writes());

    s.setFailWrites(false);
    CHECK_RESULT(StoreResult::Ok, s.write(0, neu, sizeof(neu)));
    assertSlotEquals(s, 0, neu, sizeof(neu));
    TEST_ASSERT_EQUAL_UINT32(writesBefore + 1, s.writes());
}

void test_mem_slot_corrupt_byte(void) {
    gAdapterName = "MemorySlotStore";
    MemSlots s;
    uint8_t data[3] = {7, 8, 9};
    CHECK_RESULT(StoreResult::Ok, s.write(1, data, sizeof(data)));
    CHECK_RESULT(StoreResult::Ok, s.corruptByte(1, 0));
    data[0] = static_cast<uint8_t>(data[0] ^ 0xFFu);
    assertSlotEquals(s, 1, data, sizeof(data));
    CHECK_RESULT(StoreResult::InvalidArgument, s.corruptByte(1, sizeof(data)));
    CHECK_RESULT(StoreResult::NotFound, s.corruptByte(0, 0));
    CHECK_RESULT(StoreResult::InvalidArgument, s.corruptByte(kMemSlots, 0));
}

#else  // !PIXELROOT32_ENABLE_PERSISTENCE

void test_persist_contract_flag_off_stub(void) {
    // With the flag off the store interfaces and memory adapters declare
    // nothing; this translation unit compiling without them is the contract.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PERSISTENCE=0: the persistence stores are not "
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
    for (const KvAdapter& adapter : kKvAdapters) {
        gKv = &adapter;
        gAdapterName = adapter.name;
        RUN_TEST(test_kv_i32_round_trip);
        RUN_TEST(test_kv_u32_round_trip);
        RUN_TEST(test_kv_blob_round_trip);
        RUN_TEST(test_kv_blob_round_trip_empty);
        RUN_TEST(test_kv_blob_read_into_larger_buffer);
        RUN_TEST(test_kv_overwrite_same_type);
        RUN_TEST(test_kv_overwrite_other_type_last_write_wins);
        RUN_TEST(test_kv_independent_keys);
        RUN_TEST(test_kv_keys_are_case_sensitive_and_use_full_length);
        RUN_TEST(test_kv_missing_key_not_found_outputs_untouched);
        RUN_TEST(test_kv_invalid_key_every_method);
        RUN_TEST(test_kv_blob_too_large_for_capacity_writes_nothing);
        RUN_TEST(test_kv_blob_size_query);
        RUN_TEST(test_kv_blob_invalid_arguments);
        RUN_TEST(test_kv_blob_above_store_max_is_too_large);
        RUN_TEST(test_kv_erase);
        RUN_TEST(test_kv_no_space_when_full);
        RUN_TEST(test_kv_commit_ok_and_data_kept);
    }
    for (const SlotAdapter& adapter : kSlotAdapters) {
        gSlot = &adapter;
        gAdapterName = adapter.name;
        RUN_TEST(test_slot_geometry);
        RUN_TEST(test_slot_empty_is_not_found_with_zero_size);
        RUN_TEST(test_slot_round_trip);
        RUN_TEST(test_slot_round_trip_empty_record);
        RUN_TEST(test_slot_query_reports_size);
        RUN_TEST(test_slot_out_of_range_every_method);
        RUN_TEST(test_slot_too_large_for_capacity_writes_nothing);
        RUN_TEST(test_slot_size_query);
        RUN_TEST(test_slot_invalid_arguments);
        RUN_TEST(test_slot_above_max_record_is_too_large);
        RUN_TEST(test_slot_overwrite_replaces_record);
        RUN_TEST(test_slot_slots_are_independent);
        RUN_TEST(test_slot_erase);
    }
    RUN_TEST(test_mem_kv_failed_writes_keep_previous_value);
    RUN_TEST(test_mem_kv_counters);
    RUN_TEST(test_mem_kv_corrupt_byte);
    RUN_TEST(test_mem_slot_failed_writes_keep_previous_record);
    RUN_TEST(test_mem_slot_corrupt_byte);
#else
    RUN_TEST(test_persist_contract_flag_off_stub);
#endif

    return UNITY_END();
}
