/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(ESP32)

#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <atomic>
#include <cstdint>

namespace pixelroot32::networking {

/**
 * @class ESP32EspNowTransport
 * @brief ESP-NOW link backend for ESP32 targets.
 *
 * Brings up a station interface without a router, initializes ESP-NOW on one
 * fixed channel and maps the raw ESP-NOW error codes onto @ref NetResult.
 * Received frames and send results are staged by the two ESP-NOW callbacks into
 * fixed-capacity rings, so the Wi-Fi task never blocks the main loop and the
 * main loop never waits on the Wi-Fi task.
 *
 * Single owner: ESP-NOW callbacks are process-global C function pointers, so
 * only one instance can own the radio at a time. A second instance receives
 * @ref NetResult::Busy from init() until the owner shuts down. All
 * methods must be called from the main loop; the callbacks touch nothing but
 * their ring and a drop counter.
 *
 * The rings are single-producer/single-consumer: the Wi-Fi task is the only
 * producer and the main loop is the only consumer. A full ring drops the newest
 * entry and counts it, matching the session's own backpressure policy, because
 * a late frame is worth less than a frame the peer is still waiting on.
 *
 * v1 has no encryption: every peer is registered unencrypted and
 * @ref NetPeerConfig::encrypted is ignored, so traffic is only as private as
 * the radio's coverage. Add an LMK path before trusting this on a shared band.
 */
class ESP32EspNowTransport final : public NetTransport {
public:
    /** @brief Received frames staged by the callback before new ones are dropped. */
    static constexpr uint8_t kRxRingCapacity = 4;

    /** @brief Send results staged by the callback before new ones are dropped. */
    static constexpr uint8_t kTxResultRingCapacity = 4;

    /** @brief Creates an idle transport that owns no radio resources yet. */
    ESP32EspNowTransport() = default;

    // The radio is a process-global resource reached through static callbacks;
    // a copy would leave two objects claiming callbacks that only one owns.
    ESP32EspNowTransport(const ESP32EspNowTransport&) = delete;
    ESP32EspNowTransport& operator=(const ESP32EspNowTransport&) = delete;

    /** @brief Takes the link down when this instance still owns it. */
    ~ESP32EspNowTransport() override;

    /**
     * @brief Claims the radio, brings Wi-Fi up and starts ESP-NOW.
     * @param config Channel, node label and Wi-Fi ownership.
     * @return Ok when the link is usable; InvalidArgument for a channel outside
     *         1..13; Busy when another instance owns the radio; TransportError
     *         when a Wi-Fi, ESP-NOW or MAC step fails.
     *
     * Idempotent for the same instance: the radio stays up and only a changed
     * channel is applied. A failed call unwinds everything it started, so a
     * retry begins from a clean state. When `manageWifi` is set the transport
     * brings the station up without joining an access point, because an
     * association would move the channel and drop the link.
     */
    NetResult init(const NetTransportConfig& config) override;

    /**
     * @brief Unregisters the callbacks, deinitializes ESP-NOW and clears rings.
     *
     * Safe twice, safe after a failed init(), and safe on an instance that
     * never owned the radio. Wi-Fi is only turned off when this transport
     * brought it up (`manageWifi`).
     */
    void shutdown() override;

    /**
     * @brief Registers a peer address with ESP-NOW.
     * @param peer Address and label of the peer; the label is not sent here.
     * @return Ok when the peer is registered or already existed,
     *         NotInitialized when the link is down,
     *         PeerTableFull when the ESP-NOW peer table is exhausted,
     *         InvalidArgument for an unusable address, TransportError for any
     *         other failure.
     *
     * The peer follows the interface channel (`channel = 0`), so a later
     * @ref setChannel moves it without a re-registration. `encrypted` is
     * ignored in v1 and the peer is always registered unencrypted.
     */
    NetResult addPeer(const NetPeerConfig& peer) override;

    /**
     * @brief Removes a peer address from ESP-NOW.
     * @param address Peer MAC to forget.
     * @return Ok when the address is no longer registered, including when it
     *         never was; NotInitialized when the link is down.
     */
    NetResult removePeer(const NetAddress& address) override;

    /**
     * @brief Hands one complete frame to ESP-NOW.
     * @param to Destination address or the broadcast address.
     * @param frame Frame bytes, header included; may be null only when
     *        `length` is 0.
     * @param length Frame length in bytes, at most kNetTransportFrameSize.
     * @return Ok when the radio accepted the frame, NotInitialized,
     *         InvalidArgument for a null frame with bytes, PayloadTooLarge for
     *         a frame above the transport ceiling, Busy when the radio is out
     *         of buffers, or PeerNotFound when `to` was not registered.
     *
     * Never blocks: the delivery outcome of an accepted frame arrives through
     * @ref pollTxResult from the send callback.
     */
    NetResult send(const NetAddress& to, const uint8_t* frame,
                   uint16_t length) override;

    /**
     * @brief Pops the oldest staged frame.
     * @param out Receives the source address, length and frame bytes.
     * @return true when a frame was copied, false when the ring is empty.
     */
    bool pollRx(NetRxInfo& out) override;

    /**
     * @brief Pops the oldest staged send result.
     * @param out Receives the destination and whether ESP-NOW delivered it.
     * @return true when a result was copied, false when the ring is empty.
     */
    bool pollTxResult(NetTxResult& out) override;

    /**
     * @brief Returns the station MAC read during init().
     * @return Local MAC, or an all-zero address before a successful init().
     */
    NetAddress localAddress() const override;

    /**
     * @brief Changes the Wi-Fi channel every peer follows.
     * @param channel Channel number, 1..13.
     * @return Ok on success, InvalidArgument outside 1..13, NotInitialized
     *         before init(), TransportError when the radio rejects the change.
     *
     * The previous channel is kept when the change is rejected, so the value
     * reported by a later query still describes the live link.
     */
    NetResult setChannel(uint8_t channel) override;

    /**
     * @brief Reports whether this instance owns a usable link.
     * @return true between a successful init() and shutdown().
     */
    bool isReady() const override;

private:
    /** @brief Lowest usable 2.4 GHz channel. */
    static constexpr uint8_t kMinChannel = 1;

    /** @brief Highest usable 2.4 GHz channel. */
    static constexpr uint8_t kMaxChannel = 13;

    /**
     * @brief Releases the link and everything init() started.
     *
     * Callbacks are unregistered before the rings are cleared so the Wi-Fi task
     * cannot stage into memory that is being reset.
     */
    void release() noexcept;

    /**
     * @brief Resets both rings to empty.
     *
     * Only valid while no callback is registered: it moves the consumer index
     * as well as the producer index.
     */
    void clearRings() noexcept;

    /**
     * @brief Copies one received frame into the RX ring.
     * @param mac Source MAC reported by ESP-NOW.
     * @param data Frame bytes.
     * @param length Valid bytes; anything outside 1..kNetTransportFrameSize is
     *        dropped whole rather than truncated.
     *
     * Runs in the Wi-Fi task. Only touches the ring and the drop counter; it
     * never logs, allocates or reaches session state.
     */
    void stageReceived(const uint8_t* mac, const uint8_t* data,
                       int length) noexcept;

    /**
     * @brief Stages one send result into the TX result ring.
     * @param mac Destination MAC reported by ESP-NOW.
     * @param success Whether ESP-NOW delivered the frame.
     *
     * Runs in the Wi-Fi task under the same restrictions as
     * @ref stageReceived.
     */
    void stageSendResult(const uint8_t* mac, bool success) noexcept;

    /** @brief The one instance that owns the process-global callbacks. */
    static ESP32EspNowTransport* instance_;

    NetTransportConfig config{};         ///< Values from the last init().
    NetAddress localMac{};               ///< Station MAC read during init().
    uint8_t channel = kMinChannel;       ///< Channel the link is live on.
    bool ready = false;                  ///< True between init() and shutdown().
    bool linkUp = false;                 ///< True while our esp_now_init() is live.
    bool ownsWifi = false;               ///< True when we brought Wi-Fi up ourselves.
    NetRxInfo rxRing[kRxRingCapacity]{}; ///< Frames staged by the Wi-Fi task.
    std::atomic<uint16_t> rxHead{0};     ///< Producer index; Wi-Fi task only.
    std::atomic<uint16_t> rxTail{0};     ///< Consumer index; main loop only.
    NetTxResult txRing[kTxResultRingCapacity]{};  ///< Results staged by the Wi-Fi task.
    std::atomic<uint16_t> txHead{0};     ///< Producer index; Wi-Fi task only.
    std::atomic<uint16_t> txTail{0};     ///< Consumer index; main loop only.
    std::atomic<uint32_t> rxDropped{0};  ///< Frames the RX ring could not hold.
    std::atomic<uint32_t> txDropped{0};  ///< Results the TX ring could not hold.
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING && defined(ESP32)
