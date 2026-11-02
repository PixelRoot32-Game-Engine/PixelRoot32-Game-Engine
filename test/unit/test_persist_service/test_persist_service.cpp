/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the persistence role registry and its record helpers
 * (persistence/PersistenceService.h): built-in null stores for unbound roles,
 * binding and unbinding, saveRecord/loadRecord over a slot and
 * saveSettingRecord/loadSettingRecord over a settings blob key, driven through
 * the memory adapters and their fault hooks.
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

#include "persistence/MemoryStores.h"
#include "persistence/PersistenceService.h"
#include "persistence/RecordCodec.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

using namespace pixelroot32::persistence;

namespace {

using TestKv = MemoryKeyValueStore<4, 64>;
using TestSlots = MemorySlotStore<3, 64>;

constexpr uint16_t kSchema = 0x0102;
const uint8_t kPayload[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
const uint8_t kOtherPayload[3] = {0xA1, 0xB2, 0xC3};
constexpr size_t kRecordSize = RecordCodec::kHeaderSize + sizeof(kPayload);
constexpr size_t kScratchSize = 64;

void assertResult(StoreResult expected, StoreResult actual) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),
                            static_cast<uint8_t>(actual));
}

// Sentinel outputs: a load that fails must leave both exactly as they were.
struct LoadOutputs {
    uint8_t marker = 0;
    const uint8_t* payload = &marker;
    RecordInfo info{0xBEEF, 777};

    void assertUntouched() const {
        TEST_ASSERT_EQUAL_PTR(&marker, payload);
        TEST_ASSERT_EQUAL_HEX16(0xBEEF, info.schemaVersion);
        TEST_ASSERT_EQUAL_size_t(777, info.payloadSize);
    }
};

// Asserts a successful load decoded `expected` with schema `schema`, and that
// the payload pointer lies inside `scratch`.
void assertLoaded(const LoadOutputs& out, const uint8_t* scratch, size_t capacity,
                  uint16_t schema, const uint8_t* expected, size_t size) {
    TEST_ASSERT_EQUAL_HEX16(schema, out.info.schemaVersion);
    TEST_ASSERT_EQUAL_size_t(size, out.info.payloadSize);
    TEST_ASSERT_EQUAL_PTR(scratch + RecordCodec::kHeaderSize, out.payload);
    TEST_ASSERT_TRUE(out.payload >= scratch);
    TEST_ASSERT_TRUE(out.payload + size <= scratch + capacity);
    if (size > 0) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out.payload, size);
    }
}

}  // namespace

// --- Unbound roles -----------------------------------------------------------

void test_persist_service_unbound_roles_report_unbound(void) {
    PersistenceService service;
    TEST_ASSERT_FALSE(service.hasSettings());
    TEST_ASSERT_FALSE(service.hasSaves());
}

void test_persist_service_unbound_settings_answers_unavailable(void) {
    PersistenceService service;
    IKeyValueStore& kv = service.settings();

    int32_t i32 = 11;
    uint32_t u32 = 22;
    uint8_t buf[4] = {1, 2, 3, 4};
    size_t stored = 33;

    assertResult(StoreResult::Unavailable, kv.getI32("k", i32));
    assertResult(StoreResult::Unavailable, kv.setI32("k", 5));
    assertResult(StoreResult::Unavailable, kv.getU32("k", u32));
    assertResult(StoreResult::Unavailable, kv.setU32("k", 6u));
    assertResult(StoreResult::Unavailable, kv.getBlob("k", buf, sizeof(buf), stored));
    assertResult(StoreResult::Unavailable, kv.setBlob("k", buf, sizeof(buf)));
    assertResult(StoreResult::Unavailable, kv.erase("k"));
    assertResult(StoreResult::Unavailable, kv.commit());
    // Even an invalid key answers Unavailable: there is no store to judge it.
    assertResult(StoreResult::Unavailable, kv.getI32("bad key!", i32));

    TEST_ASSERT_EQUAL_INT32(11, i32);
    TEST_ASSERT_EQUAL_UINT32(22, u32);
    TEST_ASSERT_EQUAL_size_t(33, stored);
    const uint8_t expectedBuf[4] = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedBuf, buf, sizeof(buf));
}

void test_persist_service_unbound_saves_answers_unavailable(void) {
    PersistenceService service;
    ISlotStore& slots = service.saves();

    TEST_ASSERT_EQUAL_UINT16(0, slots.slotCount());
    TEST_ASSERT_EQUAL_size_t(0, slots.maxRecordSize());

    uint8_t buf[4] = {1, 2, 3, 4};
    size_t stored = 33;
    const uint16_t slotsToTry[] = {0, 1, 0xFFFF};
    for (uint16_t slot : slotsToTry) {
        assertResult(StoreResult::Unavailable, slots.query(slot, stored));
        assertResult(StoreResult::Unavailable, slots.read(slot, buf, sizeof(buf), stored));
        assertResult(StoreResult::Unavailable, slots.write(slot, buf, sizeof(buf)));
        assertResult(StoreResult::Unavailable, slots.erase(slot));
    }
    TEST_ASSERT_EQUAL_size_t(33, stored);
    const uint8_t expectedBuf[4] = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedBuf, buf, sizeof(buf));
}

void test_persist_service_unbound_record_helpers_answer_unavailable(void) {
    PersistenceService service;
    uint8_t scratch[kScratchSize] = {};
    LoadOutputs out;

    assertResult(StoreResult::Unavailable,
                 service.saveRecord(0, kSchema, kPayload, sizeof(kPayload),
                                    scratch, sizeof(scratch)));
    assertResult(StoreResult::Unavailable,
                 service.loadRecord(0, scratch, sizeof(scratch), out.payload,
                                    out.info));
    assertResult(StoreResult::Unavailable,
                 service.saveSettingRecord("cfg", kSchema, kPayload, sizeof(kPayload), scratch,
                                    sizeof(scratch)));
    assertResult(StoreResult::Unavailable,
                 service.loadSettingRecord("cfg", scratch, sizeof(scratch), out.payload,
                                    out.info));
    // Unavailable is checked before any argument: a null scratch still answers it.
    assertResult(StoreResult::Unavailable,
                 service.saveRecord(0, kSchema, kPayload, sizeof(kPayload),
                                    nullptr, 0));
    assertResult(StoreResult::Unavailable,
                 service.loadSettingRecord("cfg", nullptr, 0, out.payload, out.info));
    out.assertUntouched();
}

// --- Binding -----------------------------------------------------------------

void test_persist_service_bind_then_unbind(void) {
    PersistenceService service;
    TestKv kv;
    TestSlots slots;

    service.bindSettings(&kv);
    service.bindSaves(&slots);
    TEST_ASSERT_TRUE(service.hasSettings());
    TEST_ASSERT_TRUE(service.hasSaves());
    TEST_ASSERT_EQUAL_PTR(&kv, &service.settings());
    TEST_ASSERT_EQUAL_PTR(&slots, &service.saves());
    TEST_ASSERT_EQUAL_UINT16(3, service.saves().slotCount());
    assertResult(StoreResult::Ok, service.settings().setI32("lives", 3));

    service.bindSettings(nullptr);
    service.bindSaves(nullptr);
    TEST_ASSERT_FALSE(service.hasSettings());
    TEST_ASSERT_FALSE(service.hasSaves());
    TEST_ASSERT_TRUE(&service.settings() != static_cast<IKeyValueStore*>(&kv));
    TEST_ASSERT_TRUE(&service.saves() != static_cast<ISlotStore*>(&slots));
    int32_t lives = 0;
    assertResult(StoreResult::Unavailable, service.settings().getI32("lives", lives));
    TEST_ASSERT_EQUAL_UINT16(0, service.saves().slotCount());

    // The store kept its data: unbinding only detaches it.
    assertResult(StoreResult::Ok, kv.getI32("lives", lives));
    TEST_ASSERT_EQUAL_INT32(3, lives);
}

void test_persist_service_roles_bind_independently(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    TEST_ASSERT_TRUE(service.hasSaves());
    TEST_ASSERT_FALSE(service.hasSettings());
    assertResult(StoreResult::Unavailable, service.settings().commit());
}

// --- Record helpers over a slot ----------------------------------------------

void test_persist_service_slot_record_round_trip(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};

    assertResult(StoreResult::Ok, service.saveRecord(1, kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));
    size_t stored = 0;
    assertResult(StoreResult::Ok, slots.query(1, stored));
    TEST_ASSERT_EQUAL_size_t(kRecordSize, stored);

    uint8_t loadScratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadRecord(1, loadScratch,
                                                     sizeof(loadScratch), out.payload,
                                                     out.info));
    assertLoaded(out, loadScratch, sizeof(loadScratch), kSchema, kPayload,
                 sizeof(kPayload));
}

void test_persist_service_slot_record_literal_zero_slot(void) {
    // A plain literal 0 names slot 0: the key helpers have their own names, so
    // this call compiles without a cast and is never ambiguous.
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};

    assertResult(StoreResult::Ok, service.saveRecord(0, kSchema, kPayload, sizeof(kPayload),
                                                     scratch, sizeof(scratch)));
    size_t stored = 0;
    assertResult(StoreResult::Ok, slots.query(0, stored));
    TEST_ASSERT_EQUAL_size_t(kRecordSize, stored);

    uint8_t loadScratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadRecord(0, loadScratch, sizeof(loadScratch),
                                                     out.payload, out.info));
    assertLoaded(out, loadScratch, sizeof(loadScratch), kSchema, kPayload,
                 sizeof(kPayload));
}

void test_persist_service_slot_record_round_trip_empty_payload(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[RecordCodec::kHeaderSize] = {};

    assertResult(StoreResult::Ok,
                 service.saveRecord(0, 7, nullptr, 0, scratch, sizeof(scratch)));
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadRecord(0, scratch, sizeof(scratch),
                                                     out.payload, out.info));
    assertLoaded(out, scratch, sizeof(scratch), 7, nullptr, 0);
}

void test_persist_service_slot_save_scratch_too_small(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kRecordSize - 1] = {};

    assertResult(StoreResult::TooLarge,
                 service.saveRecord(0, kSchema, kPayload, sizeof(kPayload),
                                    scratch, sizeof(scratch)));
    TEST_ASSERT_EQUAL_UINT32(0, slots.writes());
    size_t stored = 99;
    assertResult(StoreResult::NotFound, slots.query(0, stored));
}

void test_persist_service_slot_load_scratch_too_small(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::Ok, service.saveRecord(0, kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));

    uint8_t small[kRecordSize - 1];
    memset(small, 0x5A, sizeof(small));
    LoadOutputs out;
    assertResult(StoreResult::TooLarge, service.loadRecord(0, small, sizeof(small),
                                                           out.payload, out.info));
    out.assertUntouched();
    for (uint8_t b : small) {
        TEST_ASSERT_EQUAL_HEX8(0x5A, b);
    }
}

void test_persist_service_slot_load_missing_is_not_found(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::NotFound, service.loadRecord(2, scratch,
                                                           sizeof(scratch), out.payload,
                                                           out.info));
    out.assertUntouched();
}

void test_persist_service_slot_corrupt_record(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::Ok, service.saveRecord(0, kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));
    // One byte of the payload, past the header.
    assertResult(StoreResult::Ok, slots.corruptByte(0, RecordCodec::kHeaderSize + 2));

    LoadOutputs out;
    assertResult(StoreResult::Corrupt, service.loadRecord(0, scratch,
                                                          sizeof(scratch), out.payload,
                                                          out.info));
    out.assertUntouched();
}

void test_persist_service_slot_failing_store_keeps_previous_record(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::Ok, service.saveRecord(0, kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));

    slots.setFailWrites(true);
    assertResult(StoreResult::IoError,
                 service.saveRecord(0, 9, kOtherPayload, sizeof(kOtherPayload),
                                    scratch, sizeof(scratch)));
    slots.setFailWrites(false);

    uint8_t loadScratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadRecord(0, loadScratch,
                                                     sizeof(loadScratch), out.payload,
                                                     out.info));
    assertLoaded(out, loadScratch, sizeof(loadScratch), kSchema, kPayload,
                 sizeof(kPayload));
}

void test_persist_service_slot_out_of_range_is_invalid_argument(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    uint8_t scratch[kScratchSize] = {};
    LoadOutputs out;

    assertResult(StoreResult::InvalidArgument,
                 service.saveRecord(3, kSchema, kPayload, sizeof(kPayload),
                                    scratch, sizeof(scratch)));
    assertResult(StoreResult::InvalidArgument,
                 service.loadRecord(3, scratch, sizeof(scratch), out.payload,
                                    out.info));
    out.assertUntouched();
    TEST_ASSERT_EQUAL_UINT32(0, slots.writes());
}

void test_persist_service_slot_null_scratch_is_invalid_argument(void) {
    PersistenceService service;
    TestSlots slots;
    service.bindSaves(&slots);
    LoadOutputs out;

    assertResult(StoreResult::InvalidArgument,
                 service.saveRecord(0, kSchema, kPayload, sizeof(kPayload),
                                    nullptr, kScratchSize));
    assertResult(StoreResult::InvalidArgument,
                 service.loadRecord(0, nullptr, kScratchSize, out.payload,
                                    out.info));
    out.assertUntouched();
    TEST_ASSERT_EQUAL_UINT32(0, slots.writes());
}

// --- Record helpers over a settings key --------------------------------------

void test_persist_service_key_record_round_trip(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};

    assertResult(StoreResult::Ok, service.saveSettingRecord("cfg", kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));
    size_t stored = 0;
    assertResult(StoreResult::TooLarge, kv.getBlob("cfg", nullptr, 0, stored));
    TEST_ASSERT_EQUAL_size_t(kRecordSize, stored);

    uint8_t loadScratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadSettingRecord("cfg", loadScratch,
                                                     sizeof(loadScratch), out.payload,
                                                     out.info));
    assertLoaded(out, loadScratch, sizeof(loadScratch), kSchema, kPayload,
                 sizeof(kPayload));
}

void test_persist_service_key_record_round_trip_empty_payload(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[RecordCodec::kHeaderSize] = {};

    assertResult(StoreResult::Ok,
                 service.saveSettingRecord("empty", 0xFFFF, nullptr, 0, scratch, sizeof(scratch)));
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadSettingRecord("empty", scratch, sizeof(scratch),
                                                     out.payload, out.info));
    assertLoaded(out, scratch, sizeof(scratch), 0xFFFF, nullptr, 0);
}

void test_persist_service_key_save_scratch_too_small(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kRecordSize - 1] = {};

    assertResult(StoreResult::TooLarge,
                 service.saveSettingRecord("cfg", kSchema, kPayload, sizeof(kPayload), scratch,
                                    sizeof(scratch)));
    TEST_ASSERT_EQUAL_UINT32(0, kv.writes());
}

void test_persist_service_key_load_scratch_too_small(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::Ok, service.saveSettingRecord("cfg", kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));

    uint8_t small[kRecordSize - 1];
    memset(small, 0x5A, sizeof(small));
    LoadOutputs out;
    assertResult(StoreResult::TooLarge, service.loadSettingRecord("cfg", small, sizeof(small),
                                                           out.payload, out.info));
    out.assertUntouched();
    for (uint8_t b : small) {
        TEST_ASSERT_EQUAL_HEX8(0x5A, b);
    }
}

void test_persist_service_key_load_missing_is_not_found(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::NotFound, service.loadSettingRecord("nope", scratch, sizeof(scratch),
                                                           out.payload, out.info));
    out.assertUntouched();
}

void test_persist_service_key_corrupt_record(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::Ok, service.saveSettingRecord("cfg", kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));
    // The low schema byte, inside the CRC-covered header.
    assertResult(StoreResult::Ok, kv.corruptByte("cfg", 6));

    LoadOutputs out;
    assertResult(StoreResult::Corrupt, service.loadSettingRecord("cfg", scratch, sizeof(scratch),
                                                          out.payload, out.info));
    out.assertUntouched();
}

void test_persist_service_key_failing_store_keeps_previous_record(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::Ok, service.saveSettingRecord("cfg", kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));

    kv.setFailWrites(true);
    assertResult(StoreResult::IoError,
                 service.saveSettingRecord("cfg", 9, kOtherPayload, sizeof(kOtherPayload), scratch,
                                    sizeof(scratch)));
    kv.setFailWrites(false);

    uint8_t loadScratch[kScratchSize] = {};
    LoadOutputs out;
    assertResult(StoreResult::Ok, service.loadSettingRecord("cfg", loadScratch,
                                                     sizeof(loadScratch), out.payload,
                                                     out.info));
    assertLoaded(out, loadScratch, sizeof(loadScratch), kSchema, kPayload,
                 sizeof(kPayload));
}

void test_persist_service_key_invalid_key(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    LoadOutputs out;

    assertResult(StoreResult::InvalidKey,
                 service.saveSettingRecord("bad key", kSchema, kPayload, sizeof(kPayload), scratch,
                                    sizeof(scratch)));
    assertResult(StoreResult::InvalidKey,
                 service.loadSettingRecord("bad key", scratch, sizeof(scratch), out.payload,
                                    out.info));
    assertResult(StoreResult::InvalidKey,
                 service.loadSettingRecord(nullptr, scratch,
                                    sizeof(scratch), out.payload, out.info));
    out.assertUntouched();
    TEST_ASSERT_EQUAL_UINT32(0, kv.writes());
}

void test_persist_service_key_null_scratch_is_invalid_argument(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    LoadOutputs out;

    assertResult(StoreResult::InvalidArgument,
                 service.saveSettingRecord("cfg", kSchema, kPayload, sizeof(kPayload), nullptr,
                                    kScratchSize));
    assertResult(StoreResult::InvalidArgument,
                 service.loadSettingRecord("cfg", nullptr, kScratchSize, out.payload, out.info));
    out.assertUntouched();
    TEST_ASSERT_EQUAL_UINT32(0, kv.writes());
}

void test_persist_service_key_record_too_large_for_store(void) {
    // The record fits the scratch but not the store's per-blob maximum: the
    // store's TooLarge propagates unchanged.
    PersistenceService service;
    MemoryKeyValueStore<2, 8> kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    assertResult(StoreResult::TooLarge,
                 service.saveSettingRecord("cfg", kSchema, kPayload, sizeof(kPayload), scratch,
                                    sizeof(scratch)));
    TEST_ASSERT_EQUAL_UINT32(0, kv.writes());
}

// --- N4: the service never commits -------------------------------------------

void test_persist_service_never_commits(void) {
    PersistenceService service;
    TestKv kv;
    service.bindSettings(&kv);
    uint8_t scratch[kScratchSize] = {};
    LoadOutputs out;

    assertResult(StoreResult::Ok, service.saveSettingRecord("cfg", kSchema, kPayload,
                                                     sizeof(kPayload), scratch,
                                                     sizeof(scratch)));
    assertResult(StoreResult::Ok, service.saveSettingRecord("cfg", kSchema, kOtherPayload,
                                                     sizeof(kOtherPayload), scratch,
                                                     sizeof(scratch)));
    assertResult(StoreResult::Ok, service.loadSettingRecord("cfg", scratch, sizeof(scratch),
                                                     out.payload, out.info));
    assertResult(StoreResult::NotFound, service.loadSettingRecord("other", scratch,
                                                           sizeof(scratch), out.payload,
                                                           out.info));
    TEST_ASSERT_EQUAL_UINT32(2, kv.writes());
    TEST_ASSERT_EQUAL_UINT32(0, kv.commits());

    // The game decides when: an explicit commit through the role is counted.
    assertResult(StoreResult::Ok, service.settings().commit());
    TEST_ASSERT_EQUAL_UINT32(1, kv.commits());
}

#else  // !PIXELROOT32_ENABLE_PERSISTENCE

void test_persist_service_flag_off_stub(void) {
    // With the flag off, PersistenceService.h declares nothing at all. This
    // translation unit compiling and passing without referencing any of its
    // symbols is the flags-off contract every existing game relies on.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PERSISTENCE=0: the persistence service is not "
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
    RUN_TEST(test_persist_service_unbound_roles_report_unbound);
    RUN_TEST(test_persist_service_unbound_settings_answers_unavailable);
    RUN_TEST(test_persist_service_unbound_saves_answers_unavailable);
    RUN_TEST(test_persist_service_unbound_record_helpers_answer_unavailable);
    RUN_TEST(test_persist_service_bind_then_unbind);
    RUN_TEST(test_persist_service_roles_bind_independently);
    RUN_TEST(test_persist_service_slot_record_round_trip);
    RUN_TEST(test_persist_service_slot_record_literal_zero_slot);
    RUN_TEST(test_persist_service_slot_record_round_trip_empty_payload);
    RUN_TEST(test_persist_service_slot_save_scratch_too_small);
    RUN_TEST(test_persist_service_slot_load_scratch_too_small);
    RUN_TEST(test_persist_service_slot_load_missing_is_not_found);
    RUN_TEST(test_persist_service_slot_corrupt_record);
    RUN_TEST(test_persist_service_slot_failing_store_keeps_previous_record);
    RUN_TEST(test_persist_service_slot_out_of_range_is_invalid_argument);
    RUN_TEST(test_persist_service_slot_null_scratch_is_invalid_argument);
    RUN_TEST(test_persist_service_key_record_round_trip);
    RUN_TEST(test_persist_service_key_record_round_trip_empty_payload);
    RUN_TEST(test_persist_service_key_save_scratch_too_small);
    RUN_TEST(test_persist_service_key_load_scratch_too_small);
    RUN_TEST(test_persist_service_key_load_missing_is_not_found);
    RUN_TEST(test_persist_service_key_corrupt_record);
    RUN_TEST(test_persist_service_key_failing_store_keeps_previous_record);
    RUN_TEST(test_persist_service_key_invalid_key);
    RUN_TEST(test_persist_service_key_null_scratch_is_invalid_argument);
    RUN_TEST(test_persist_service_key_record_too_large_for_store);
    RUN_TEST(test_persist_service_never_commits);
#else
    RUN_TEST(test_persist_service_flag_off_stub);
#endif

    return UNITY_END();
}
