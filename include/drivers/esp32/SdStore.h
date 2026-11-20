/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_SD_STORAGE && defined(ESP32)

#include "drivers/esp32/SdConfig.h"
#include "persistence/SlotStore.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>

namespace pixelroot32::persistence {

/**
 * @brief Longest path SdSlotStore builds, in bytes, NUL included.
 *
 * Paths are FAT paths relative to the SD mountpoint; one slot file plus its
 * `.tmp` sibling must fit, so init() rejects a root directory and prefix whose
 * combined length leaves no room. Paths live in fixed stack buffers for the
 * duration of a call; the store object itself holds no path buffer.
 */
inline constexpr size_t kSdMaxPathBytes = 160;

/**
 * @class SdSlotStore
 * @brief ISlotStore over one file per slot on a FAT microSD card.
 *
 * Files live in `<rootDir>/<prefix>/` and are named after the slot index in
 * decimal, then `.slot` (`2.slot`); a missing file is an empty slot. `rootDir`
 * is an SD path relative to the mountpoint the Arduino `SD` library uses
 * (`/sd` by default), so root `"/pr32"` maps to `<mountpoint>/pr32` on the
 * card; `prefix` names a sub-directory of it. The two strings are held by
 * pointer and must outlive the object, exactly like the NVS adapter's prefix
 * (string literals are the intended use).
 *
 * init(config) is non-destructive: it brings up the SPI bus named by
 * @p config, mounts the card with `SD.begin(cs, spi, frequencyHz)` and creates
 * the two directories (`mkdir` on an existing directory is not an error). It
 * never formats, erases or repairs the card. Until init() returns Ok, every
 * operation - including out-of-range and null arguments - returns Unavailable,
 * so a game can boot, fail to mount and keep running. Calling init() again on
 * a ready store is Ok.
 *
 * Replace-or-keep (S5): every write goes to `<file>.tmp`, which is flushed
 * (the Arduino FS flush calls `fsync`) and closed, then the old `<file>` is
 * removed and `SD.rename(tmp, file)` moves the new bytes into place. FAT
 * cannot rename over an existing destination, so the removal is required, and
 * that pair is the accepted non-atomic replace on FAT: a power loss before the
 * removal keeps the old record, a loss during the rename gives the new record
 * or a torn one that the RecordCodec envelope rejects, and a loss in between
 * leaves an empty slot - the limit FAT imposes. A failed write removes the
 * stray `.tmp`; an early failure leaves the previous record untouched.
 *
 * Errors (S6): a missing file is NotFound; a full card is NoSpace; a file
 * larger than maxRecordSize(), a malformed file or any other I/O failure is
 * IoError (never a silent NotFound, so damage is visible); an uninitialised or
 * unmounted card is Unavailable; an out-of-range slot or a null buffer with a
 * non-zero capacity is InvalidArgument; a record above maxRecordSize() is
 * TooLarge. There is no commit(): every write is flushed and in place.
 *
 * Memory: no heap of its own, fixed buffers on the call stack only; two
 * pointer members and the limits. Calls come from one task (the game loop);
 * the adapter is not thread-safe. The FATFS driver the Arduino SD library
 * brings in costs several KB of RAM.
 */
class SdSlotStore final : public ISlotStore {
public:
    /**
     * @param rootDir Directory under the SD mountpoint; non-empty, starts with
     *        '/' and must outlive the object.
     * @param prefix Sub-directory of this store; must pass isValidKey() and
     *        must outlive the object.
     * @param slots Number of slots; must be non-zero.
     * @param maxRecordBytes Largest record write() accepts; must be non-zero.
     */
    SdSlotStore(const char* rootDir, const char* prefix, uint16_t slots,
                size_t maxRecordBytes) noexcept;
    ~SdSlotStore() override;

    SdSlotStore(const SdSlotStore&) = delete;
    SdSlotStore& operator=(const SdSlotStore&) = delete;

    /**
     * @brief Checks the arguments, mounts the card and creates the directory.
     * @param config SPI bus, pins and clock; `spi` must be non-null and `cs`
     *        a valid pin.
     * @return Ok (also when already ready); InvalidArgument for a null or
     *         empty root, a root without a leading '/', zero slots, a zero
     *         maximum, a null bus, a negative CS or any pin outside -1..127,
     *         or an over-long path; InvalidKey for a prefix that fails
     *         isValidKey(); Unavailable when the card cannot be mounted.
     *         Nothing is created or mounted on an argument error.
     */
    StoreResult init(const SdConfig& config) noexcept;

    uint16_t slotCount() const override { return slots_; }
    size_t maxRecordSize() const override { return maxRecordBytes_; }
    StoreResult query(uint16_t slot, size_t& stored) override;
    StoreResult read(uint16_t slot, uint8_t* out, size_t capacity,
                     size_t& stored) override;
    StoreResult write(uint16_t slot, const uint8_t* data, size_t size) override;
    StoreResult erase(uint16_t slot) override;

private:
    StoreResult usable(uint16_t slot) const noexcept;
    void filePath(uint16_t slot, char* out) const noexcept;

    const char* rootDir_;
    const char* prefix_;
    size_t maxRecordBytes_;
    uint16_t slots_;
    bool ready_ = false;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_SD_STORAGE && defined(ESP32)
