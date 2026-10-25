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
 * @brief Largest frame one transport call may move, header included.
 *
 * This is the complete protocol frame: the 16-byte wire header plus the
 * configured application payload (80 bytes at the default limits). The
 * transport layer never sees an application payload on its own, so the
 * session can hand frames through without re-framing them.
 */
inline constexpr uint16_t kNetTransportFrameSize =
    platforms::config::NetHeaderSize + platforms::config::NetMaxPayload;

// ESP-NOW moves at most 250 bytes in one frame. Failing the build here is
// cheaper than discovering at runtime that a configured frame cannot be sent.
static_assert(kNetTransportFrameSize <= 250,
              "kNetTransportFrameSize must fit one ESP-NOW frame (<= 250 bytes)");

/**
 * @struct NetTransportConfig
 * @brief Per-transport initialization values.
 *
 * Mirrors the session configuration rather than owning it: the transport is
 * told which channel to use and which label its node advertises, and it does
 * not interpret either beyond link setup.
 */
struct NetTransportConfig {
    uint8_t channel = 1;       ///< Wi-Fi channel, 1..13 on real hardware.
    uint8_t nodeId = 0;        ///< Application label; 0 means derive from the MAC.
    bool manageWifi = true;    ///< false leaves Wi-Fi ownership to the application.
};

/**
 * @struct NetPeerConfig
 * @brief One peer registration handed to the transport.
 */
struct NetPeerConfig {
    NetAddress address{};      ///< Peer MAC, the link-layer identity.
    uint8_t nodeId = 0;        ///< Application label advertised by that peer.
    bool encrypted = false;    ///< Reserved for v2; the loopback ignores it.
};

/**
 * @struct NetRxInfo
 * @brief One received frame together with its source address.
 *
 * `data` holds a complete frame (header plus payload), not an application
 * payload, and is sized to the maximum frame so receiving never allocates.
 */
struct NetRxInfo {
    NetAddress from{};                          ///< Sender's link-layer address.
    uint16_t length = 0;                        ///< Valid bytes in @ref data.
    uint8_t data[kNetTransportFrameSize]{};     ///< Complete frame bytes.
};

/**
 * @struct NetTxResult
 * @brief Outcome of one accepted send, reported asynchronously.
 *
 * One result is queued per delivered destination. `success` is false when the
 * link (or a fault injector) accepted the send but did not deliver it.
 */
struct NetTxResult {
    NetAddress to{};       ///< Destination that this result refers to.
    bool success = false;  ///< true when the frame was handed to the link.
};

/**
 * @class NetTransport
 * @brief Abstract frame-level transport used by the networking session.
 *
 * The transport moves complete frames between peers. It owns peer addresses,
 * the channel and the link state, and nothing else: it does not parse the
 * wire header, sequence messages, ACK, retry or touch game state.
 *
 * Threading contract: every method may be called only from the main loop. A
 * real link backend may stage received frames from its own callback, but it
 * must only publish them through @ref pollRx so the session drains them from
 * the main loop. @ref pollRx and @ref pollTxResult never block and never
 * allocate; they return false when nothing is pending.
 */
class NetTransport {
public:
    /** @brief Virtual destructor so a backend can release link resources. */
    virtual ~NetTransport() = default;

    /**
     * @brief Brings the link up and applies the transport configuration.
     * @param config Channel, node label and Wi-Fi ownership.
     * @return Ok on success, or a NetResult describing why the link is down.
     *
     * Must be idempotent: a second call with the same configuration leaves an
     * already initialized transport usable.
     */
    virtual NetResult init(const NetTransportConfig& config) = 0;

    /**
     * @brief Takes the link down and releases its resources.
     *
     * Must be safe to call on a transport that was never initialized.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Registers a peer address with the link layer.
     * @param peer Address and label of the peer.
     * @return Ok when the peer is known to the transport.
     */
    virtual NetResult addPeer(const NetPeerConfig& peer) = 0;

    /**
     * @brief Removes a previously registered peer.
     * @param address Peer MAC to forget.
     * @return Ok when the address is no longer registered.
     */
    virtual NetResult removePeer(const NetAddress& address) = 0;

    /**
     * @brief Sends one complete frame.
     * @param to Destination address, or the broadcast address.
     * @param frame Frame bytes, header included; may be null only when
     *        `length` is 0.
     * @param length Frame length in bytes, at most @ref kNetTransportFrameSize.
     * @return Ok when the link accepted the send (its result arrives through
     *         @ref pollTxResult), otherwise the rejection reason.
     *
     * Never blocks. A return other than Ok means nothing was queued.
     */
    virtual NetResult send(const NetAddress& to, const uint8_t* frame,
                           uint16_t length) = 0;

    /**
     * @brief Pops one received frame, if any.
     * @param out Receives the source address, length and frame bytes.
     * @return true when a frame was written to `out`; false when none is due.
     *
     * Non-blocking. The caller owns the copy for as long as it needs it.
     */
    virtual bool pollRx(NetRxInfo& out) = 0;

    /**
     * @brief Pops one send result, if any.
     * @param out Receives the destination and whether delivery succeeded.
     * @return true when a result was written to `out`; false when none is due.
     *
     * Non-blocking and strictly FIFO: results are reported in the order the
     * sends were accepted.
     */
    virtual bool pollTxResult(NetTxResult& out) = 0;

    /**
     * @brief Returns this transport's own link-layer address.
     * @return Local MAC used as the source of every outgoing frame.
     */
    virtual NetAddress localAddress() const = 0;

    /**
     * @brief Changes the Wi-Fi channel used by the link.
     * @param channel Channel number, 1..13.
     * @return Ok on success, InvalidArgument for a channel outside 1..13.
     *
     * Both peers must share a channel for traffic to be delivered.
     */
    virtual NetResult setChannel(uint8_t channel) = 0;

    /**
     * @brief Reports whether the link is initialized and usable.
     * @return true between a successful init() and shutdown().
     */
    virtual bool isReady() const = 0;
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
