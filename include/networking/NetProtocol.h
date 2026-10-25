/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include "networking/NetTypes.h"

#include <cstdint>

namespace pixelroot32::networking {

/**
 * @brief Frame magic tag: the ASCII pair 'P' (0x50) then 'R' (0x52).
 *
 * The magic is a two-byte tag in reading order, not a numeric field, so it is
 * the one header field that is not written little-endian: byte 0 is 0x50 and
 * byte 1 is 0x52. Reading those bytes back as a big-endian value yields this
 * constant, which is what a peer compares against.
 */
inline constexpr uint16_t kNetMagic = 0x5052;

/** @brief Size in bytes of the wire header (magic through payloadLength). */
inline constexpr uint16_t kNetHeaderSize = platforms::config::NetHeaderSize;

/** @brief Largest frame accepted by the ESP-NOW link layer. */
inline constexpr uint16_t kNetMaxFrameSize = 250;

/** @brief Hard application ceiling once the header is subtracted. */
inline constexpr uint16_t kNetMaxAppPayload = kNetMaxFrameSize - kNetHeaderSize;

/** @brief Flag bit 0: the sender expects an ACK and retries. */
inline constexpr uint8_t kNetFlagReliable = 0x01;

// The wire layout is field-by-field, so a header of any other size would put
// every offset in this file and in NetProtocol.cpp out of sync.
static_assert(kNetHeaderSize == 16,
              "The networking wire header must be exactly 16 bytes.");

/**
 * @enum NetPacketType
 * @brief Discriminator carried in the header.
 *
 * Values are part of the wire format: never renumber a shipped value.
 */
enum class NetPacketType : uint8_t {
    Data = 1,      ///< Application payload.
    Hello = 2,     ///< Discovery beacon carrying node identity and uptime.
    Bye = 3,       ///< Shutdown notice.
    Ack = 4,       ///< ACKs a reliable sequence; payload is the 32-bit sequence.
    Snapshot = 5   ///< Versioned sync-layer state transfer.
};

/**
 * @struct NetPacketHeader
 * @brief Decoded, host-order view of the 16-byte wire header.
 *
 * This struct is never copied to or from the wire as a whole: padding and
 * host endianness are handled by explicit field-by-field encode/decode, so
 * the on-wire bytes are identical on ESP32 (little-endian RISC-V) and native.
 */
struct NetPacketHeader {
    uint8_t version;        ///< Wire protocol version, must equal the build's.
    NetPacketType type;     ///< Packet discriminator.
    uint8_t flags;          ///< Bit 0 reliable; every other bit reserved (must be zero).
    uint8_t sourceNodeId;   ///< Sender's application label.
    uint16_t sourceBootId;  ///< Sender's boot generation.
    uint32_t sequence;      ///< Sender's monotonic sequence.
    uint16_t ackSequence;   ///< Reserved (0 in v1); ACKs carry the 32-bit sequence in their 4-byte payload.
    uint16_t payloadLength; ///< Declared payload byte count.
};

/**
 * @enum NetParseResult
 * @brief Outcome of decoding a frame.
 *
 * The order of checks is fixed: TooShort before BadMagic, magic before
 * version, version before type, type before flags, flags before the payload
 * ceiling, and the declared length last. Callers therefore see exactly one
 * reason per malformed frame.
 */
enum class NetParseResult : uint8_t {
    Ok = 0,             ///< Frame is well formed.
    InvalidArgument,    ///< Null frame, null output header, or null payload sink.
    TooShort,           ///< Fewer than kNetHeaderSize bytes.
    BadMagic,           ///< Bytes 0..1 are not the 'PR' tag.
    BadVersion,         ///< Version does not match the build.
    BadType,            ///< Type is not one of the five known packet types.
    BadFlags,           ///< A reserved flag bit is set.
    PayloadTooLarge,    ///< Declared payload exceeds the configured ceiling.
    LengthMismatch      ///< frameLength != kNetHeaderSize + payloadLength.
};

/**
 * @brief Checks whether a type byte names a known packet type.
 * @param type Candidate type.
 * @return true for Data, Hello, Bye, Ack or Snapshot; false for anything else.
 */
bool isValidPacketType(NetPacketType type) noexcept;

/**
 * @brief Decodes and validates a complete frame header.
 *
 * `frameLength` must describe the whole frame, not just the header: the
 * decoder verifies the declared payload length against it, because a header
 * that disagrees with its frame would make the payload pointer meaningless.
 *
 * @param frame Frame bytes, at least kNetHeaderSize long.
 * @param frameLength Total frame length in bytes, payload included.
 * @param out Receives the header only when the result is Ok.
 * @return Ok, or the first validation failure (see NetParseResult).
 *
 * Zero-copy: reads the input, never writes to it, never allocates.
 */
NetParseResult decodeHeader(const uint8_t* frame, uint16_t frameLength,
                            NetPacketHeader& out);

/**
 * @brief Decodes a frame and points at its payload in place.
 *
 * Same validation as decodeHeader. On success `*payload` is either
 * `frame + kNetHeaderSize` (when the header declares a payload) or nullptr;
 * on failure it is set to nullptr, so a rejected frame can never hand out a
 * stale pointer.
 *
 * @param frame Frame bytes.
 * @param frameLength Total frame length in bytes, payload included.
 * @param out Receives the decoded header only when the result is Ok.
 * @param payload Receives a pointer into `frame`; must not be null.
 * @return Ok, or the first validation failure (see NetParseResult).
 *
 * Zero-copy: the payload is never copied and the input is never modified.
 */
NetParseResult decodeFrame(const uint8_t* frame, uint16_t frameLength,
                           NetPacketHeader& out, const uint8_t** payload);

/**
 * @brief Encodes a header into the caller's buffer.
 *
 * Rejects every header that decodeHeader would reject, so a build can never
 * emit a version, type, flag or length its own parser refuses.
 *
 * @param header Host-order header to encode.
 * @param out Destination buffer, at least kNetHeaderSize bytes.
 * @param capacity Writable bytes available in `out`.
 * @return kNetHeaderSize on success, 0 on invalid header or short buffer.
 */
uint16_t encodeHeader(const NetPacketHeader& header, uint8_t* out,
                      uint16_t capacity);

/**
 * @brief Encodes a header plus payload into the caller's buffer.
 *
 * @param header Host-order header whose payloadLength describes `payload`.
 * @param payload Payload bytes; may be null only when payloadLength is 0.
 * @param out Destination buffer, at least header size plus payload.
 * @param capacity Writable bytes available in `out`.
 * @return Total bytes written on success, 0 on invalid input or short buffer.
 *
 * Field-by-field encoding: no struct is memcpy'd, so padding cannot leak onto
 * the wire.
 */
uint16_t encodeFrame(const NetPacketHeader& header, const uint8_t* payload,
                     uint8_t* out, uint16_t capacity);

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
