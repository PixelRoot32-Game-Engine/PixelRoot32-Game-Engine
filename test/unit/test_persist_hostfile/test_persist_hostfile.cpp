/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * HostFile-only tests for the native file adapters
 * (drivers/native/HostFileStore.h): behaviour the shared contract suite cannot
 * see through IKeyValueStore / ISlotStore - init() and its argument checks,
 * Unavailable before init(), data surviving a new store object over the same
 * root (the "reboot"), the temp-file-and-replace write path under a simulated
 * crash, stale temp files, case-sensitive keys on a case-insensitive file
 * system, damaged files and the on-disk layout.
 *
 * Every test works under one unique directory in the system temp directory,
 * never inside the repository; main() removes that tree when the suite ends.
 *
 * The persistence module is compiled out unless PIXELROOT32_ENABLE_PERSISTENCE
 * is 1; under the flag off this file runs a single stub, as the other
 * persistence suites do.
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include "drivers/native/HostFileStore.h"
#include "persistence/StoreTypes.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>

using namespace pixelroot32::persistence;
namespace fs = std::filesystem;

namespace {

constexpr size_t kBlobBytes = 32;
constexpr uint16_t kSlots = 3;
constexpr size_t kRecordBytes = 32;
constexpr uint8_t kGuard = 0x5A;

std::string gBase;  // Unique scratch root of this run; removed by main().

#define CHECK_RESULT(expected, actual)                              \
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),         \
                            static_cast<uint8_t>(actual))

std::string makeBase() {
    std::error_code ec;
    fs::path tmp = fs::temp_directory_path(ec);
    if (ec) {
        tmp = fs::path(".");
    }
    std::random_device rd;
    const auto now = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    char name[64];
    snprintf(name, sizeof(name), "pr32_persist_hostfile_%08x_%llx", rd(), now);
    return (tmp / name).generic_string();
}

/** A fresh root directory (not yet created) under the run's scratch root. */
std::string root(const char* name) {
    return gBase + "/" + name;
}

bool isDirectory(const std::string& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

void writeRaw(const std::string& path, const uint8_t* data, size_t size) {
    FILE* f = fopen(path.c_str(), "wb");
    TEST_ASSERT_NOT_NULL(f);
    if (size > 0) {
        TEST_ASSERT_EQUAL_UINT32(size, fwrite(data, 1, size, f));
    }
    TEST_ASSERT_EQUAL_INT(0, fclose(f));
}

void truncateTo(const std::string& path, size_t size) {
    std::error_code ec;
    fs::resize_file(path, size, ec);
    TEST_ASSERT_FALSE(ec);
}

void assertBlob(IKeyValueStore& kv, const char* key, const uint8_t* expected, size_t size) {
    uint8_t buf[kBlobBytes + 1];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 9999;
    CHECK_RESULT(StoreResult::Ok, kv.getBlob(key, buf, size, stored));
    TEST_ASSERT_EQUAL_UINT32(size, stored);
    if (size > 0) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buf, size);
    }
    TEST_ASSERT_EQUAL_HEX8(kGuard, buf[size]);
}

void assertSlot(ISlotStore& s, uint16_t slot, const uint8_t* expected, size_t size) {
    uint8_t buf[kRecordBytes + 1];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 9999;
    CHECK_RESULT(StoreResult::Ok, s.read(slot, buf, size, stored));
    TEST_ASSERT_EQUAL_UINT32(size, stored);
    if (size > 0) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buf, size);
    }
    TEST_ASSERT_EQUAL_HEX8(kGuard, buf[size]);
}

void assertI32(IKeyValueStore& kv, const char* key, int32_t expected) {
    int32_t v = expected ^ 0x55;
    CHECK_RESULT(StoreResult::Ok, kv.getI32(key, v));
    TEST_ASSERT_EQUAL_INT32(expected, v);
}

}  // namespace

// ---------------------------------------------------------------------------
// init() and Unavailable
// ---------------------------------------------------------------------------

void test_kv_unavailable_before_init(void) {
    const std::string r = root("unavail_kv");
    HostFileKeyValueStore kv(r.c_str(), "ns", kBlobBytes);
    const uint8_t data[2] = {1, 2};
    int32_t i = 11;
    uint32_t u = 12;
    uint8_t buf[4];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 13;

    CHECK_RESULT(StoreResult::Unavailable, kv.setI32("k", 1));
    CHECK_RESULT(StoreResult::Unavailable, kv.getI32("k", i));
    CHECK_RESULT(StoreResult::Unavailable, kv.setU32("k", 1));
    CHECK_RESULT(StoreResult::Unavailable, kv.getU32("k", u));
    CHECK_RESULT(StoreResult::Unavailable, kv.setBlob("k", data, sizeof(data)));
    CHECK_RESULT(StoreResult::Unavailable, kv.getBlob("k", buf, sizeof(buf), stored));
    CHECK_RESULT(StoreResult::Unavailable, kv.erase("k"));
    CHECK_RESULT(StoreResult::Unavailable, kv.commit());
    CHECK_RESULT(StoreResult::Unavailable, kv.setI32("a b", 1));  // even an invalid key
    TEST_ASSERT_EQUAL_INT32(11, i);
    TEST_ASSERT_EQUAL_UINT32(12u, u);
    TEST_ASSERT_EQUAL_UINT32(13u, stored);
    TEST_ASSERT_EQUAL_HEX8(kGuard, buf[0]);
    TEST_ASSERT_FALSE(exists(r));  // nothing touched the disk
}

void test_slot_unavailable_before_init(void) {
    const std::string r = root("unavail_slot");
    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    const uint8_t data[2] = {1, 2};
    uint8_t buf[4];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 13;

    CHECK_RESULT(StoreResult::Unavailable, s.query(0, stored));
    CHECK_RESULT(StoreResult::Unavailable, s.read(0, buf, sizeof(buf), stored));
    CHECK_RESULT(StoreResult::Unavailable, s.write(0, data, sizeof(data)));
    CHECK_RESULT(StoreResult::Unavailable, s.erase(0));
    CHECK_RESULT(StoreResult::Unavailable, s.query(kSlots, stored));  // even out of range
    TEST_ASSERT_EQUAL_UINT32(13u, stored);
    TEST_ASSERT_EQUAL_HEX8(kGuard, buf[0]);
    // Geometry is configuration, known before init().
    TEST_ASSERT_EQUAL_UINT16(kSlots, s.slotCount());
    TEST_ASSERT_EQUAL_UINT32(kRecordBytes, s.maxRecordSize());
    TEST_ASSERT_FALSE(exists(r));
}

void test_unavailable_after_failed_init(void) {
    const std::string r = root("failed_init");
    HostFileKeyValueStore kv(r.c_str(), "a b", kBlobBytes);
    CHECK_RESULT(StoreResult::InvalidKey, kv.init());
    CHECK_RESULT(StoreResult::Unavailable, kv.setI32("k", 1));

    HostFileSlotStore s(r.c_str(), "saves", 0, kRecordBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, s.init());
    const uint8_t data[1] = {1};
    CHECK_RESULT(StoreResult::Unavailable, s.write(0, data, sizeof(data)));
    TEST_ASSERT_FALSE(exists(r));
}

void test_init_creates_nested_root(void) {
    const std::string r = root("nested/a/b");
    TEST_ASSERT_FALSE(exists(r));
    HostFileKeyValueStore kv(r.c_str(), "settings", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    TEST_ASSERT_TRUE(isDirectory(r + "/settings"));
    CHECK_RESULT(StoreResult::Ok, kv.init());  // idempotent over an existing tree

    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::Ok, s.init());
    TEST_ASSERT_TRUE(isDirectory(r + "/saves"));

    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 4));
    assertI32(kv, "k", 4);
}

void test_init_fails_when_root_is_a_file(void) {
    const std::string r = root("plainfile");
    std::error_code ec;
    fs::create_directories(gBase, ec);
    TEST_ASSERT_TRUE(isDirectory(gBase));
    const uint8_t b = 1;
    writeRaw(r, &b, 1);
    HostFileKeyValueStore kv(r.c_str(), "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::IoError, kv.init());
    CHECK_RESULT(StoreResult::Unavailable, kv.setI32("k", 1));
}

void test_init_rejects_invalid_arguments(void) {
    const std::string r = root("invalid_args");
    const char* const badNames[] = {nullptr, "", "a b", "a.b", "a/b", "abcdefghijklmnop"};
    for (const char* name : badNames) {
        HostFileKeyValueStore kv(r.c_str(), name, kBlobBytes);
        CHECK_RESULT(StoreResult::InvalidKey, kv.init());
        HostFileSlotStore s(r.c_str(), name, kSlots, kRecordBytes);
        CHECK_RESULT(StoreResult::InvalidKey, s.init());
    }

    HostFileKeyValueStore nullRoot(nullptr, "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, nullRoot.init());
    HostFileKeyValueStore emptyRoot("", "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, emptyRoot.init());
    HostFileKeyValueStore zeroBlob(r.c_str(), "ns", 0);
    CHECK_RESULT(StoreResult::InvalidArgument, zeroBlob.init());

    HostFileSlotStore nullSlotRoot(nullptr, "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, nullSlotRoot.init());
    HostFileSlotStore zeroSlots(r.c_str(), "saves", 0, kRecordBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, zeroSlots.init());
    HostFileSlotStore zeroRecord(r.c_str(), "saves", kSlots, 0);
    CHECK_RESULT(StoreResult::InvalidArgument, zeroRecord.init());

    TEST_ASSERT_FALSE(exists(r));
}

void test_init_rejects_too_long_root(void) {
    // Longest root that still fits: root + '/' + name + the file name reserve
    // + NUL == kHostFileMaxPathBytes.
    const char* name = "n";
    const size_t limit = kHostFileMaxPathBytes - 1 - 1 - strlen(name) -
                         kHostFileNameReserveBytes;
    std::string atLimit = gBase + "/long_";
    TEST_ASSERT_TRUE(atLimit.size() < limit);
    atLimit.append(limit - atLimit.size(), 'x');
    TEST_ASSERT_EQUAL_UINT32(limit, atLimit.size());
    const std::string overLimit = atLimit + "y";

    HostFileKeyValueStore over(overLimit.c_str(), name, kBlobBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, over.init());
    HostFileSlotStore overSlots(overLimit.c_str(), name, kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, overSlots.init());
    const std::string huge(kHostFileMaxPathBytes * 2, 'z');
    HostFileKeyValueStore hugeKv(huge.c_str(), name, kBlobBytes);
    CHECK_RESULT(StoreResult::InvalidArgument, hugeKv.init());
    TEST_ASSERT_FALSE(exists(overLimit));

    // At the limit the longest key and its temp file still fit.
    HostFileKeyValueStore at(atLimit.c_str(), name, kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, at.init());
    CHECK_RESULT(StoreResult::Ok, at.setI32("ABCDEFGHIJKLMNO", 15));
    assertI32(at, "ABCDEFGHIJKLMNO", 15);
}

// ---------------------------------------------------------------------------
// Durability and layout
// ---------------------------------------------------------------------------

void test_data_survives_a_new_store_object(void) {
    const std::string r = root("reboot");
    const uint8_t blob[5] = {1, 2, 3, 4, 5};
    const uint8_t rec[7] = {9, 8, 7, 6, 5, 4, 3};
    {
        HostFileKeyValueStore kv(r.c_str(), "settings", kBlobBytes);
        CHECK_RESULT(StoreResult::Ok, kv.init());
        CHECK_RESULT(StoreResult::Ok, kv.setI32("i", -77));
        CHECK_RESULT(StoreResult::Ok, kv.setU32("u", 0xCAFEF00Du));
        CHECK_RESULT(StoreResult::Ok, kv.setBlob("b", blob, sizeof(blob)));
        CHECK_RESULT(StoreResult::Ok, kv.commit());
        HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
        CHECK_RESULT(StoreResult::Ok, s.init());
        CHECK_RESULT(StoreResult::Ok, s.write(2, rec, sizeof(rec)));
        CHECK_RESULT(StoreResult::Ok, s.write(0, nullptr, 0));
    }
    HostFileKeyValueStore kv(r.c_str(), "settings", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    assertI32(kv, "i", -77);
    uint32_t u = 0;
    CHECK_RESULT(StoreResult::Ok, kv.getU32("u", u));
    TEST_ASSERT_EQUAL_HEX32(0xCAFEF00Du, u);
    assertBlob(kv, "b", blob, sizeof(blob));

    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::Ok, s.init());
    assertSlot(s, 2, rec, sizeof(rec));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::Ok, s.query(0, stored));
    TEST_ASSERT_EQUAL_UINT32(0u, stored);
    CHECK_RESULT(StoreResult::NotFound, s.query(1, stored));
    TEST_ASSERT_EQUAL_UINT32(0u, stored);
}

void test_file_layout(void) {
    const std::string r = root("layout");
    HostFileKeyValueStore kv(r.c_str(), "settings", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    CHECK_RESULT(StoreResult::Ok, kv.setI32("Ab_C-1", 1));
    TEST_ASSERT_TRUE(exists(r + "/settings/^ab_^c-1.kv"));
    CHECK_RESULT(StoreResult::Ok, kv.erase("Ab_C-1"));
    TEST_ASSERT_FALSE(exists(r + "/settings/^ab_^c-1.kv"));

    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::Ok, s.init());
    const uint8_t rec[1] = {1};
    CHECK_RESULT(StoreResult::Ok, s.write(2, rec, sizeof(rec)));
    TEST_ASSERT_TRUE(exists(r + "/saves/2.slot"));
    TEST_ASSERT_FALSE(exists(r + "/saves/2.slot.tmp"));
}

void test_keys_differing_in_case_are_distinct(void) {
    const std::string r = root("case");
    HostFileKeyValueStore kv(r.c_str(), "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    CHECK_RESULT(StoreResult::Ok, kv.setI32("Ab", 1));
    CHECK_RESULT(StoreResult::Ok, kv.setI32("ab", 2));
    CHECK_RESULT(StoreResult::Ok, kv.setI32("aB", 3));
    const uint8_t blob[2] = {4, 4};
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("AB", blob, sizeof(blob)));
    assertI32(kv, "Ab", 1);
    assertI32(kv, "ab", 2);
    assertI32(kv, "aB", 3);
    assertBlob(kv, "AB", blob, sizeof(blob));

    CHECK_RESULT(StoreResult::Ok, kv.erase("ab"));
    int32_t v = 0;
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("ab", v));
    assertI32(kv, "Ab", 1);
    assertI32(kv, "aB", 3);
    assertBlob(kv, "AB", blob, sizeof(blob));
}

void test_two_namespaces_under_one_root_are_independent(void) {
    const std::string r = root("namespaces");
    HostFileKeyValueStore one(r.c_str(), "one", kBlobBytes);
    HostFileKeyValueStore two(r.c_str(), "two", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, one.init());
    CHECK_RESULT(StoreResult::Ok, two.init());

    CHECK_RESULT(StoreResult::Ok, one.setI32("k", 1));
    int32_t v = 0;
    CHECK_RESULT(StoreResult::NotFound, two.getI32("k", v));
    CHECK_RESULT(StoreResult::Ok, two.setI32("k", 2));
    assertI32(one, "k", 1);
    assertI32(two, "k", 2);
    CHECK_RESULT(StoreResult::Ok, one.erase("k"));
    CHECK_RESULT(StoreResult::NotFound, one.getI32("k", v));
    assertI32(two, "k", 2);
}

// ---------------------------------------------------------------------------
// Replace-or-keep (S7) under a simulated crash
// ---------------------------------------------------------------------------

void test_kv_fail_before_replace_keeps_previous_value(void) {
    const std::string r = root("crash_kv");
    HostFileKeyValueStore kv(r.c_str(), "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    const uint8_t old[3] = {1, 2, 3};
    const uint8_t neu[5] = {9, 9, 9, 9, 9};
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("k", old, sizeof(old)));

    kv.setFailBeforeReplace(true);
    CHECK_RESULT(StoreResult::IoError, kv.setBlob("k", neu, sizeof(neu)));
    TEST_ASSERT_TRUE(exists(r + "/ns/k.kv.tmp"));
    CHECK_RESULT(StoreResult::IoError, kv.setI32("k", 5));
    CHECK_RESULT(StoreResult::IoError, kv.setU32("k", 6));
    CHECK_RESULT(StoreResult::IoError, kv.setI32("new", 7));
    CHECK_RESULT(StoreResult::InvalidKey, kv.setI32("a b", 1));  // argument checks first
    assertBlob(kv, "k", old, sizeof(old));
    int32_t v = 0;
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("k", v));
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("new", v));
    TEST_ASSERT_TRUE(exists(r + "/ns/new.kv.tmp"));

    kv.setFailBeforeReplace(false);
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("k", neu, sizeof(neu)));
    assertBlob(kv, "k", neu, sizeof(neu));
    TEST_ASSERT_FALSE(exists(r + "/ns/k.kv.tmp"));
    // The reboot sees the same thing.
    HostFileKeyValueStore again(r.c_str(), "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, again.init());
    assertBlob(again, "k", neu, sizeof(neu));
    CHECK_RESULT(StoreResult::NotFound, again.getI32("new", v));
}

void test_slot_fail_before_replace_keeps_previous_record(void) {
    const std::string r = root("crash_slot");
    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::Ok, s.init());
    const uint8_t old[4] = {1, 2, 3, 4};
    const uint8_t neu[6] = {7, 7, 7, 7, 7, 7};
    CHECK_RESULT(StoreResult::Ok, s.write(0, old, sizeof(old)));

    s.setFailBeforeReplace(true);
    CHECK_RESULT(StoreResult::IoError, s.write(0, neu, sizeof(neu)));
    CHECK_RESULT(StoreResult::IoError, s.write(1, neu, sizeof(neu)));
    CHECK_RESULT(StoreResult::InvalidArgument, s.write(kSlots, neu, sizeof(neu)));
    TEST_ASSERT_TRUE(exists(r + "/saves/0.slot.tmp"));
    TEST_ASSERT_TRUE(exists(r + "/saves/1.slot.tmp"));
    assertSlot(s, 0, old, sizeof(old));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::NotFound, s.query(1, stored));

    s.setFailBeforeReplace(false);
    CHECK_RESULT(StoreResult::Ok, s.write(0, neu, sizeof(neu)));
    assertSlot(s, 0, neu, sizeof(neu));
    TEST_ASSERT_FALSE(exists(r + "/saves/0.slot.tmp"));
}

void test_stale_tmp_with_garbage_does_not_change_reads(void) {
    const std::string r = root("stale");
    HostFileKeyValueStore kv(r.c_str(), "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::Ok, s.init());
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 5));
    const uint8_t rec[3] = {3, 2, 1};
    CHECK_RESULT(StoreResult::Ok, s.write(0, rec, sizeof(rec)));

    uint8_t garbage[100];
    for (size_t n = 0; n < sizeof(garbage); ++n) {
        garbage[n] = static_cast<uint8_t>(0xA5u ^ n);
    }
    writeRaw(r + "/ns/k.kv.tmp", garbage, sizeof(garbage));
    writeRaw(r + "/ns/q.kv.tmp", garbage, 5);
    writeRaw(r + "/saves/0.slot.tmp", garbage, sizeof(garbage));
    writeRaw(r + "/saves/1.slot.tmp", garbage, 4);

    assertI32(kv, "k", 5);
    int32_t v = 0;
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("q", v));
    CHECK_RESULT(StoreResult::NotFound, kv.erase("q"));
    assertSlot(s, 0, rec, sizeof(rec));
    size_t stored = 99;
    CHECK_RESULT(StoreResult::NotFound, s.query(1, stored));
    CHECK_RESULT(StoreResult::NotFound, s.erase(1));

    // The next write overwrites the stale temp file and lands normally.
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 6));
    assertI32(kv, "k", 6);
    CHECK_RESULT(StoreResult::Ok, kv.setI32("q", 7));
    assertI32(kv, "q", 7);
    CHECK_RESULT(StoreResult::Ok, s.write(1, rec, 2));
    assertSlot(s, 1, rec, 2);
    TEST_ASSERT_FALSE(exists(r + "/ns/k.kv.tmp"));
    TEST_ASSERT_FALSE(exists(r + "/saves/1.slot.tmp"));
}

// ---------------------------------------------------------------------------
// Damaged files
// ---------------------------------------------------------------------------

void test_corrupted_kv_file_is_io_error(void) {
    const std::string r = root("corrupt_kv");
    HostFileKeyValueStore kv(r.c_str(), "ns", kBlobBytes);
    CHECK_RESULT(StoreResult::Ok, kv.init());
    const std::string file = r + "/ns/k.kv";
    int32_t i = 11;
    uint32_t u = 12;
    uint8_t buf[kBlobBytes];
    memset(buf, kGuard, sizeof(buf));
    size_t stored = 13;

    // A truncated integer.
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 1));
    truncateTo(file, 2);
    CHECK_RESULT(StoreResult::IoError, kv.getI32("k", i));
    CHECK_RESULT(StoreResult::IoError, kv.getU32("k", u));
    CHECK_RESULT(StoreResult::IoError, kv.getBlob("k", buf, sizeof(buf), stored));

    // An empty file has no type tag.
    truncateTo(file, 0);
    CHECK_RESULT(StoreResult::IoError, kv.getI32("k", i));
    CHECK_RESULT(StoreResult::IoError, kv.getBlob("k", nullptr, 0, stored));

    // An unknown type tag.
    const uint8_t badTag[5] = {0x7F, 1, 2, 3, 4};
    writeRaw(file, badTag, sizeof(badTag));
    CHECK_RESULT(StoreResult::IoError, kv.getU32("k", u));
    CHECK_RESULT(StoreResult::IoError, kv.getBlob("k", buf, sizeof(buf), stored));

    // A blob longer than the store accepts.
    uint8_t longBlob[1 + kBlobBytes + 1];
    memset(longBlob, 0x11, sizeof(longBlob));
    CHECK_RESULT(StoreResult::Ok, kv.setBlob("k", longBlob, 1));
    longBlob[0] = 0;
    {
        // Reuse the tag the adapter wrote, so only the size is wrong.
        FILE* f = fopen(file.c_str(), "rb");
        TEST_ASSERT_NOT_NULL(f);
        TEST_ASSERT_EQUAL_UINT32(1u, fread(longBlob, 1, 1, f));
        fclose(f);
    }
    writeRaw(file, longBlob, sizeof(longBlob));
    CHECK_RESULT(StoreResult::IoError, kv.getBlob("k", buf, sizeof(buf), stored));
    CHECK_RESULT(StoreResult::IoError, kv.getI32("k", i));

    TEST_ASSERT_EQUAL_INT32(11, i);
    TEST_ASSERT_EQUAL_UINT32(12u, u);
    TEST_ASSERT_EQUAL_UINT32(13u, stored);
    TEST_ASSERT_EQUAL_HEX8(kGuard, buf[0]);

    // A write replaces the damaged file; erase removes one too.
    CHECK_RESULT(StoreResult::Ok, kv.setI32("k", 9));
    assertI32(kv, "k", 9);
    truncateTo(file, 1);
    CHECK_RESULT(StoreResult::Ok, kv.erase("k"));
    CHECK_RESULT(StoreResult::NotFound, kv.getI32("k", i));
}

void test_oversized_slot_file_is_io_error(void) {
    const std::string r = root("corrupt_slot");
    HostFileSlotStore s(r.c_str(), "saves", kSlots, kRecordBytes);
    CHECK_RESULT(StoreResult::Ok, s.init());
    uint8_t big[kRecordBytes + 1];
    memset(big, 0x22, sizeof(big));
    writeRaw(r + "/saves/1.slot", big, sizeof(big));

    size_t stored = 13;
    uint8_t buf[kRecordBytes + 1];
    memset(buf, kGuard, sizeof(buf));
    CHECK_RESULT(StoreResult::IoError, s.query(1, stored));
    CHECK_RESULT(StoreResult::IoError, s.read(1, buf, sizeof(buf), stored));
    TEST_ASSERT_EQUAL_UINT32(13u, stored);
    TEST_ASSERT_EQUAL_HEX8(kGuard, buf[0]);

    CHECK_RESULT(StoreResult::Ok, s.write(1, big, kRecordBytes));
    assertSlot(s, 1, big, kRecordBytes);
}

#else  // !PIXELROOT32_ENABLE_PERSISTENCE

void test_persist_hostfile_flag_off_stub(void) {
    // With the flag off the host file adapter declares nothing; this
    // translation unit compiling without it is the contract.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PERSISTENCE=0: the host file stores are not "
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
    gBase = makeBase();
    RUN_TEST(test_kv_unavailable_before_init);
    RUN_TEST(test_slot_unavailable_before_init);
    RUN_TEST(test_unavailable_after_failed_init);
    RUN_TEST(test_init_creates_nested_root);
    RUN_TEST(test_init_fails_when_root_is_a_file);
    RUN_TEST(test_init_rejects_invalid_arguments);
    RUN_TEST(test_init_rejects_too_long_root);
    RUN_TEST(test_data_survives_a_new_store_object);
    RUN_TEST(test_file_layout);
    RUN_TEST(test_keys_differing_in_case_are_distinct);
    RUN_TEST(test_two_namespaces_under_one_root_are_independent);
    RUN_TEST(test_kv_fail_before_replace_keeps_previous_value);
    RUN_TEST(test_slot_fail_before_replace_keeps_previous_record);
    RUN_TEST(test_stale_tmp_with_garbage_does_not_change_reads);
    RUN_TEST(test_corrupted_kv_file_is_io_error);
    RUN_TEST(test_oversized_slot_file_is_io_error);
    std::error_code ec;
    fs::remove_all(gBase, ec);
#else
    RUN_TEST(test_persist_hostfile_flag_off_stub);
#endif

    return UNITY_END();
}
