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
 * @class ISlotStore
 * @brief Storage model for a fixed number of numbered records (save slots).
 *
 * Rules shared by every adapter:
 * - Slots are numbered 0..slotCount()-1; any other slot returns
 *   InvalidArgument before the medium is touched, and no output is written.
 * - An empty slot reads as NotFound with `stored = 0`.
 * - Reads never write past @p capacity (see read()).
 * - write() is replace-or-keep: after a failed or interrupted write, read()
 *   returns either the previous record or the new one, never a mix. Each
 *   adapter documents how it guarantees this.
 * - Records are opaque bytes; RecordCodec validates them above this layer.
 * - Calls come from one task (the game loop); adapters are not thread-safe.
 */
class ISlotStore {
public:
    virtual ~ISlotStore() = default;

    /** @brief Number of slots; valid slot indices are 0..slotCount()-1. */
    virtual uint16_t slotCount() const = 0;

    /** @brief Largest record write() accepts, in bytes. */
    virtual size_t maxRecordSize() const = 0;

    /**
     * @brief Reports the size of the record in a slot.
     * @param slot Slot index.
     * @param stored Set to the record size on Ok, to 0 on NotFound; untouched
     *        on every other result.
     * @return Ok; NotFound for an empty slot; InvalidArgument for an
     *         out-of-range slot; IoError or Unavailable from the medium.
     */
    virtual StoreResult query(uint16_t slot, size_t& stored) = 0;

    /**
     * @brief Reads a record into a caller-owned buffer.
     *
     * Never writes past @p capacity, nor past the record size inside it.
     * `capacity == 0` is the size query: it returns TooLarge with @p stored
     * set when the record is non-empty, and only then may @p out be nullptr.
     *
     * @param slot Slot index.
     * @param out Destination; nullptr only when @p capacity is 0.
     * @param capacity Size of @p out in bytes.
     * @param stored Set to the record size on Ok and TooLarge, to 0 on
     *        NotFound; untouched on every other result.
     * @return Ok; NotFound for an empty slot; TooLarge when the record exceeds
     *         @p capacity (nothing is written to @p out); InvalidArgument for
     *         an out-of-range slot or a null @p out with a non-zero
     *         @p capacity; IoError or Unavailable from the medium.
     */
    virtual StoreResult read(uint16_t slot, uint8_t* out, size_t capacity,
                             size_t& stored) = 0;

    /**
     * @brief Replaces the record in a slot (replace-or-keep).
     * @param slot Slot index.
     * @param data Record bytes; nullptr only when @p size is 0.
     * @param size Record size; 0 stores an empty record (the slot is then
     *        occupied, not empty).
     * @return Ok; InvalidArgument for an out-of-range slot or a null @p data
     *         with a non-zero @p size; TooLarge when @p size exceeds
     *         maxRecordSize(); NoSpace when the medium is full; IoError or
     *         Unavailable from the medium. On failure the previous record is
     *         kept.
     */
    virtual StoreResult write(uint16_t slot, const uint8_t* data, size_t size) = 0;

    /**
     * @brief Empties a slot.
     * @return Ok; NotFound when the slot is already empty; InvalidArgument for
     *         an out-of-range slot; IoError or Unavailable from the medium.
     */
    virtual StoreResult erase(uint16_t slot) = 0;
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
