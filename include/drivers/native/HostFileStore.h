/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE && defined(PLATFORM_NATIVE)

#include "persistence/KeyValueStore.h"
#include "persistence/SlotStore.h"
#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace pixelroot32::persistence {

/**
 * @brief Longest path the host file adapters build, in bytes, NUL included.
 *
 * The Windows MAX_PATH value, applied on every host so a root that works on
 * one machine works on the others. Paths live in fixed buffers of this size.
 */
inline constexpr size_t kHostFileMaxPathBytes = 260;

/**
 * @brief Bytes reserved after the store directory for its longest file path.
 *
 * One separator, the longest escaped key (2 * kMaxKeyLength, see
 * HostFileKeyValueStore) and the longest suffix (`.kv.tmp`). init() accepts a
 * root only when `strlen(root) + 1 + strlen(name) + kHostFileNameReserveBytes`
 * plus the NUL fits in kHostFileMaxPathBytes.
 */
inline constexpr size_t kHostFileNameReserveBytes = 1 + 2 * kMaxKeyLength + 7;

/**
 * @class HostFileKeyValueStore
 * @brief IKeyValueStore over one file per key, for native (PC) builds.
 *
 * Files live in `<rootDir>/<nameSpace>/`. A key's file name is the key with
 * each uppercase letter written as `^` plus its lowercase letter, then `.kv`
 * (`Ab_C` -> `^ab_^c.kv`): host file systems may ignore case, `^` is not a
 * key character, so distinct keys always get distinct files. A file holds a
 * one-byte type tag (1 = i32, 2 = u32, 3 = blob) followed by the value;
 * integers are 4 little-endian bytes.
 *
 * Replace-or-keep (S7): every set writes `<file>.tmp`, flushes and closes it,
 * then moves it over the final file in one step - `MoveFileExA` with
 * `MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH` on Windows, `rename()`
 * on POSIX. A crash leaves either the old file or the new one, plus at worst
 * a stale `.tmp` that reads never open and the next write overwrites.
 *
 * Errors: a missing file is NotFound; a full disk is NoSpace; any other I/O
 * failure is IoError. A file whose tag or size is malformed (empty,
 * truncated, unknown tag, blob above the store's maximum) is IoError on every
 * get - never a silent NotFound - so damage is visible; a set replaces such a
 * file and erase removes it. There is no envelope here: RecordCodec validates
 * records above this layer.
 *
 * init() must succeed before use; until then every operation, commit()
 * included, returns Unavailable without touching the disk. commit() returns
 * Ok because every set is already flushed and in place. The constructor
 * copies its strings, so they need not outlive it. No exceptions; the object
 * holds fixed-size buffers only (no heap of its own; the C library may
 * allocate inside its file calls on the host).
 *
 * Test hook (not part of IKeyValueStore): setFailBeforeReplace().
 */
class HostFileKeyValueStore final : public IKeyValueStore {
public:
    /**
     * @param rootDir Root directory; created with its parents by init().
     * @param nameSpace Sub-directory of this store; must pass isValidKey().
     * @param maxBlobBytes Largest blob setBlob() accepts; must be non-zero.
     */
    HostFileKeyValueStore(const char* rootDir, const char* nameSpace,
                          size_t maxBlobBytes) noexcept;

    /**
     * @brief Checks the arguments and creates the store directory.
     * @return Ok; InvalidArgument for a null or empty root, a zero maximum or
     *         a root too long for kHostFileMaxPathBytes; InvalidKey for a
     *         namespace that fails isValidKey(); IoError when the directory
     *         cannot be created or is not a directory. Nothing is created on
     *         an argument error. Calling it again on a ready store is Ok.
     */
    StoreResult init() noexcept;

    /** @brief Largest blob setBlob() accepts, in bytes. */
    size_t maxBlobSize() const noexcept { return maxBlobBytes_; }

    /**
     * @brief Test hook: simulates a crash between the temp write and the move.
     *
     * While on, every set writes and closes `<file>.tmp`, then returns IoError
     * without replacing the final file, which keeps its previous value. Argument
     * checks still come first; erase() is unaffected.
     */
    void setFailBeforeReplace(bool fail) noexcept { failBeforeReplace_ = fail; }

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
    void filePath(const char* key, char* out) const noexcept;
    StoreResult openValue(const char* key, std::FILE*& file, uint8_t& tag,
                          size_t& payload) const noexcept;
    StoreResult getInt(const char* key, uint8_t tag, uint32_t& out) noexcept;
    StoreResult setInt(const char* key, uint8_t tag, uint32_t value) noexcept;

    char dir_[kHostFileMaxPathBytes] = {};  ///< `<root>/<namespace>`.
    StoreResult argCheck_ = StoreResult::Ok;  ///< Constructor-time verdict for init().
    size_t maxBlobBytes_ = 0;
    bool ready_ = false;
    bool failBeforeReplace_ = false;
};

/**
 * @class HostFileSlotStore
 * @brief ISlotStore over one file per slot, for native (PC) builds.
 *
 * Files live in `<rootDir>/<prefix>/` and are named after the slot index in
 * decimal, then `.slot` (`2.slot`); a file holds the record bytes as they
 * are, and a missing file is an empty slot. Writes use the same temp file and
 * atomic move as HostFileKeyValueStore, so replace-or-keep (S7) holds the
 * same way, and the same error mapping applies: a file larger than
 * maxRecordSize() is IoError.
 *
 * init() must succeed before use; until then query(), read(), write() and
 * erase() return Unavailable, even for an out-of-range slot. slotCount() and
 * maxRecordSize() report the configuration at any time.
 *
 * Test hook (not part of ISlotStore): setFailBeforeReplace().
 */
class HostFileSlotStore final : public ISlotStore {
public:
    /**
     * @param rootDir Root directory; created with its parents by init().
     * @param prefix Sub-directory of this store; must pass isValidKey().
     * @param slots Number of slots; must be non-zero.
     * @param maxRecordBytes Largest record write() accepts; must be non-zero.
     */
    HostFileSlotStore(const char* rootDir, const char* prefix, uint16_t slots,
                      size_t maxRecordBytes) noexcept;

    /**
     * @brief Checks the arguments and creates the store directory.
     * @return As HostFileKeyValueStore::init(), with InvalidArgument also for
     *         zero slots and InvalidKey for an invalid prefix.
     */
    StoreResult init() noexcept;

    /** @brief Test hook; see HostFileKeyValueStore::setFailBeforeReplace(). */
    void setFailBeforeReplace(bool fail) noexcept { failBeforeReplace_ = fail; }

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
    StoreResult openRecord(uint16_t slot, std::FILE*& file, size_t& size) const noexcept;

    char dir_[kHostFileMaxPathBytes] = {};  ///< `<root>/<prefix>`.
    StoreResult argCheck_ = StoreResult::Ok;  ///< Constructor-time verdict for init().
    size_t maxRecordBytes_ = 0;
    uint16_t slots_ = 0;
    bool ready_ = false;
    bool failBeforeReplace_ = false;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE && defined(PLATFORM_NATIVE)
