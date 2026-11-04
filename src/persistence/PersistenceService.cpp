/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "persistence/PersistenceService.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

namespace pixelroot32::persistence {

namespace {

// Decodes a record read into scratch; outputs change only on success.
StoreResult decodeInto(const uint8_t* scratch, size_t stored, const uint8_t*& payload,
                       RecordInfo& info) {
    const uint8_t* decoded = nullptr;
    RecordInfo decodedInfo{};
    if (RecordCodec::decode(scratch, stored, decoded, decodedInfo) != StoreResult::Ok) {
        return StoreResult::Corrupt;
    }
    payload = decoded;
    info = decodedInfo;
    return StoreResult::Ok;
}

}  // namespace

StoreResult PersistenceService::saveRecord(uint16_t slot, uint16_t schema,
                                           const uint8_t* payload, size_t size,
                                           uint8_t* scratch, size_t scratchCapacity) {
    if (saves_ == nullptr) {
        return StoreResult::Unavailable;
    }
    if (scratch == nullptr) {
        return StoreResult::InvalidArgument;
    }
    size_t written = 0;
    const StoreResult r =
        RecordCodec::encode(schema, payload, size, scratch, scratchCapacity, written);
    if (r != StoreResult::Ok) {
        return r;
    }
    return saves_->write(slot, scratch, written);
}

StoreResult PersistenceService::loadRecord(uint16_t slot, uint8_t* scratch,
                                           size_t scratchCapacity, const uint8_t*& payload,
                                           RecordInfo& info) {
    if (saves_ == nullptr) {
        return StoreResult::Unavailable;
    }
    if (scratch == nullptr) {
        return StoreResult::InvalidArgument;
    }
    size_t stored = 0;
    const StoreResult r = saves_->read(slot, scratch, scratchCapacity, stored);
    if (r != StoreResult::Ok) {
        return r;
    }
    return decodeInto(scratch, stored, payload, info);
}

StoreResult PersistenceService::saveSettingRecord(const char* key, uint16_t schema,
                                                  const uint8_t* payload, size_t size,
                                                  uint8_t* scratch, size_t scratchCapacity) {
    if (settings_ == nullptr) {
        return StoreResult::Unavailable;
    }
    if (scratch == nullptr) {
        return StoreResult::InvalidArgument;
    }
    size_t written = 0;
    const StoreResult r =
        RecordCodec::encode(schema, payload, size, scratch, scratchCapacity, written);
    if (r != StoreResult::Ok) {
        return r;
    }
    return settings_->setBlob(key, scratch, written);
}

StoreResult PersistenceService::loadSettingRecord(const char* key, uint8_t* scratch,
                                                  size_t scratchCapacity, const uint8_t*& payload,
                                                  RecordInfo& info) {
    if (settings_ == nullptr) {
        return StoreResult::Unavailable;
    }
    if (scratch == nullptr) {
        return StoreResult::InvalidArgument;
    }
    size_t stored = 0;
    const StoreResult r = settings_->getBlob(key, scratch, scratchCapacity, stored);
    if (r != StoreResult::Ok) {
        return r;
    }
    return decodeInto(scratch, stored, payload, info);
}

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
