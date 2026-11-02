/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include "persistence/KeyValueStore.h"
#include "persistence/SlotStore.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pixelroot32::persistence {

/**
 * @class MemoryKeyValueStore
 * @brief IKeyValueStore over a fixed in-RAM table; for tests and volatile use.
 *
 * Holds up to @p Entries keys, each with an integer or a blob of up to
 * @p BlobBytes bytes. Integers are stored as 4 little-endian bytes. No heap;
 * contents are lost with the object. A set or erase applies in full or not at
 * all, so replace-or-keep holds trivially; commit() only counts.
 *
 * Test hooks (not part of IKeyValueStore): setFailWrites(), corruptByte(),
 * writes() and commits().
 *
 * @tparam Entries Maximum number of keys; a new key beyond it is NoSpace.
 * @tparam BlobBytes Largest blob accepted; a larger one is TooLarge.
 */
template <size_t Entries, size_t BlobBytes>
class MemoryKeyValueStore final : public IKeyValueStore {
    static_assert(Entries >= 1, "MemoryKeyValueStore needs at least one entry");
    static_assert(BlobBytes >= 1, "MemoryKeyValueStore needs at least one blob byte");

public:
    StoreResult getI32(const char* key, int32_t& out) override {
        uint32_t raw = 0;
        const StoreResult r = getInt(key, Type::I32, raw);
        if (r == StoreResult::Ok) {
            out = static_cast<int32_t>(raw);
        }
        return r;
    }

    StoreResult setI32(const char* key, int32_t value) override {
        return setInt(key, Type::I32, static_cast<uint32_t>(value));
    }

    StoreResult getU32(const char* key, uint32_t& out) override {
        return getInt(key, Type::U32, out);
    }

    StoreResult setU32(const char* key, uint32_t value) override {
        return setInt(key, Type::U32, value);
    }

    StoreResult getBlob(const char* key, uint8_t* out, size_t capacity,
                        size_t& stored) override {
        if (!isValidKey(key)) {
            return StoreResult::InvalidKey;
        }
        if (out == nullptr && capacity > 0) {
            return StoreResult::InvalidArgument;
        }
        const Entry* e = find(key);
        if (e == nullptr || e->type != Type::Blob) {
            return StoreResult::NotFound;
        }
        stored = e->size;
        if (e->size > capacity) {
            return StoreResult::TooLarge;
        }
        if (e->size > 0) {
            memcpy(out, e->data, e->size);
        }
        return StoreResult::Ok;
    }

    StoreResult setBlob(const char* key, const uint8_t* data, size_t size) override {
        if (!isValidKey(key)) {
            return StoreResult::InvalidKey;
        }
        if (data == nullptr && size > 0) {
            return StoreResult::InvalidArgument;
        }
        if (size > BlobBytes) {
            return StoreResult::TooLarge;
        }
        Entry* e = nullptr;
        const StoreResult r = slotFor(key, e);
        if (r != StoreResult::Ok) {
            return r;
        }
        if (size > 0) {
            memcpy(e->data, data, size);
        }
        e->size = size;
        e->type = Type::Blob;
        ++writes_;
        return StoreResult::Ok;
    }

    StoreResult erase(const char* key) override {
        if (!isValidKey(key)) {
            return StoreResult::InvalidKey;
        }
        if (failWrites_) {
            return StoreResult::IoError;
        }
        Entry* e = find(key);
        if (e == nullptr) {
            return StoreResult::NotFound;
        }
        *e = Entry{};
        ++writes_;
        return StoreResult::Ok;
    }

    StoreResult commit() override {
        if (failWrites_) {
            return StoreResult::IoError;
        }
        ++commits_;
        return StoreResult::Ok;
    }

    /**
     * @brief Makes every later set, erase and commit fail with IoError.
     *
     * Failing calls change nothing, so the previous value survives (S7).
     * Argument checks (InvalidKey, InvalidArgument, TooLarge) still run first.
     * Reads are unaffected.
     */
    void setFailWrites(bool fail) { failWrites_ = fail; }

    /**
     * @brief Flips every bit of one stored byte, simulating media damage.
     * @param key Key whose value is damaged; integers count as 4 bytes.
     * @param index Byte offset inside the stored value.
     * @return Ok; InvalidKey; NotFound for a missing key; InvalidArgument when
     *         @p index is past the stored size.
     */
    StoreResult corruptByte(const char* key, size_t index) {
        if (!isValidKey(key)) {
            return StoreResult::InvalidKey;
        }
        Entry* e = find(key);
        if (e == nullptr) {
            return StoreResult::NotFound;
        }
        if (index >= e->size) {
            return StoreResult::InvalidArgument;
        }
        e->data[index] = static_cast<uint8_t>(e->data[index] ^ 0xFFu);
        return StoreResult::Ok;
    }

    /** @brief Number of sets and erases that succeeded. */
    uint32_t writes() const { return writes_; }

    /** @brief Number of commit() calls that succeeded. */
    uint32_t commits() const { return commits_; }

private:
    enum class Type : uint8_t { Empty, I32, U32, Blob };

    static constexpr size_t kIntBytes = 4;
    static constexpr size_t kDataBytes = BlobBytes > kIntBytes ? BlobBytes : kIntBytes;

    struct Entry {
        char key[kMaxKeyLength + 1] = {};
        Type type = Type::Empty;
        size_t size = 0;
        uint8_t data[kDataBytes] = {};
    };

    Entry* find(const char* key) {
        for (Entry& e : entries_) {
            if (e.type != Type::Empty && strcmp(e.key, key) == 0) {
                return &e;
            }
        }
        return nullptr;
    }

    // Finds the entry a set of `key` writes to: the existing one, else a free
    // one (key copied in). Applies the fault hook before choosing.
    StoreResult slotFor(const char* key, Entry*& out) {
        if (failWrites_) {
            return StoreResult::IoError;
        }
        out = find(key);
        if (out != nullptr) {
            return StoreResult::Ok;
        }
        for (Entry& e : entries_) {
            if (e.type == Type::Empty) {
                memcpy(e.key, key, strlen(key) + 1);  // isValidKey bounds the length
                out = &e;
                return StoreResult::Ok;
            }
        }
        return StoreResult::NoSpace;
    }

    StoreResult getInt(const char* key, Type type, uint32_t& out) {
        if (!isValidKey(key)) {
            return StoreResult::InvalidKey;
        }
        const Entry* e = find(key);
        if (e == nullptr || e->type != type) {
            return StoreResult::NotFound;
        }
        out = static_cast<uint32_t>(e->data[0]) | (static_cast<uint32_t>(e->data[1]) << 8) |
              (static_cast<uint32_t>(e->data[2]) << 16) |
              (static_cast<uint32_t>(e->data[3]) << 24);
        return StoreResult::Ok;
    }

    StoreResult setInt(const char* key, Type type, uint32_t value) {
        if (!isValidKey(key)) {
            return StoreResult::InvalidKey;
        }
        Entry* e = nullptr;
        const StoreResult r = slotFor(key, e);
        if (r != StoreResult::Ok) {
            return r;
        }
        for (size_t i = 0; i < kIntBytes; ++i) {
            e->data[i] = static_cast<uint8_t>(value >> (8 * i));
        }
        e->size = kIntBytes;
        e->type = type;
        ++writes_;
        return StoreResult::Ok;
    }

    Entry entries_[Entries] = {};
    uint32_t writes_ = 0;
    uint32_t commits_ = 0;
    bool failWrites_ = false;
};

/**
 * @class MemorySlotStore
 * @brief ISlotStore over fixed in-RAM records; for tests and volatile use.
 *
 * @p Slots records of up to @p RecordBytes bytes each. No heap; contents are
 * lost with the object. A write applies in full or not at all, so
 * replace-or-keep holds trivially.
 *
 * Test hooks (not part of ISlotStore): setFailWrites(), corruptByte() and
 * writes().
 *
 * @tparam Slots Number of slots.
 * @tparam RecordBytes Largest record accepted; a larger one is TooLarge.
 */
template <uint16_t Slots, size_t RecordBytes>
class MemorySlotStore final : public ISlotStore {
    static_assert(Slots >= 1, "MemorySlotStore needs at least one slot");
    static_assert(RecordBytes >= 1, "MemorySlotStore needs at least one record byte");

public:
    uint16_t slotCount() const override { return Slots; }

    size_t maxRecordSize() const override { return RecordBytes; }

    StoreResult query(uint16_t slot, size_t& stored) override {
        if (slot >= Slots) {
            return StoreResult::InvalidArgument;
        }
        const Record& rec = records_[slot];
        stored = rec.used ? rec.size : 0;
        return rec.used ? StoreResult::Ok : StoreResult::NotFound;
    }

    StoreResult read(uint16_t slot, uint8_t* out, size_t capacity, size_t& stored) override {
        if (slot >= Slots || (out == nullptr && capacity > 0)) {
            return StoreResult::InvalidArgument;
        }
        const Record& rec = records_[slot];
        if (!rec.used) {
            stored = 0;
            return StoreResult::NotFound;
        }
        stored = rec.size;
        if (rec.size > capacity) {
            return StoreResult::TooLarge;
        }
        if (rec.size > 0) {
            memcpy(out, rec.data, rec.size);
        }
        return StoreResult::Ok;
    }

    StoreResult write(uint16_t slot, const uint8_t* data, size_t size) override {
        if (slot >= Slots || (data == nullptr && size > 0)) {
            return StoreResult::InvalidArgument;
        }
        if (size > RecordBytes) {
            return StoreResult::TooLarge;
        }
        if (failWrites_) {
            return StoreResult::IoError;
        }
        Record& rec = records_[slot];
        if (size > 0) {
            memcpy(rec.data, data, size);
        }
        rec.size = size;
        rec.used = true;
        ++writes_;
        return StoreResult::Ok;
    }

    StoreResult erase(uint16_t slot) override {
        if (slot >= Slots) {
            return StoreResult::InvalidArgument;
        }
        if (failWrites_) {
            return StoreResult::IoError;
        }
        Record& rec = records_[slot];
        if (!rec.used) {
            return StoreResult::NotFound;
        }
        rec = Record{};
        ++writes_;
        return StoreResult::Ok;
    }

    /**
     * @brief Makes every later write and erase fail with IoError.
     *
     * Failing calls change nothing, so the previous record survives (S7).
     * Argument checks (InvalidArgument, TooLarge) still run first. Reads are
     * unaffected.
     */
    void setFailWrites(bool fail) { failWrites_ = fail; }

    /**
     * @brief Flips every bit of one stored byte, simulating media damage.
     * @param slot Slot whose record is damaged.
     * @param index Byte offset inside the record.
     * @return Ok; NotFound for an empty slot; InvalidArgument for an
     *         out-of-range slot or an @p index past the record size.
     */
    StoreResult corruptByte(uint16_t slot, size_t index) {
        if (slot >= Slots) {
            return StoreResult::InvalidArgument;
        }
        Record& rec = records_[slot];
        if (!rec.used) {
            return StoreResult::NotFound;
        }
        if (index >= rec.size) {
            return StoreResult::InvalidArgument;
        }
        rec.data[index] = static_cast<uint8_t>(rec.data[index] ^ 0xFFu);
        return StoreResult::Ok;
    }

    /** @brief Number of writes and erases that succeeded. */
    uint32_t writes() const { return writes_; }

private:
    struct Record {
        bool used = false;
        size_t size = 0;
        uint8_t data[RecordBytes] = {};
    };

    Record records_[Slots] = {};
    uint32_t writes_ = 0;
    bool failWrites_ = false;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
