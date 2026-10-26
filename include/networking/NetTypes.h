/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include <cstdint>

namespace pixelroot32::networking {

/** @brief Length in bytes of a MAC address, the peer identity on ESP-NOW. */
inline constexpr uint8_t kNetAddressSize = 6;

/**
 * @struct NetAddress
 * @brief Fixed-size peer address, trivially copyable and free of constructors.
 *
 * A 6-byte MAC with no methods that own resources: the transport hands it
 * around by value and the peer table stores it inline, so the module never
 * needs a heap allocation to identify a peer.
 */
struct NetAddress {
    uint8_t bytes[kNetAddressSize]{};  ///< Raw MAC bytes, in transmission order.

    /**
     * @brief Compares two addresses byte by byte.
     * @param other Address to compare against.
     * @return true when every byte matches.
     */
    bool operator==(const NetAddress& other) const noexcept {
        for (uint8_t i = 0; i < kNetAddressSize; ++i) {
            if (bytes[i] != other.bytes[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Compares two addresses byte by byte.
     * @param other Address to compare against.
     * @return true when at least one byte differs.
     */
    bool operator!=(const NetAddress& other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Reports whether every byte is 0xFF.
     * @return true for the ESP-NOW broadcast address.
     */
    bool isBroadcast() const noexcept {
        for (uint8_t i = 0; i < kNetAddressSize; ++i) {
            if (bytes[i] != 0xFFu) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Reports whether every byte is 0x00.
     * @return true for an unset address, which is never a valid peer.
     */
    bool isZero() const noexcept {
        for (uint8_t i = 0; i < kNetAddressSize; ++i) {
            if (bytes[i] != 0x00u) {
                return false;
            }
        }
        return true;
    }
};

/**
 * @enum NetResult
 * @brief Every fallible networking entry point returns one of these.
 *
 * A dedicated enum (instead of a bool plus out-parameter) keeps transport,
 * session and application failures distinguishable without exceptions, which
 * this engine builds out.
 */
enum class NetResult : uint8_t {
    Ok = 0,
    NotInitialized,
    InvalidArgument,
    PayloadTooLarge,
    PeerNotFound,
    PeerTableFull,
    NoMemory,
    Busy,
    TransportError,
    Unsupported
};

/**
 * @enum NetDelivery
 * @brief Delivery contract requested for one outgoing message.
 */
enum class NetDelivery : uint8_t {
    Unreliable = 0,  ///< Fire and forget, no ACK, no retry.
    Reliable = 1     ///< ACKed and retried until acknowledged or given up.
};

/**
 * @enum NetEventType
 * @brief Kind of event drained from the session's receive queue.
 */
enum class NetEventType : uint8_t {
    Message = 0,   ///< Application payload from a peer.
    PeerJoined = 1,  ///< A peer became reachable.
    PeerLeft = 2,    ///< A peer timed out.
    SendFailed = 3   ///< A reliable message exhausted its retries.
};

/**
 * @struct NetEvent
 * @brief One fixed-capacity event queued for the application.
 *
 * The payload buffer is inline at the configured ceiling
 * (`platforms::config::NetMaxPayload`), so delivering an event never
 * allocates. Field order is declaration order for alignment and is pinned by
 * the session's memory budget; do not reorder casually.
 */
struct NetEvent {
    NetEventType type = NetEventType::Message;  ///< Event kind.
    uint8_t peerId = 0;                         ///< Sender / affected peer slot.
    uint8_t nodeId = 0;                         ///< Application label of that peer.
    uint32_t sequence = 0;                      ///< Wire sequence that produced the event.
    uint16_t length = 0;                        ///< Valid bytes in @ref payload.
    uint8_t payload[platforms::config::NetMaxPayload]{};  ///< Inline application bytes.
};

/**
 * @struct NetPeerInfo
 * @brief Snapshot of one peer table slot, copied out on query.
 */
struct NetPeerInfo {
    NetAddress address{};   ///< Peer MAC, the identity used by the transport.
    uint8_t nodeId = 0;     ///< Application label last advertised by the peer.
    uint16_t bootId = 0;    ///< Boot generation, used to reset dedupe state.
    uint16_t rttMs = 0;     ///< Last measured round-trip time.
    uint32_t lastSeenMs = 0;  ///< Timestamp of the last accepted packet.
    uint32_t uptimeMs = 0;    ///< Peer uptime last advertised by HELLO.
    bool online = false;      ///< false once the peer timeout elapsed.
};

/**
 * @struct NetStats
 * @brief Monotonic traffic counters owned by the session.
 *
 * Counters only ever increase for the lifetime of the session; consumers
 * derive rates by differencing two snapshots.
 */
struct NetStats {
    uint32_t txMessages = 0;    ///< Frames handed to the transport.
    uint32_t rxMessages = 0;    ///< Frames accepted from the transport.
    uint32_t txRetries = 0;     ///< Reliable retransmissions.
    uint32_t txFailed = 0;      ///< Reliable messages that gave up.
    uint32_t rxDropped = 0;     ///< Frames dropped before delivery (bad frame, full queue).
    uint32_t rxDuplicates = 0;  ///< Frames rejected by the dedupe window.
    uint32_t txBusy = 0;        ///< Sends deferred because the transport was busy.
    uint16_t lastRttMs = 0;     ///< Most recent round-trip time.
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
