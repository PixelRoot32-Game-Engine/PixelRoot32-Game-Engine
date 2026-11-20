/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_SD_STORAGE && defined(ESP32)

#include "drivers/esp32/SdStore.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace pixelroot32::persistence {

namespace {

// Decimal spelling of a uint16_t index: at most "65535".
constexpr size_t kSlotDigitsMax = 5;
constexpr char kSlotSuffix[] = ".slot";
constexpr char kTmpSuffix[] = ".tmp";

// Bytes a slot file path needs after the store directory: '/' + the digits +
// ".slot" + ".tmp" + the NUL. init() reserves them for both names.
constexpr size_t kPathReserveBytes = 1 + kSlotDigitsMax + (sizeof(kSlotSuffix) - 1) +
                                     (sizeof(kTmpSuffix) - 1) + 1;

/** A pin is either "use the bus default" (-1) or a real pin. */
bool validBusPin(int pin) {
    return pin >= -1 && pin <= 127;
}

/** Maps a POSIX errno from the FS layer to StoreResult (S6). */
StoreResult fromErrno(int err) {
    return err == ENOSPC ? StoreResult::NoSpace : StoreResult::IoError;
}

/**
 * Opens @p path for reading and reports its size. A missing file is NotFound;
 * a file larger than any write is IoError (damage is visible, never a silent
 * empty slot). On Ok the caller owns @p file.
 */
StoreResult openRecord(const char* path, size_t maxRecordBytes, fs::File& file,
                       size_t& size) {
    fs::File f = SD.open(path, FILE_READ);
    if (!f) {
        // exists() separates a missing file from a failed open.
        return SD.exists(path) ? StoreResult::IoError : StoreResult::NotFound;
    }
    const size_t n = f.size();
    if (n > maxRecordBytes) {
        f.close();
        return StoreResult::IoError;
    }
    file = f;
    size = n;
    return StoreResult::Ok;
}

}  // namespace

SdSlotStore::SdSlotStore(const char* rootDir, const char* prefix, uint16_t slots,
                         size_t maxRecordBytes) noexcept
    : rootDir_(rootDir),
      prefix_(prefix),
      maxRecordBytes_(maxRecordBytes),
      slots_(slots) {}

SdSlotStore::~SdSlotStore() = default;

StoreResult SdSlotStore::init(const SdConfig& config) noexcept {
    if (ready_) {
        return StoreResult::Ok;
    }
    if (rootDir_ == nullptr || rootDir_[0] == '\0') {
        return StoreResult::InvalidArgument;
    }
    // The Arduino FS prepends its mountpoint and rejects paths without a
    // leading '/' (mkdir would silently build "<mountpoint>pr32" otherwise).
    if (rootDir_[0] != '/') {
        return StoreResult::InvalidArgument;
    }
    if (!isValidKey(prefix_)) {
        return StoreResult::InvalidKey;
    }
    if (slots_ == 0 || maxRecordBytes_ == 0) {
        return StoreResult::InvalidArgument;
    }
    if (config.spi == nullptr || config.cs < 0 || config.cs > 127 ||
        !validBusPin(config.sck) || !validBusPin(config.miso) ||
        !validBusPin(config.mosi)) {
        return StoreResult::InvalidArgument;
    }

    size_t rootLen = strlen(rootDir_);
    while (rootLen > 1 && rootDir_[rootLen - 1] == '/') {
        --rootLen;  // "<root>/" and "<root>" name the same directory.
    }
    const size_t sep = rootDir_[rootLen - 1] == '/' ? 0 : 1;  // Only "/" has none.
    const size_t prefixLen = strlen(prefix_);
    if (rootLen + sep + prefixLen + kPathReserveBytes > kSdMaxPathBytes) {
        return StoreResult::InvalidArgument;
    }

    // Bring up the bus with the configured pins before the driver probes the
    // card. SD.begin() calls spi.begin() too, but only with the bus defaults.
    config.spi->begin(static_cast<int8_t>(config.sck), static_cast<int8_t>(config.miso),
                      static_cast<int8_t>(config.mosi), static_cast<int8_t>(config.cs));
    // Never formats or erases: the default format_if_empty stays false.
    if (!SD.begin(static_cast<uint8_t>(config.cs), *config.spi, config.frequencyHz)) {
        return StoreResult::Unavailable;
    }

    // Create both levels. mkdir() on an existing directory is not an error on
    // the Arduino FS, so a card that already has the store is left untouched.
    char root[kSdMaxPathBytes];
    memcpy(root, rootDir_, rootLen);
    root[rootLen] = '\0';
    if (!SD.mkdir(root)) {
        return StoreResult::IoError;
    }
    char dir[kSdMaxPathBytes];
    size_t len = rootLen;
    memcpy(dir, root, rootLen);
    if (sep == 1) {
        dir[len++] = '/';
    }
    memcpy(dir + len, prefix_, prefixLen + 1);
    if (!SD.mkdir(dir)) {
        return StoreResult::IoError;
    }
    ready_ = true;
    return StoreResult::Ok;
}

StoreResult SdSlotStore::usable(uint16_t slot) const noexcept {
    // Unavailable wins over InvalidArgument: a store nobody mounted answers
    // the same for every slot, as ISlotStore documents. cardType() is a cached
    // field read, so it also catches a card unmounted after init().
    if (!ready_ || SD.cardType() == CARD_NONE) {
        return StoreResult::Unavailable;
    }
    return slot < slots_ ? StoreResult::Ok : StoreResult::InvalidArgument;
}

void SdSlotStore::filePath(uint16_t slot, char* out) const noexcept {
    size_t len = strlen(rootDir_);
    while (len > 1 && rootDir_[len - 1] == '/') {
        --len;
    }
    memcpy(out, rootDir_, len);
    if (out[len - 1] != '/') {
        out[len++] = '/';
    }
    const size_t prefixLen = strlen(prefix_);
    memcpy(out + len, prefix_, prefixLen);
    len += prefixLen;
    out[len++] = '/';
    // init() proved the remaining room for the digits, suffix and NUL.
    (void)snprintf(out + len, kSdMaxPathBytes - len, "%u%s", static_cast<unsigned>(slot),
                   kSlotSuffix);
}

StoreResult SdSlotStore::query(uint16_t slot, size_t& stored) {
    const StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    char path[kSdMaxPathBytes];
    filePath(slot, path);
    fs::File file;
    size_t size = 0;
    const StoreResult res = openRecord(path, maxRecordBytes_, file, size);
    if (res == StoreResult::NotFound) {
        stored = 0;
        return res;
    }
    if (res != StoreResult::Ok) {
        return res;
    }
    file.close();
    stored = size;
    return StoreResult::Ok;
}

StoreResult SdSlotStore::read(uint16_t slot, uint8_t* out, size_t capacity,
                              size_t& stored) {
    const StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (out == nullptr && capacity > 0) {
        return StoreResult::InvalidArgument;
    }
    char path[kSdMaxPathBytes];
    filePath(slot, path);
    fs::File file;
    size_t size = 0;
    const StoreResult res = openRecord(path, maxRecordBytes_, file, size);
    if (res == StoreResult::NotFound) {
        stored = 0;
        return res;
    }
    if (res != StoreResult::Ok) {
        return res;
    }
    if (size == 0) {
        file.close();
        stored = 0;
        return StoreResult::Ok;  // An empty record reads as Ok, size 0.
    }
    if (size > capacity) {
        file.close();
        stored = size;  // capacity 0 is the size query: TooLarge, nothing read.
        return StoreResult::TooLarge;
    }
    const size_t got = file.read(out, size);
    file.close();
    if (got != size) {
        return StoreResult::IoError;  // stored untouched, as the contract says.
    }
    stored = size;
    return StoreResult::Ok;
}

StoreResult SdSlotStore::write(uint16_t slot, const uint8_t* data, size_t size) {
    const StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (data == nullptr && size > 0) {
        return StoreResult::InvalidArgument;
    }
    if (size > maxRecordBytes_) {
        return StoreResult::TooLarge;
    }
    char path[kSdMaxPathBytes];
    filePath(slot, path);
    char tmp[kSdMaxPathBytes];
    const int n = snprintf(tmp, sizeof(tmp), "%s%s", path, kTmpSuffix);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(tmp)) {
        return StoreResult::InvalidArgument;  // Unreachable: init() checked.
    }

    errno = 0;
    fs::File file = SD.open(tmp, FILE_WRITE);  // "w": create or truncate.
    if (!file) {
        return fromErrno(errno);
    }
    bool ok = true;
    int err = 0;
    if (size > 0) {
        const size_t written = file.write(data, size);
        if (written != size) {
            ok = false;
            err = errno;  // ENOSPC on a full card.
        }
    }
    if (ok) {
        file.flush();  // The Arduino FS flush flushes stdio and fsyncs.
    }
    file.close();
    if (!ok) {
        (void)SD.remove(tmp);
        return fromErrno(err);
    }

    // FAT cannot rename over an existing destination, so the old record is
    // removed first; the pair is the accepted non-atomic replace on FAT.
    if (SD.exists(path) && !SD.remove(path)) {
        (void)SD.remove(tmp);
        return StoreResult::IoError;  // The old record is still there.
    }
    if (!SD.rename(tmp, path)) {
        (void)SD.remove(tmp);
        return fromErrno(errno);
    }
    return StoreResult::Ok;
}

StoreResult SdSlotStore::erase(uint16_t slot) {
    const StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    char path[kSdMaxPathBytes];
    filePath(slot, path);
    if (SD.remove(path)) {
        return StoreResult::Ok;
    }
    // remove() fails for a missing file too; exists() tells them apart.
    return SD.exists(path) ? StoreResult::IoError : StoreResult::NotFound;
}

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_SD_STORAGE && defined(ESP32)
