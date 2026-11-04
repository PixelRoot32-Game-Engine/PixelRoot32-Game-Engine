/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include "persistence/KeyValueStore.h"
#include "persistence/RecordCodec.h"
#include "persistence/SlotStore.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>

namespace pixelroot32::persistence {

/**
 * @class NullKeyValueStore
 * @brief IKeyValueStore that is never available.
 *
 * Stands in for an unbound settings role. Every call returns Unavailable
 * without validating its arguments and without touching any output.
 */
class NullKeyValueStore final : public IKeyValueStore {
public:
    StoreResult getI32(const char*, int32_t&) override { return StoreResult::Unavailable; }
    StoreResult setI32(const char*, int32_t) override { return StoreResult::Unavailable; }
    StoreResult getU32(const char*, uint32_t&) override { return StoreResult::Unavailable; }
    StoreResult setU32(const char*, uint32_t) override { return StoreResult::Unavailable; }
    StoreResult getBlob(const char*, uint8_t*, size_t, size_t&) override {
        return StoreResult::Unavailable;
    }
    StoreResult setBlob(const char*, const uint8_t*, size_t) override {
        return StoreResult::Unavailable;
    }
    StoreResult erase(const char*) override { return StoreResult::Unavailable; }
    StoreResult commit() override { return StoreResult::Unavailable; }
};

/**
 * @class NullSlotStore
 * @brief ISlotStore that is never available.
 *
 * Stands in for an unbound saves role: it has no slots (slotCount() and
 * maxRecordSize() are 0) and every operation returns Unavailable - including
 * for a slot index that would be out of range - without touching any output.
 */
class NullSlotStore final : public ISlotStore {
public:
    uint16_t slotCount() const override { return 0; }
    size_t maxRecordSize() const override { return 0; }
    StoreResult query(uint16_t, size_t&) override { return StoreResult::Unavailable; }
    StoreResult read(uint16_t, uint8_t*, size_t, size_t&) override {
        return StoreResult::Unavailable;
    }
    StoreResult write(uint16_t, const uint8_t*, size_t) override {
        return StoreResult::Unavailable;
    }
    StoreResult erase(uint16_t) override { return StoreResult::Unavailable; }
};

/**
 * @class PersistenceService
 * @brief Thin role registry: a settings key-value store and a saves slot store.
 *
 * The game constructs its stores and this service, binds each store to a
 * role at setup, and may still call a store directly. The service adds two
 * things on top:
 * - settings() and saves() are never null: an unbound role answers through a
 *   built-in null store whose every call is Unavailable.
 * - saveRecord()/loadRecord() (save slots) and saveSettingRecord()/
 *   loadSettingRecord() (settings blob keys) wrap a payload in the
 *   RecordCodec envelope on the way to the store, and validate it on the way
 *   back.
 *
 * Writes are the game's decision (N4). The service never calls commit() and
 * never writes in the background: on the ESP32 a flash write or erase stalls
 * flash-resident code on both cores, so the game makes its saves - and calls
 * `settings().commit()` - at a safe point such as a menu.
 *
 * The record helpers use only caller-owned scratch buffers; the service holds
 * no buffer and allocates nothing. Its size is two pointers plus two null
 * stores (one vtable pointer each): four pointers in total, 16 bytes on the
 * ESP32. Like the stores, it is used from one task (the game loop).
 */
class PersistenceService {
public:
    /** @brief Binds the settings role; nullptr unbinds it. The store is not owned. */
    void bindSettings(IKeyValueStore* store) { settings_ = store; }

    /** @brief Binds the saves role; nullptr unbinds it. The store is not owned. */
    void bindSaves(ISlotStore* store) { saves_ = store; }

    /** @brief The bound settings store, or a null store answering Unavailable. */
    IKeyValueStore& settings() {
        return settings_ != nullptr ? *settings_ : static_cast<IKeyValueStore&>(nullSettings_);
    }

    /** @brief The bound saves store, or a null store with no slots. */
    ISlotStore& saves() {
        return saves_ != nullptr ? *saves_ : static_cast<ISlotStore&>(nullSaves_);
    }

    /** @brief Whether a settings store is bound. */
    bool hasSettings() const { return settings_ != nullptr; }

    /** @brief Whether a saves store is bound. */
    bool hasSaves() const { return saves_ != nullptr; }

    /**
     * @brief Encodes a payload into @p scratch and writes it to a save slot.
     *
     * Checks run in this order: Unavailable when no saves store is bound;
     * InvalidArgument for a null @p scratch; the codec's result (TooLarge when
     * the record needs more than @p scratchCapacity, InvalidArgument for a null
     * @p payload with a non-zero size) with the store untouched; then the
     * store's write() result, unchanged.
     *
     * @param slot Slot index.
     * @param schema Game-owned schema version, stored as is.
     * @param payload Payload bytes; nullptr only when @p size is 0.
     * @param size Payload length; 0 is a valid record.
     * @param scratch Caller-owned buffer that receives the encoded record.
     * @param scratchCapacity Size of @p scratch; needs RecordCodec::kHeaderSize + @p size.
     * @return Ok; Unavailable; InvalidArgument; TooLarge; or any error of
     *         ISlotStore::write() (InvalidArgument for an out-of-range slot,
     *         TooLarge, NoSpace, IoError). On failure the slot keeps its
     *         previous record.
     */
    StoreResult saveRecord(uint16_t slot, uint16_t schema, const uint8_t* payload, size_t size,
                           uint8_t* scratch, size_t scratchCapacity);

    /**
     * @brief Reads a save slot into @p scratch and validates its envelope.
     *
     * Checks run in this order: Unavailable when no saves store is bound;
     * InvalidArgument for a null @p scratch; the store's read() result,
     * unchanged (NotFound, TooLarge when the stored record exceeds
     * @p scratchCapacity, InvalidArgument for an out-of-range slot, IoError);
     * then Corrupt when the record fails RecordCodec::decode(). The schema
     * version is reported, never judged: migration is the game's job.
     *
     * @param slot Slot index.
     * @param scratch Caller-owned buffer that receives the whole record.
     * @param scratchCapacity Size of @p scratch.
     * @param payload On Ok, points at the payload inside @p scratch; valid as
     *        long as @p scratch is. Untouched on failure.
     * @param info On Ok, the stored schema version and payload size. Untouched
     *        on failure.
     * @return Ok or the first failure above.
     */
    StoreResult loadRecord(uint16_t slot, uint8_t* scratch, size_t scratchCapacity,
                           const uint8_t*& payload, RecordInfo& info);

    /**
     * @brief Encodes a payload into @p scratch and stores it as a settings blob.
     *
     * Same check order and results as saveRecord(), over the settings role
     * and IKeyValueStore::setBlob() (InvalidKey for a key breaking
     * isValidKey(), TooLarge, NoSpace, IoError propagate unchanged).
     *
     * This does not call commit() (N4): the blob may stay volatile until the
     * game calls `settings().commit()` at a safe point.
     *
     * @param key Settings key the record is stored under.
     * @param schema Game-owned schema version, stored as is.
     * @param payload Payload bytes; nullptr only when @p size is 0.
     * @param size Payload length; 0 is a valid record.
     * @param scratch Caller-owned buffer that receives the encoded record.
     * @param scratchCapacity Size of @p scratch; needs RecordCodec::kHeaderSize + @p size.
     * @return Ok or the first failure; on failure the key keeps its previous value.
     */
    StoreResult saveSettingRecord(const char* key, uint16_t schema, const uint8_t* payload,
                                  size_t size, uint8_t* scratch, size_t scratchCapacity);

    /**
     * @brief Reads a settings blob into @p scratch and validates its envelope.
     *
     * Same check order and results as loadRecord(), over the settings role
     * and IKeyValueStore::getBlob() (NotFound when the key is missing or holds
     * an integer, TooLarge, InvalidKey, IoError propagate unchanged; Corrupt
     * when decoding fails).
     *
     * Never calls commit() (N4); reading has no durability step.
     *
     * @param key Settings key to read.
     * @param scratch Caller-owned buffer that receives the whole record.
     * @param scratchCapacity Size of @p scratch.
     * @param payload On Ok, points at the payload inside @p scratch. Untouched
     *        on failure.
     * @param info On Ok, the stored schema version and payload size. Untouched
     *        on failure.
     * @return Ok or the first failure.
     */
    StoreResult loadSettingRecord(const char* key, uint8_t* scratch, size_t scratchCapacity,
                                  const uint8_t*& payload, RecordInfo& info);

private:
    IKeyValueStore* settings_ = nullptr;
    ISlotStore* saves_ = nullptr;
    NullKeyValueStore nullSettings_;
    NullSlotStore nullSaves_;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
