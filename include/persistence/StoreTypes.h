/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include <cstddef>
#include <cstdint>

namespace pixelroot32::persistence {

/**
 * @enum StoreResult
 * @brief Outcome of every persistence operation.
 *
 * Every store, adapter and codec call reports one of these values: the module
 * never answers with a bare bool and never throws, so a caller can always tell
 * a missing record from a full medium or a damaged one.
 */
enum class StoreResult : uint8_t {
    Ok,              ///< The operation completed.
    NotFound,        ///< The key or slot holds no record; outputs are untouched.
    TooLarge,        ///< The data does not fit the caller's buffer or the medium's limit.
    NoSpace,         ///< The medium is full.
    Corrupt,         ///< Stored bytes failed validation (envelope, length or CRC).
    IoError,         ///< The medium reported a failure the module cannot classify further.
    Unavailable,     ///< No store is bound, or the medium is not initialised.
    InvalidKey,      ///< The key or namespace breaks the rules of isValidKey().
    InvalidArgument  ///< A null pointer, out-of-range slot or other bad parameter.
};

/**
 * @brief Longest key or namespace accepted, in bytes.
 *
 * This is the ESP32 NVS limit, applied to every medium so a record written on
 * the host keeps the same key on the board.
 */
inline constexpr size_t kMaxKeyLength = 15;

/**
 * @brief Reports whether a key or namespace follows the shared key rules.
 *
 * A valid key is 1..kMaxKeyLength bytes, each in `[A-Za-z0-9_-]`. The set is
 * safe as a file name on Windows and POSIX, which matters because file-backed
 * adapters name files after keys. The scan stops after kMaxKeyLength + 1
 * bytes, so an unterminated or overlong buffer is rejected without reading
 * past that point.
 *
 * @param key Null-terminated key; nullptr is rejected.
 * @return true when the key is valid.
 */
inline bool isValidKey(const char* key) {
    if (key == nullptr) {
        return false;
    }
    for (size_t i = 0; i <= kMaxKeyLength; ++i) {
        const char c = key[i];
        if (c == '\0') {
            return i > 0;
        }
        const bool allowed = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                             (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!allowed || i == kMaxKeyLength) {
            return false;
        }
    }
    return false;
}

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
