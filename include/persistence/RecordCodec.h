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
 * @brief Computes a CRC-32/IEEE (the zlib/PNG CRC) over a byte range.
 *
 * Reflected polynomial 0xEDB88320 with init and final xor 0xFFFFFFFF; the
 * check value for "123456789" is 0xCBF43926. The result chains: passing the
 * CRC of a first range as @p crc continues the stream, so
 * `crc32(b, nb, crc32(a, na))` equals the CRC of `a` followed by `b`.
 * A 16-entry nibble table keeps the flash cost to 64 bytes.
 *
 * @param data Bytes to hash; may be nullptr when @p size is 0.
 * @param size Number of bytes.
 * @param crc CRC of the preceding bytes, or 0 to start a new stream.
 * @return CRC of the whole stream so far.
 */
uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc = 0);

/**
 * @struct RecordInfo
 * @brief Header fields reported by RecordCodec::decode().
 */
struct RecordInfo {
    uint16_t schemaVersion;  ///< Game-owned schema version, reported as stored.
    size_t payloadSize;      ///< Payload length in bytes, excluding the header.
};

/**
 * @class RecordCodec
 * @brief Wraps a game payload in a self-checking envelope.
 *
 * Adapters move opaque bytes; this codec is the one place a record is
 * validated. The 16-byte header is written byte by byte, little-endian:
 *
 * | Bytes | Field |
 * |-------|-------|
 * | 0-3   | Magic, ASCII `P` `R` `S` `T` |
 * | 4     | Envelope version (kEnvelopeVersion) |
 * | 5     | Reserved, 0 |
 * | 6-7   | Schema version (u16, game-owned) |
 * | 8-11  | Payload length (u32) |
 * | 12-15 | CRC-32 over bytes 0..11 followed by the payload |
 *
 * Covering the header in the CRC means a flipped schema version or length is
 * caught even though neither field is checked against a fixed value.
 */
class RecordCodec {
public:
    /** @brief Size in bytes of the envelope header. */
    static constexpr size_t kHeaderSize = 16;

    /** @brief Envelope layout version written and accepted by this codec. */
    static constexpr uint8_t kEnvelopeVersion = 1;

    /**
     * @brief Writes header and payload into a caller-owned buffer.
     * @param schema Game-owned schema version, stored as is.
     * @param payload Payload bytes; may be nullptr only when @p size is 0.
     * @param size Payload length; 0 is a valid record.
     * @param out Destination buffer.
     * @param capacity Size of @p out in bytes.
     * @param written Set to kHeaderSize + @p size on success, 0 on failure.
     * @return Ok; InvalidArgument for a null @p out, a null @p payload with a
     *         non-zero size, or a size beyond the u32 length field; TooLarge
     *         when @p capacity is below kHeaderSize + @p size. Nothing is
     *         written to @p out on failure.
     */
    static StoreResult encode(uint16_t schema, const uint8_t* payload, size_t size,
                              uint8_t* out, size_t capacity, size_t& written);

    /**
     * @brief Validates a record and locates its payload.
     * @param in Record bytes, header first.
     * @param size Exact record size in bytes.
     * @param payload Set to `in + kHeaderSize` on success.
     * @param info Set to the stored schema version and payload size on success.
     * @return Ok; InvalidArgument for a null @p in; Corrupt for a truncated
     *         record, wrong magic, unknown envelope version, non-zero reserved
     *         byte, a length field that disagrees with @p size, or a CRC
     *         mismatch. The schema version is reported, never judged.
     *         @p payload and @p info are untouched on failure.
     */
    static StoreResult decode(const uint8_t* in, size_t size, const uint8_t*& payload,
                              RecordInfo& info);
};

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_PERSISTENCE
