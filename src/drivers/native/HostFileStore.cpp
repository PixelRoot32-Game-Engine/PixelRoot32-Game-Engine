/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE && defined(PLATFORM_NATIVE)

#include "drivers/native/HostFileStore.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#if defined(_WIN32)
// Only the file API is needed. NOUSER/NOGDI also keep winuser.h out, whose
// INPUT type collides with the Arduino mock's INPUT macro on native builds.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOUSER
#define NOUSER
#endif
#ifndef NOGDI
#define NOGDI
#endif
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace pixelroot32::persistence {

namespace {

// KV file type tags (first byte of every `.kv` file).
constexpr uint8_t kTagI32 = 1;
constexpr uint8_t kTagU32 = 2;
constexpr uint8_t kTagBlob = 3;
constexpr size_t kIntBytes = 4;

constexpr char kKvSuffix[] = ".kv";
constexpr char kSlotSuffix[] = ".slot";
constexpr char kTmpSuffix[] = ".tmp";

static_assert(kHostFileNameReserveBytes ==
                  1 + 2 * kMaxKeyLength + (sizeof(kKvSuffix) - 1) + (sizeof(kTmpSuffix) - 1),
              "the reserve must cover '/' + the longest escaped key + \".kv.tmp\"");
// A slot file name is at most 5 digits (uint16_t) + ".slot.tmp", which the
// reserve also covers.
static_assert(1 + 5 + (sizeof(kSlotSuffix) - 1) + (sizeof(kTmpSuffix) - 1) <=
                  kHostFileNameReserveBytes,
              "slot file names must fit the reserve");

StoreResult fromErrno(int err) {
    return err == ENOSPC ? StoreResult::NoSpace : StoreResult::IoError;
}

/**
 * Builds `<root>/<name>` into @p out after checking the shared init()
 * arguments; the verdict is what init() later reports.
 */
StoreResult composeDir(char* out, const char* root, const char* name) {
    out[0] = '\0';
    if (root == nullptr || root[0] == '\0') {
        return StoreResult::InvalidArgument;
    }
    if (!isValidKey(name)) {
        return StoreResult::InvalidKey;
    }
    const size_t rootLen = strlen(root);
    const size_t nameLen = strlen(name);
    // root + '/' + name + reserve + NUL must fit.
    if (rootLen + 1 + nameLen + kHostFileNameReserveBytes + 1 > kHostFileMaxPathBytes) {
        return StoreResult::InvalidArgument;
    }
    memcpy(out, root, rootLen);
    out[rootLen] = '/';
    memcpy(out + rootLen + 1, name, nameLen + 1);
    return StoreResult::Ok;
}

bool isDirectory(const char* path) {
#if defined(_WIN32)
    struct _stat st;
    return _stat(path, &st) == 0 && (st.st_mode & _S_IFDIR) != 0;
#else
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

void makeOneDir(const char* path) {
    // Errors are ignored: the directory may exist already, or the prefix may
    // be a drive ("C:"); makeDirs() checks the end result instead.
#if defined(_WIN32)
    (void)_mkdir(path);
#else
    (void)mkdir(path, 0777);
#endif
}

/** Creates @p dir and every missing parent, like `mkdir -p`. */
StoreResult makeDirs(const char* dir) {
    char buf[kHostFileMaxPathBytes];
    const size_t len = strlen(dir);
    memcpy(buf, dir, len + 1);
    for (size_t i = 1; i < len; ++i) {
        if (buf[i] == '/' || buf[i] == '\\') {
            const char sep = buf[i];
            buf[i] = '\0';
            makeOneDir(buf);
            buf[i] = sep;
        }
    }
    makeOneDir(buf);
    return isDirectory(buf) ? StoreResult::Ok : StoreResult::IoError;
}

/**
 * Opens @p path for reading and reports its size. A missing file is
 * NotFound; on Ok the caller owns @p file, positioned at the start.
 */
StoreResult openForRead(const char* path, FILE*& file, size_t& size) {
    errno = 0;
    FILE* f = fopen(path, "rb");
    if (f == nullptr) {
        return errno == ENOENT ? StoreResult::NotFound : StoreResult::IoError;
    }
    long end = -1;
    if (fseek(f, 0, SEEK_END) == 0) {
        end = ftell(f);
    }
    if (end < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return StoreResult::IoError;
    }
    file = f;
    size = static_cast<size_t>(end);
    return StoreResult::Ok;
}

/** Reads exactly @p size bytes, then closes @p file. */
StoreResult readAndClose(FILE* file, uint8_t* out, size_t size) {
    const bool ok = size == 0 || fread(out, 1, size, file) == size;
    fclose(file);
    return ok ? StoreResult::Ok : StoreResult::IoError;
}

bool syncToDisk(FILE* file) {
#if defined(_WIN32)
    return _commit(_fileno(file)) == 0;
#else
    return fsync(fileno(file)) == 0;
#endif
}

/** Moves @p tmp over @p final in one step, replacing an existing file. */
StoreResult replaceFile(const char* tmp, const char* finalPath) {
#if defined(_WIN32)
    if (MoveFileExA(tmp, finalPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0) {
        return StoreResult::Ok;
    }
    const DWORD err = GetLastError();
    return (err == ERROR_DISK_FULL || err == ERROR_HANDLE_DISK_FULL) ? StoreResult::NoSpace
                                                                     : StoreResult::IoError;
#else
    if (rename(tmp, finalPath) == 0) {
        return StoreResult::Ok;
    }
    return fromErrno(errno);
#endif
}

/**
 * Replace-or-keep write: @p head then @p data go to `<finalPath>.tmp`, which
 * is flushed, synced and closed, then moved over @p finalPath. On failure the
 * final file is untouched and the temp file is removed, except under the
 * @p failBeforeReplace test hook, which leaves it as a crash would.
 */
StoreResult writeReplace(const char* finalPath, const uint8_t* head, size_t headSize,
                         const uint8_t* data, size_t size, bool failBeforeReplace) {
    char tmp[kHostFileMaxPathBytes];
    const int n = snprintf(tmp, sizeof(tmp), "%s%s", finalPath, kTmpSuffix);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(tmp)) {
        return StoreResult::InvalidArgument;  // Unreachable: init() checked the length.
    }
    errno = 0;
    FILE* f = fopen(tmp, "wb");
    if (f == nullptr) {
        return fromErrno(errno);
    }
    bool ok = (headSize == 0 || fwrite(head, 1, headSize, f) == headSize) &&
              (size == 0 || fwrite(data, 1, size, f) == size) && fflush(f) == 0 &&
              syncToDisk(f);
    int err = ok ? 0 : errno;
    if (fclose(f) != 0 && ok) {
        ok = false;
        err = errno;
    }
    if (!ok) {
        (void)remove(tmp);
        return fromErrno(err);
    }
    if (failBeforeReplace) {
        return StoreResult::IoError;
    }
    const StoreResult r = replaceFile(tmp, finalPath);
    if (r != StoreResult::Ok) {
        (void)remove(tmp);
    }
    return r;
}

StoreResult removeFile(const char* path) {
    errno = 0;
    if (remove(path) == 0) {
        return StoreResult::Ok;
    }
    return errno == ENOENT ? StoreResult::NotFound : StoreResult::IoError;
}

void storeLe32(uint8_t* out, uint32_t v) {
    out[0] = static_cast<uint8_t>(v);
    out[1] = static_cast<uint8_t>(v >> 8);
    out[2] = static_cast<uint8_t>(v >> 16);
    out[3] = static_cast<uint8_t>(v >> 24);
}

uint32_t loadLe32(const uint8_t* in) {
    return static_cast<uint32_t>(in[0]) | (static_cast<uint32_t>(in[1]) << 8) |
           (static_cast<uint32_t>(in[2]) << 16) | (static_cast<uint32_t>(in[3]) << 24);
}

}  // namespace

// =============================================================================
// HostFileKeyValueStore
// =============================================================================

HostFileKeyValueStore::HostFileKeyValueStore(const char* rootDir, const char* nameSpace,
                                             size_t maxBlobBytes) noexcept
    : maxBlobBytes_(maxBlobBytes) {
    argCheck_ = composeDir(dir_, rootDir, nameSpace);
    if (argCheck_ == StoreResult::Ok && maxBlobBytes == 0) {
        argCheck_ = StoreResult::InvalidArgument;
    }
}

StoreResult HostFileKeyValueStore::init() noexcept {
    if (argCheck_ != StoreResult::Ok) {
        return argCheck_;
    }
    const StoreResult r = makeDirs(dir_);
    ready_ = r == StoreResult::Ok;
    return r;
}

StoreResult HostFileKeyValueStore::usable(const char* key) const noexcept {
    if (!ready_) {
        return StoreResult::Unavailable;
    }
    return isValidKey(key) ? StoreResult::Ok : StoreResult::InvalidKey;
}

void HostFileKeyValueStore::filePath(const char* key, char* out) const noexcept {
    size_t len = strlen(dir_);
    memcpy(out, dir_, len);
    out[len++] = '/';
    for (const char* c = key; *c != '\0'; ++c) {
        if (*c >= 'A' && *c <= 'Z') {
            out[len++] = '^';
            out[len++] = static_cast<char>(*c - 'A' + 'a');
        } else {
            out[len++] = *c;
        }
    }
    memcpy(out + len, kKvSuffix, sizeof(kKvSuffix));
}

StoreResult HostFileKeyValueStore::openValue(const char* key, FILE*& file, uint8_t& tag,
                                             size_t& payload) const noexcept {
    char path[kHostFileMaxPathBytes];
    filePath(key, path);
    FILE* f = nullptr;
    size_t size = 0;
    const StoreResult r = openForRead(path, f, size);
    if (r != StoreResult::Ok) {
        return r;
    }
    uint8_t t = 0;
    bool valid = size >= 1 && fread(&t, 1, 1, f) == 1;
    if (valid) {
        const size_t body = size - 1;
        valid = ((t == kTagI32 || t == kTagU32) && body == kIntBytes) ||
                (t == kTagBlob && body <= maxBlobBytes_);
    }
    if (!valid) {
        fclose(f);
        return StoreResult::IoError;  // Damaged file: never a silent NotFound.
    }
    file = f;
    tag = t;
    payload = size - 1;
    return StoreResult::Ok;
}

StoreResult HostFileKeyValueStore::getInt(const char* key, uint8_t tag,
                                          uint32_t& out) noexcept {
    StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    FILE* f = nullptr;
    uint8_t t = 0;
    size_t payload = 0;
    r = openValue(key, f, t, payload);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (t != tag) {
        fclose(f);
        return StoreResult::NotFound;
    }
    uint8_t bytes[kIntBytes];
    r = readAndClose(f, bytes, sizeof(bytes));
    if (r == StoreResult::Ok) {
        out = loadLe32(bytes);
    }
    return r;
}

StoreResult HostFileKeyValueStore::setInt(const char* key, uint8_t tag,
                                          uint32_t value) noexcept {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    char path[kHostFileMaxPathBytes];
    filePath(key, path);
    uint8_t bytes[kIntBytes];
    storeLe32(bytes, value);
    return writeReplace(path, &tag, 1, bytes, sizeof(bytes), failBeforeReplace_);
}

StoreResult HostFileKeyValueStore::getI32(const char* key, int32_t& out) {
    uint32_t raw = 0;
    const StoreResult r = getInt(key, kTagI32, raw);
    if (r == StoreResult::Ok) {
        out = static_cast<int32_t>(raw);
    }
    return r;
}

StoreResult HostFileKeyValueStore::setI32(const char* key, int32_t value) {
    return setInt(key, kTagI32, static_cast<uint32_t>(value));
}

StoreResult HostFileKeyValueStore::getU32(const char* key, uint32_t& out) {
    return getInt(key, kTagU32, out);
}

StoreResult HostFileKeyValueStore::setU32(const char* key, uint32_t value) {
    return setInt(key, kTagU32, value);
}

StoreResult HostFileKeyValueStore::getBlob(const char* key, uint8_t* out, size_t capacity,
                                           size_t& stored) {
    StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (out == nullptr && capacity > 0) {
        return StoreResult::InvalidArgument;
    }
    FILE* f = nullptr;
    uint8_t t = 0;
    size_t payload = 0;
    r = openValue(key, f, t, payload);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (t != kTagBlob) {
        fclose(f);
        return StoreResult::NotFound;
    }
    if (payload > capacity) {
        fclose(f);
        stored = payload;
        return StoreResult::TooLarge;
    }
    r = readAndClose(f, out, payload);
    if (r == StoreResult::Ok) {
        stored = payload;
    }
    return r;
}

StoreResult HostFileKeyValueStore::setBlob(const char* key, const uint8_t* data,
                                           size_t size) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (data == nullptr && size > 0) {
        return StoreResult::InvalidArgument;
    }
    if (size > maxBlobBytes_) {
        return StoreResult::TooLarge;
    }
    char path[kHostFileMaxPathBytes];
    filePath(key, path);
    const uint8_t tag = kTagBlob;
    return writeReplace(path, &tag, 1, data, size, failBeforeReplace_);
}

StoreResult HostFileKeyValueStore::erase(const char* key) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    char path[kHostFileMaxPathBytes];
    filePath(key, path);
    return removeFile(path);
}

StoreResult HostFileKeyValueStore::commit() {
    // Every set is already synced and moved into place.
    return ready_ ? StoreResult::Ok : StoreResult::Unavailable;
}

// =============================================================================
// HostFileSlotStore
// =============================================================================

HostFileSlotStore::HostFileSlotStore(const char* rootDir, const char* prefix, uint16_t slots,
                                     size_t maxRecordBytes) noexcept
    : maxRecordBytes_(maxRecordBytes), slots_(slots) {
    argCheck_ = composeDir(dir_, rootDir, prefix);
    if (argCheck_ == StoreResult::Ok && (slots == 0 || maxRecordBytes == 0)) {
        argCheck_ = StoreResult::InvalidArgument;
    }
}

StoreResult HostFileSlotStore::init() noexcept {
    if (argCheck_ != StoreResult::Ok) {
        return argCheck_;
    }
    const StoreResult r = makeDirs(dir_);
    ready_ = r == StoreResult::Ok;
    return r;
}

StoreResult HostFileSlotStore::usable(uint16_t slot) const noexcept {
    if (!ready_) {
        return StoreResult::Unavailable;
    }
    return slot < slots_ ? StoreResult::Ok : StoreResult::InvalidArgument;
}

void HostFileSlotStore::filePath(uint16_t slot, char* out) const noexcept {
    snprintf(out, kHostFileMaxPathBytes, "%s/%u%s", dir_, static_cast<unsigned>(slot),
             kSlotSuffix);
}

StoreResult HostFileSlotStore::openRecord(uint16_t slot, FILE*& file,
                                          size_t& size) const noexcept {
    char path[kHostFileMaxPathBytes];
    filePath(slot, path);
    FILE* f = nullptr;
    size_t n = 0;
    const StoreResult r = openForRead(path, f, n);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (n > maxRecordBytes_) {
        fclose(f);
        return StoreResult::IoError;  // Damaged file: larger than any write.
    }
    file = f;
    size = n;
    return StoreResult::Ok;
}

StoreResult HostFileSlotStore::query(uint16_t slot, size_t& stored) {
    StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    FILE* f = nullptr;
    size_t size = 0;
    r = openRecord(slot, f, size);
    if (r == StoreResult::NotFound) {
        stored = 0;
    }
    if (r != StoreResult::Ok) {
        return r;
    }
    fclose(f);
    stored = size;
    return StoreResult::Ok;
}

StoreResult HostFileSlotStore::read(uint16_t slot, uint8_t* out, size_t capacity,
                                    size_t& stored) {
    StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (out == nullptr && capacity > 0) {
        return StoreResult::InvalidArgument;
    }
    FILE* f = nullptr;
    size_t size = 0;
    r = openRecord(slot, f, size);
    if (r == StoreResult::NotFound) {
        stored = 0;
    }
    if (r != StoreResult::Ok) {
        return r;
    }
    if (size > capacity) {
        fclose(f);
        stored = size;
        return StoreResult::TooLarge;
    }
    r = readAndClose(f, out, size);
    if (r == StoreResult::Ok) {
        stored = size;
    }
    return r;
}

StoreResult HostFileSlotStore::write(uint16_t slot, const uint8_t* data, size_t size) {
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
    char path[kHostFileMaxPathBytes];
    filePath(slot, path);
    return writeReplace(path, nullptr, 0, data, size, failBeforeReplace_);
}

StoreResult HostFileSlotStore::erase(uint16_t slot) {
    const StoreResult r = usable(slot);
    if (r != StoreResult::Ok) {
        return r;
    }
    char path[kHostFileMaxPathBytes];
    filePath(slot, path);
    return removeFile(path);
}

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE && defined(PLATFORM_NATIVE)
