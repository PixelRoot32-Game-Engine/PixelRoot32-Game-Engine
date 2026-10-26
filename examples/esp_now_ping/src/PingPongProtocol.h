/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

/**
 * @file PingPongProtocol.h
 * @brief Pure, platform-independent ping/pong codec for the ESP-NOW ping demo.
 *
 * The header is the whole wire contract: no global state, no allocation, no
 * platform header. It includes only `<cstdint>`, so the example and the native
 * unit tests share it, and every function is `inline ... noexcept` so including
 * it in several translation units cannot produce duplicate symbols.
 *
 * Wire contract, exactly @ref kPingPongPayloadSize (7) bytes, little endian:
 *
 *   byte 0    tag: kPingTag (16) for a ping, kPongTag (17) for a pong
 *   byte 1-2  sequence, uint16 little endian
 *   byte 3-6  stampMs, uint32 little endian
 *
 * The tag registry is per game. `NetCounter` owns module tags 1, 2 and 3, so
 * this example starts at 16 and uses 16 and 17. A game with more message kinds
 * should keep its own enumerator of tags in one place and never reuse one.
 *
 * The stamp is an application timestamp carried verbatim; it is not an absolute
 * clock and both peers are not expected to share one. Compare two stamps from
 * the same sender with elapsedMs().
 */
namespace esp_now_ping {

/**
 * @brief Message tag of a ping, sent by the node that starts a measurement.
 */
inline constexpr uint8_t kPingTag = 16;

/**
 * @brief Message tag of a pong, the peer's reply to a ping.
 */
inline constexpr uint8_t kPongTag = 17;

/**
 * @brief Size of every ping and pong payload: tag + u16 sequence + u32 stamp.
 */
inline constexpr uint8_t kPingPongPayloadSize = 7;

/**
 * @brief Fields carried by both a ping and a pong.
 *
 * The same struct is used in both directions so a pong can echo the sequence
 * and stamp of the ping it answers.
 */
struct PingPongMessage {
    uint16_t sequence = 0;  ///< Application sequence number; echoed by a pong.
    uint32_t stampMs = 0;   ///< Sender's timestamp in milliseconds; echoed.
};

namespace detail {

/**
 * @brief Shared encoder: writes one tag and the fixed little-endian record.
 *
 * The null and capacity checks run before the first write, so a rejected call
 * leaves @p out untouched.
 *
 * @param tag  Message tag to write in byte 0.
 * @param out  Destination buffer.
 * @param capacity Bytes available at @p out.
 * @param message Fields to write.
 * @return kPingPongPayloadSize on success, 0 when @p out is null or
 *         @p capacity is smaller than kPingPongPayloadSize.
 */
inline uint16_t encodeMessage(uint8_t tag, uint8_t* out, uint16_t capacity,
                              const PingPongMessage& message) noexcept {
    if (out == nullptr || capacity < kPingPongPayloadSize) {
        return 0;
    }
    out[0] = tag;
    out[1] = static_cast<uint8_t>(message.sequence & 0xFFu);
    out[2] = static_cast<uint8_t>((message.sequence >> 8) & 0xFFu);
    out[3] = static_cast<uint8_t>(message.stampMs & 0xFFu);
    out[4] = static_cast<uint8_t>((message.stampMs >> 8) & 0xFFu);
    out[5] = static_cast<uint8_t>((message.stampMs >> 16) & 0xFFu);
    out[6] = static_cast<uint8_t>((message.stampMs >> 24) & 0xFFu);
    return kPingPongPayloadSize;
}

/**
 * @brief Shared decoder: validates the tag and length, then reads the record.
 *
 * The exact-length check runs before the first field read, so a short or long
 * payload is never read as a record. On failure @p out is untouched.
 *
 * @param expectedTag Tag byte required in @p data[0].
 * @param data  Received payload.
 * @param length Bytes received; must equal kPingPongPayloadSize.
 * @param out   Destination for the decoded fields.
 * @return true when the payload is a valid record of @p expectedTag.
 */
inline bool decodeMessage(uint8_t expectedTag, const uint8_t* data,
                          uint16_t length, PingPongMessage& out) noexcept {
    if (data == nullptr || length != kPingPongPayloadSize ||
        data[0] != expectedTag) {
        return false;
    }
    PingPongMessage message;
    message.sequence = static_cast<uint16_t>(
        static_cast<uint16_t>(data[1]) |
        (static_cast<uint16_t>(data[2]) << 8));
    message.stampMs = static_cast<uint32_t>(data[3]) |
                      (static_cast<uint32_t>(data[4]) << 8) |
                      (static_cast<uint32_t>(data[5]) << 16) |
                      (static_cast<uint32_t>(data[6]) << 24);
    out = message;
    return true;
}

}  // namespace detail

/**
 * @brief Encode a ping into @p out.
 *
 * @param out      Destination buffer of at least @p capacity bytes.
 * @param capacity Bytes available at @p out.
 * @param message  Sequence and stamp to send.
 * @return kPingPongPayloadSize (7) on success, 0 when @p out is null or
 *         @p capacity is smaller than 7.
 */
inline uint16_t encodePing(uint8_t* out, uint16_t capacity,
                           const PingPongMessage& message) noexcept {
    return detail::encodeMessage(kPingTag, out, capacity, message);
}

/**
 * @brief Encode a pong into @p out.
 *
 * @param out      Destination buffer of at least @p capacity bytes.
 * @param capacity Bytes available at @p out.
 * @param message  Sequence and stamp echoed from the ping.
 * @return kPingPongPayloadSize (7) on success, 0 when @p out is null or
 *         @p capacity is smaller than 7.
 */
inline uint16_t encodePong(uint8_t* out, uint16_t capacity,
                           const PingPongMessage& message) noexcept {
    return detail::encodeMessage(kPongTag, out, capacity, message);
}

/**
 * @brief Decode a ping payload.
 *
 * Rejects a null pointer, a length other than exactly 7, and a tag other than
 * kPingTag. A rejected payload never reads past its 7 declared bytes and never
 * modifies @p out.
 *
 * @param data   Received payload.
 * @param length Bytes received; must equal kPingPongPayloadSize.
 * @param out    Destination for the decoded fields.
 * @return true when the payload is a valid ping.
 */
inline bool decodePing(const uint8_t* data, uint16_t length,
                       PingPongMessage& out) noexcept {
    return detail::decodeMessage(kPingTag, data, length, out);
}

/**
 * @brief Decode a pong payload.
 *
 * Rejects a null pointer, a length other than exactly 7, and a tag other than
 * kPongTag. A rejected payload never reads past its 7 declared bytes and never
 * modifies @p out.
 *
 * @param data   Received payload.
 * @param length Bytes received; must equal kPingPongPayloadSize.
 * @param out    Destination for the decoded fields.
 * @return true when the payload is a valid pong.
 */
inline bool decodePong(const uint8_t* data, uint16_t length,
                       PingPongMessage& out) noexcept {
    return detail::decodeMessage(kPongTag, data, length, out);
}

/**
 * @brief Application round-trip interval between two millisecond stamps.
 *
 * This is an interval, not an absolute timestamp: it is defined only for two
 * stamps from the same 32-bit millisecond clock, with @p nowMs at or after
 * @p thenMs in the unsigned sense. Unsigned subtraction makes the result
 * correct across the 32-bit wrap, so
 * elapsedMs(5, 0xFFFFFFF0u) is 21, not a huge number.
 *
 * @param nowMs  Later stamp (for example `millis()` at pong receipt).
 * @param thenMs Earlier stamp (the stamp carried by the ping).
 * @return Elapsed milliseconds, modulo 2^32.
 */
inline uint32_t elapsedMs(uint32_t nowMs, uint32_t thenMs) noexcept {
    return nowMs - thenMs;
}

}  // namespace esp_now_ping
