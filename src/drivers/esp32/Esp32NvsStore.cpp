/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE && defined(ESP32)

#include "drivers/esp32/Esp32NvsStore.h"

#include <nvs.h>
#include <nvs_flash.h>

#include <cstdio>
#include <cstring>
#include <type_traits>

namespace pixelroot32::persistence {

static_assert(std::is_same<nvs_handle_t, uint32_t>::value,
              "the stores keep the NVS handle as uint32_t");

namespace {

// Bound on the erase loop: NVS can hold one item per type under a key, and the
// adapter only ever writes three types (i32, u32, blob). One spare round.
constexpr int kMaxItemsPerKey = 4;

/** Maps an ESP-IDF NVS error to StoreResult (S12). */
StoreResult fromEsp(esp_err_t err) {
    switch (err) {
        case ESP_OK:
            return StoreResult::Ok;
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_ERR_NVS_TYPE_MISMATCH:
            return StoreResult::NotFound;
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            return StoreResult::NoSpace;
        case ESP_ERR_NVS_KEY_TOO_LONG:
        case ESP_ERR_NVS_INVALID_NAME:
            return StoreResult::InvalidKey;
        case ESP_ERR_NVS_INVALID_LENGTH:
        case ESP_ERR_NVS_VALUE_TOO_LONG:
            return StoreResult::TooLarge;
        case ESP_ERR_NVS_NOT_INITIALIZED:
        case ESP_ERR_NVS_INVALID_HANDLE:
            return StoreResult::Unavailable;
        case ESP_ERR_NVS_NO_FREE_PAGES:
        case ESP_ERR_NVS_NEW_VERSION_FOUND:
            return StoreResult::Corrupt;
        default:
            return StoreResult::IoError;
    }
}

enum class ItemKind : uint8_t { I32, U32, Blob };

/**
 * Probes @p key for an item of @p kind without reading its value: ESP_OK when
 * present, ESP_ERR_NVS_NOT_FOUND when absent, ESP_ERR_NVS_TYPE_MISMATCH when
 * NVS found the key under another type first.
 */
esp_err_t probe(nvs_handle_t h, const char* key, ItemKind kind) {
    switch (kind) {
        case ItemKind::I32: {
            int32_t v = 0;
            return nvs_get_i32(h, key, &v);
        }
        case ItemKind::U32: {
            uint32_t v = 0;
            return nvs_get_u32(h, key, &v);
        }
        case ItemKind::Blob:
        default: {
            size_t len = 0;
            return nvs_get_blob(h, key, nullptr, &len);
        }
    }
}

/** Erases every item stored under @p key, whatever its type. */
esp_err_t eraseAllItems(nvs_handle_t h, const char* key) {
    esp_err_t first = nvs_erase_key(h, key);
    if (first != ESP_OK) {
        return first;
    }
    for (int i = 1; i < kMaxItemsPerKey; ++i) {
        const esp_err_t err = nvs_erase_key(h, key);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            return ESP_OK;
        }
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

/**
 * Keeps one value per key: before an item of @p kind is written, removes any
 * item of another type under @p key. Same-type items are left for the write
 * to replace in place (replace-or-keep).
 */
esp_err_t clearOtherTypes(nvs_handle_t h, const char* key, ItemKind kind) {
    const esp_err_t own = probe(h, key, kind);
    if (own == ESP_OK) {
        return ESP_OK;  // Same type: the set replaces it.
    }
    bool other = own == ESP_ERR_NVS_TYPE_MISMATCH;
    if (!other && own != ESP_ERR_NVS_NOT_FOUND) {
        return own;
    }
    // NOT_FOUND for this type does not prove the key is free: measured on
    // IDF 4.4.7 (tools/persist_nvs_smoke), NVS keeps one item per type under a
    // key, and a get of an absent type is NOT_FOUND, not TYPE_MISMATCH. So the
    // other two types are probed too.
    for (ItemKind k : {ItemKind::I32, ItemKind::U32, ItemKind::Blob}) {
        if (other || k == kind) {
            continue;
        }
        const esp_err_t err = probe(h, key, k);
        if (err == ESP_OK || err == ESP_ERR_NVS_TYPE_MISMATCH) {
            other = true;
        } else if (err != ESP_ERR_NVS_NOT_FOUND) {
            return err;
        }
    }
    return other ? eraseAllItems(h, key) : ESP_OK;
}

/** Initialises @p partition if nobody has yet, then opens @p nameSpace. */
StoreResult openNamespace(const char* partition, const char* nameSpace, uint32_t& handle) {
    // A partition the Arduino core or the game already initialised returns
    // ESP_OK here. Nothing is ever erased: a partition that needs an erase
    // (NO_FREE_PAGES / NEW_VERSION_FOUND) is reported as Corrupt.
    esp_err_t err = nvs_flash_init_partition(partition);
    if (err != ESP_OK) {
        const StoreResult r = fromEsp(err);
        return r == StoreResult::Corrupt ? r : StoreResult::IoError;
    }
    nvs_handle_t h = 0;
    err = nvs_open_from_partition(partition, nameSpace, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        const StoreResult r = fromEsp(err);
        return r == StoreResult::Corrupt || r == StoreResult::InvalidKey ? r
                                                                         : StoreResult::IoError;
    }
    handle = h;
    return StoreResult::Ok;
}

bool validPartition(const char* partition) {
    return partition != nullptr && partition[0] != '\0';
}

/** Blob read shared by getBlob() and slot read(): size first, then bytes. */
StoreResult readBlob(nvs_handle_t h, const char* key, uint8_t* out, size_t capacity,
                     size_t& stored) {
    size_t len = 0;
    esp_err_t err = nvs_get_blob(h, key, nullptr, &len);
    if (err != ESP_OK) {
        return fromEsp(err);
    }
    if (len > capacity) {
        stored = len;
        return StoreResult::TooLarge;  // Nothing written to out.
    }
    if (len > 0) {
        size_t got = len;  // Exactly the stored size: nothing past it is written.
        err = nvs_get_blob(h, key, out, &got);
        if (err != ESP_OK) {
            return fromEsp(err);
        }
        len = got;
    }
    stored = len;
    return StoreResult::Ok;
}

StoreResult writeBlob(nvs_handle_t h, const char* key, const uint8_t* data, size_t size) {
    esp_err_t err = clearOtherTypes(h, key, ItemKind::Blob);
    if (err == ESP_OK) {
        // An empty blob may arrive with a null pointer. IDF 4.4 accepts that,
        // but its argument checks are not part of the API contract, so a
        // non-null pointer is passed anyway; with size 0 NVS reads nothing
        // from it, and the key serves (no static byte, no RAM).
        const void* bytes = size > 0 ? static_cast<const void*>(data) : key;
        err = nvs_set_blob(h, key, bytes, size);
    }
    return fromEsp(err);
}

StoreResult eraseKey(nvs_handle_t h, const char* key) {
    return fromEsp(eraseAllItems(h, key));
}

}  // namespace

// =============================================================================
// Esp32NvsKeyValueStore
// =============================================================================

Esp32NvsKeyValueStore::Esp32NvsKeyValueStore(const char* nameSpace, size_t maxBlobBytes,
                                             const char* partition) noexcept
    : nameSpace_(nameSpace), partition_(partition), maxBlobBytes_(maxBlobBytes) {}

Esp32NvsKeyValueStore::~Esp32NvsKeyValueStore() {
    if (ready_) {
        nvs_close(handle_);
    }
}

StoreResult Esp32NvsKeyValueStore::init() noexcept {
    if (ready_) {
        return StoreResult::Ok;
    }
    if (!isValidKey(nameSpace_)) {
        return StoreResult::InvalidKey;
    }
    if (maxBlobBytes_ == 0 || !validPartition(partition_)) {
        return StoreResult::InvalidArgument;
    }
    const StoreResult r = openNamespace(partition_, nameSpace_, handle_);
    ready_ = r == StoreResult::Ok;
    return r;
}

StoreResult Esp32NvsKeyValueStore::usable(const char* key) const noexcept {
    if (!ready_) {
        return StoreResult::Unavailable;
    }
    return isValidKey(key) ? StoreResult::Ok : StoreResult::InvalidKey;
}

StoreResult Esp32NvsKeyValueStore::getI32(const char* key, int32_t& out) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    int32_t v = 0;
    const esp_err_t err = nvs_get_i32(handle_, key, &v);
    if (err == ESP_OK) {
        out = v;
    }
    return fromEsp(err);
}

StoreResult Esp32NvsKeyValueStore::setI32(const char* key, int32_t value) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    esp_err_t err = clearOtherTypes(handle_, key, ItemKind::I32);
    if (err == ESP_OK) {
        err = nvs_set_i32(handle_, key, value);
    }
    return fromEsp(err);
}

StoreResult Esp32NvsKeyValueStore::getU32(const char* key, uint32_t& out) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    uint32_t v = 0;
    const esp_err_t err = nvs_get_u32(handle_, key, &v);
    if (err == ESP_OK) {
        out = v;
    }
    return fromEsp(err);
}

StoreResult Esp32NvsKeyValueStore::setU32(const char* key, uint32_t value) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    esp_err_t err = clearOtherTypes(handle_, key, ItemKind::U32);
    if (err == ESP_OK) {
        err = nvs_set_u32(handle_, key, value);
    }
    return fromEsp(err);
}

StoreResult Esp32NvsKeyValueStore::getBlob(const char* key, uint8_t* out, size_t capacity,
                                           size_t& stored) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (out == nullptr && capacity > 0) {
        return StoreResult::InvalidArgument;
    }
    return readBlob(handle_, key, out, capacity, stored);
}

StoreResult Esp32NvsKeyValueStore::setBlob(const char* key, const uint8_t* data,
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
    return writeBlob(handle_, key, data, size);
}

StoreResult Esp32NvsKeyValueStore::erase(const char* key) {
    const StoreResult r = usable(key);
    if (r != StoreResult::Ok) {
        return r;
    }
    return eraseKey(handle_, key);
}

StoreResult Esp32NvsKeyValueStore::commit() {
    if (!ready_) {
        return StoreResult::Unavailable;
    }
    return fromEsp(nvs_commit(handle_));
}

// =============================================================================
// Esp32NvsSlotStore
// =============================================================================

Esp32NvsSlotStore::Esp32NvsSlotStore(const char* nameSpace, const char* prefix, uint16_t slots,
                                     size_t maxRecordBytes, const char* partition) noexcept
    : nameSpace_(nameSpace),
      prefix_(prefix),
      partition_(partition),
      maxRecordBytes_(maxRecordBytes),
      slots_(slots) {}

Esp32NvsSlotStore::~Esp32NvsSlotStore() {
    if (ready_) {
        nvs_close(handle_);
    }
}

StoreResult Esp32NvsSlotStore::init() noexcept {
    if (ready_) {
        return StoreResult::Ok;
    }
    if (!isValidKey(nameSpace_) || !isValidKey(prefix_)) {
        return StoreResult::InvalidKey;
    }
    if (slots_ == 0 || maxRecordBytes_ == 0 || !validPartition(partition_)) {
        return StoreResult::InvalidArgument;
    }
    // The longest key is the prefix plus the digits of the last slot.
    char digits[8];
    const int n = snprintf(digits, sizeof(digits), "%u", static_cast<unsigned>(slots_ - 1));
    if (n <= 0 || strlen(prefix_) + static_cast<size_t>(n) > kMaxKeyLength) {
        return StoreResult::InvalidKey;
    }
    const StoreResult r = openNamespace(partition_, nameSpace_, handle_);
    ready_ = r == StoreResult::Ok;
    return r;
}

StoreResult Esp32NvsSlotStore::usable(uint16_t slot, char* key) const noexcept {
    if (!ready_) {
        return StoreResult::Unavailable;
    }
    if (slot >= slots_) {
        return StoreResult::InvalidArgument;
    }
    // init() proved prefix + digits fits kMaxKeyLength.
    snprintf(key, kMaxKeyLength + 1, "%s%u", prefix_, static_cast<unsigned>(slot));
    return StoreResult::Ok;
}

StoreResult Esp32NvsSlotStore::query(uint16_t slot, size_t& stored) {
    char key[kMaxKeyLength + 1];
    const StoreResult r = usable(slot, key);
    if (r != StoreResult::Ok) {
        return r;
    }
    size_t len = 0;
    const esp_err_t err = nvs_get_blob(handle_, key, nullptr, &len);
    const StoreResult res = fromEsp(err);
    if (res == StoreResult::Ok) {
        stored = len;
    } else if (res == StoreResult::NotFound) {
        stored = 0;
    }
    return res;
}

StoreResult Esp32NvsSlotStore::read(uint16_t slot, uint8_t* out, size_t capacity,
                                    size_t& stored) {
    char key[kMaxKeyLength + 1];
    const StoreResult r = usable(slot, key);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (out == nullptr && capacity > 0) {
        return StoreResult::InvalidArgument;
    }
    const StoreResult res = readBlob(handle_, key, out, capacity, stored);
    if (res == StoreResult::NotFound) {
        stored = 0;
    }
    return res;
}

StoreResult Esp32NvsSlotStore::write(uint16_t slot, const uint8_t* data, size_t size) {
    char key[kMaxKeyLength + 1];
    const StoreResult r = usable(slot, key);
    if (r != StoreResult::Ok) {
        return r;
    }
    if (data == nullptr && size > 0) {
        return StoreResult::InvalidArgument;
    }
    if (size > maxRecordBytes_) {
        return StoreResult::TooLarge;
    }
    return writeBlob(handle_, key, data, size);
}

StoreResult Esp32NvsSlotStore::erase(uint16_t slot) {
    char key[kMaxKeyLength + 1];
    const StoreResult r = usable(slot, key);
    if (r != StoreResult::Ok) {
        return r;
    }
    return eraseKey(handle_, key);
}

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE && defined(ESP32)
