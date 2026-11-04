/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_PERSISTENCE

#include "persistence/StoreTypes.h"

#include <cstddef>
#include <cstdint>

namespace pixelroot32::persistence {

/**
 * @class IKeyValueStore
 * @brief Storage model for small named values: typed integers and blobs.
 *
 * Rules shared by every adapter:
 * - Keys follow isValidKey(); any other key returns InvalidKey before the
 *   medium is touched, and no output is written.
 * - A key holds one value. A set of any type replaces whatever the key held
 *   (last write wins), so a typed get on a key that now holds another type
 *   returns NotFound.
 * - A missing key returns NotFound and leaves every output untouched.
 * - Blob reads never write past @p capacity (see getBlob()).
 * - Writes are explicit: nothing is written in the background. A set or
 *   erase may stay volatile until commit(); see commit().
 * - Calls come from one task (the game loop); adapters are not thread-safe.
 */
class IKeyValueStore {
public:
    virtual ~IKeyValueStore() = default;

    /**
     * @brief Reads a signed 32-bit value.
     * @param key Key to read.
     * @param out Receives the value on Ok; untouched otherwise.
     * @return Ok; NotFound when the key is missing or holds another type;
     *         InvalidKey; IoError or Unavailable from the medium.
     */
    virtual StoreResult getI32(const char* key, int32_t& out) = 0;

    /**
     * @brief Stores a signed 32-bit value, replacing any value of any type.
     * @return Ok; InvalidKey; NoSpace when the key is new and the store is
     *         full; IoError or Unavailable from the medium. On failure the
     *         previous value is kept.
     */
    virtual StoreResult setI32(const char* key, int32_t value) = 0;

    /**
     * @brief Reads an unsigned 32-bit value.
     * @param key Key to read.
     * @param out Receives the value on Ok; untouched otherwise.
     * @return Ok; NotFound when the key is missing or holds another type;
     *         InvalidKey; IoError or Unavailable from the medium.
     */
    virtual StoreResult getU32(const char* key, uint32_t& out) = 0;

    /**
     * @brief Stores an unsigned 32-bit value, replacing any value of any type.
     * @return As setI32().
     */
    virtual StoreResult setU32(const char* key, uint32_t value) = 0;

    /**
     * @brief Reads a blob into a caller-owned buffer.
     *
     * Never writes past @p capacity, nor past the stored size inside it.
     * `capacity == 0` is the size query: it returns TooLarge with @p stored
     * set when the blob is non-empty (Ok with 0 when it is empty), and only
     * then may @p out be nullptr.
     *
     * @param key Key to read.
     * @param out Destination; nullptr only when @p capacity is 0.
     * @param capacity Size of @p out in bytes.
     * @param stored Set to the stored blob size on Ok and TooLarge; untouched
     *        on every other result.
     * @return Ok; TooLarge when the blob exceeds @p capacity (nothing is
     *         written to @p out); NotFound when the key is missing or holds a
     *         typed integer; InvalidKey; InvalidArgument for a null @p out with
     *         a non-zero @p capacity; IoError or Unavailable from the medium.
     */
    virtual StoreResult getBlob(const char* key, uint8_t* out, size_t capacity,
                                size_t& stored) = 0;

    /**
     * @brief Stores a blob, replacing any value of any type.
     * @param key Key to write.
     * @param data Bytes to store; nullptr only when @p size is 0.
     * @param size Blob size; 0 stores an empty blob.
     * @return Ok; InvalidKey; InvalidArgument for a null @p data with a
     *         non-zero @p size; TooLarge when @p size exceeds the store's
     *         per-value maximum; NoSpace when the store (or medium) is full;
     *         IoError or Unavailable from the medium. On failure the previous
     *         value is kept.
     */
    virtual StoreResult setBlob(const char* key, const uint8_t* data, size_t size) = 0;

    /**
     * @brief Removes a key and its value, whatever its type.
     * @return Ok; NotFound when the key is missing; InvalidKey; IoError or
     *         Unavailable from the medium.
     */
    virtual StoreResult erase(const char* key) = 0;

    /**
     * @brief Makes every earlier set and erase durable.
     *
     * The durability point, and the game's decision: call it at a safe
     * moment. On the ESP32 a flash write stalls flash-resident code on both
     * cores. Adapters without a write-back stage return Ok.
     *
     * @return Ok; IoError or Unavailable from the medium.
     */
    virtual StoreResult commit() = 0;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
