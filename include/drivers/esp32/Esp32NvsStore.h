/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE && defined(ESP32)

#include "persistence/KeyValueStore.h"
#include "persistence/SlotStore.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>

namespace pixelroot32::persistence {

/**
 * @brief Default NVS partition label: the one the bootloader's table and the
 * Arduino core (`Preferences`) use.
 */
inline constexpr const char* kNvsDefaultPartition = "nvs";

/**
 * @class Esp32NvsKeyValueStore
 * @brief IKeyValueStore over one ESP-IDF NVS namespace (`nvs.h`).
 *
 * Integers are stored as NVS `I32` / `U32` items and blobs as NVS blobs, so a
 * blob written by Arduino `Preferences::putBytes(key, ...)` in the same
 * namespace reads back byte-identically through getBlob() (S13), and the
 * other way round.
 *
 * One value per key: NVS itself can hold one item per type under the same
 * key (measured on IDF 4.4.7: after `nvs_set_i32` then `nvs_set_blob` on one
 * key both succeed, `nvs_get_i32` still returns the old integer and the blob
 * length query answers NOT_FOUND), so before a set the adapter probes the key
 * with cheap reads and, when an item of another type is there, erases it
 * (`nvs_erase_key`, repeated until the key is gone) before writing the new
 * one. A same-type set is a single NVS
 * write, which is replace-or-keep: NVS writes the new entry before it marks
 * the old one erased. A type change is not: power lost between the erase and
 * the write loses the key. erase() removes every item under the key.
 *
 * Errors (S12): `ESP_ERR_NVS_NOT_FOUND` and `ESP_ERR_NVS_TYPE_MISMATCH` are
 * NotFound; `ESP_ERR_NVS_NOT_ENOUGH_SPACE` is NoSpace; `ESP_ERR_NVS_KEY_TOO_LONG`
 * and `ESP_ERR_NVS_INVALID_NAME` are InvalidKey; `ESP_ERR_NVS_INVALID_LENGTH`
 * and `ESP_ERR_NVS_VALUE_TOO_LONG` are TooLarge; `ESP_ERR_NVS_NOT_INITIALIZED`
 * and `ESP_ERR_NVS_INVALID_HANDLE` are Unavailable; anything else is IoError.
 * Keys are checked with isValidKey() before NVS sees them, so the key errors
 * are only a safety net.
 *
 * init() never erases. It initialises the partition when nobody has yet
 * (`nvs_flash_init_partition`; a partition the Arduino core or the game
 * already initialised is fine) and opens the namespace read-write. A
 * partition NVS cannot use as it is - `ESP_ERR_NVS_NO_FREE_PAGES` or
 * `ESP_ERR_NVS_NEW_VERSION_FOUND` - is reported as Corrupt and left alone:
 * erasing it would wipe every namespace on it, which is the game's decision
 * (`nvs_flash_erase_partition`), never the engine's.
 *
 * Until init() returns Ok every operation, commit() included, returns
 * Unavailable without touching flash. The destructor closes the handle.
 *
 * Writes (N4): sets and erases reach flash when they are made (NVS has no
 * write-back cache), and commit() calls `nvs_commit`; the game calls both at a
 * safe point. Any NVS write or page erase disables the flash cache on both
 * cores, stalling every task that runs from flash - render and audio included
 * - until it ends.
 *
 * Memory: the object holds a handle, two string pointers and the limits; no
 * buffers, no heap. The namespace and partition strings are used by init()
 * only, but must stay valid until it has run (string literals are the
 * intended use). Calls come from one task.
 */
class Esp32NvsKeyValueStore final : public IKeyValueStore {
public:
    /**
     * @param nameSpace NVS namespace; must pass isValidKey().
     * @param maxBlobBytes Largest blob setBlob() accepts; must be non-zero.
     * @param partition NVS partition label; non-empty.
     */
    Esp32NvsKeyValueStore(const char* nameSpace, size_t maxBlobBytes,
                          const char* partition = kNvsDefaultPartition) noexcept;
    ~Esp32NvsKeyValueStore() override;

    Esp32NvsKeyValueStore(const Esp32NvsKeyValueStore&) = delete;
    Esp32NvsKeyValueStore& operator=(const Esp32NvsKeyValueStore&) = delete;

    /**
     * @brief Initialises the partition if needed and opens the namespace.
     * @return Ok (also when already open); InvalidKey for an invalid
     *         namespace; InvalidArgument for a zero maximum or a null/empty
     *         partition label; Corrupt when the partition needs an erase;
     *         IoError for any other NVS failure (missing partition included).
     */
    StoreResult init() noexcept;

    /** @brief Largest blob setBlob() accepts, in bytes. */
    size_t maxBlobSize() const noexcept { return maxBlobBytes_; }

    StoreResult getI32(const char* key, int32_t& out) override;
    StoreResult setI32(const char* key, int32_t value) override;
    StoreResult getU32(const char* key, uint32_t& out) override;
    StoreResult setU32(const char* key, uint32_t value) override;
    StoreResult getBlob(const char* key, uint8_t* out, size_t capacity,
                        size_t& stored) override;
    StoreResult setBlob(const char* key, const uint8_t* data, size_t size) override;
    StoreResult erase(const char* key) override;
    StoreResult commit() override;

private:
    StoreResult usable(const char* key) const noexcept;

    const char* nameSpace_;
    const char* partition_;
    size_t maxBlobBytes_;
    uint32_t handle_ = 0;  ///< `nvs_handle_t` (uint32_t on IDF 4.4 and 5.x).
    bool ready_ = false;
};

/**
 * @class Esp32NvsSlotStore
 * @brief ISlotStore over one ESP-IDF NVS namespace: one blob per slot.
 *
 * Slot `i` is the blob under key `<prefix><i>` (decimal, `save0`, `save1`,
 * ...). init() checks that the longest key, prefix plus the digits of
 * `slots - 1`, fits kMaxKeyLength. An empty slot is a missing key.
 *
 * Replace-or-keep (S7): a write is one `nvs_set_blob`, and NVS writes the new
 * blob (its data chunks, then its index entry) before it erases the old one,
 * so after a power loss read() returns the old record or the new one. If the
 * key holds an item of another type (not written by this store), it is erased
 * first, as in Esp32NvsKeyValueStore.
 *
 * Errors, init(), Unavailable-before-init, writes and memory as
 * Esp32NvsKeyValueStore; the prefix string is used on every call and must
 * outlive the object. slotCount() and maxRecordSize() report the
 * configuration at any time; query(), read(), write() and erase() return
 * Unavailable before init(), even for an out-of-range slot.
 */
class Esp32NvsSlotStore final : public ISlotStore {
public:
    /**
     * @param nameSpace NVS namespace; must pass isValidKey().
     * @param prefix Key prefix; must pass isValidKey() and leave room for the
     *        slot digits.
     * @param slots Number of slots; must be non-zero.
     * @param maxRecordBytes Largest record write() accepts; must be non-zero.
     * @param partition NVS partition label; non-empty.
     */
    Esp32NvsSlotStore(const char* nameSpace, const char* prefix, uint16_t slots,
                      size_t maxRecordBytes,
                      const char* partition = kNvsDefaultPartition) noexcept;
    ~Esp32NvsSlotStore() override;

    Esp32NvsSlotStore(const Esp32NvsSlotStore&) = delete;
    Esp32NvsSlotStore& operator=(const Esp32NvsSlotStore&) = delete;

    /**
     * @brief As Esp32NvsKeyValueStore::init(), with InvalidArgument also for
     *        zero slots and InvalidKey for an invalid prefix or one too long
     *        for the slot digits.
     */
    StoreResult init() noexcept;

    uint16_t slotCount() const override { return slots_; }
    size_t maxRecordSize() const override { return maxRecordBytes_; }
    StoreResult query(uint16_t slot, size_t& stored) override;
    StoreResult read(uint16_t slot, uint8_t* out, size_t capacity,
                     size_t& stored) override;
    StoreResult write(uint16_t slot, const uint8_t* data, size_t size) override;
    StoreResult erase(uint16_t slot) override;

private:
    StoreResult usable(uint16_t slot, char* key) const noexcept;

    const char* nameSpace_;
    const char* prefix_;
    const char* partition_;
    size_t maxRecordBytes_;
    uint32_t handle_ = 0;  ///< `nvs_handle_t`.
    uint16_t slots_;
    bool ready_ = false;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE && defined(ESP32)
