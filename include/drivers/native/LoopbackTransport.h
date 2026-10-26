/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace pixelroot32::networking {

/**
 * @struct LoopbackFaults
 * @brief Injectable link faults for deterministic native testing.
 *
 * Percentages are applied per destination in a fixed order: loss, then
 * duplication, then reordering. All time comes from the owning bus, never from
 * the system clock, so a test script is reproducible on any machine.
 */
struct LoopbackFaults {
    uint8_t lossPercent = 0;       ///< 0..100 chance a frame is dropped.
    uint8_t duplicatePercent = 0;  ///< 0..100 chance a frame is enqueued twice.
    uint8_t reorderPercent = 0;    ///< 0..100 chance a frame jumps the previous one.
    uint16_t delayMs = 0;          ///< Delivery delay added to the bus clock.
    bool sendBusy = false;         ///< Makes every send return NetResult::Busy.
    uint32_t seed = 0x12345678u;   ///< Seed of the xorshift32 decision generator.
};

class LoopbackTransport;

/**
 * @class LoopbackBus
 * @brief In-process bus shared by a set of loopback transports.
 *
 * The bus is the simulation's clock and its fault injector. It owns a small
 * fixed list of attached transports, the current `nowMs` value and one
 * xorshift32 generator, so a script replayed on a fresh bus produces the same
 * decisions byte for byte. It allocates nothing and has no static state: every
 * object on the bus is owned by the caller.
 *
 * The bus must outlive every transport attached to it.
 */
class LoopbackBus {
public:
    /** @brief Maximum number of transports that can share one bus. */
    static constexpr uint8_t kMaxTransports = 8;

    LoopbackBus() = default;
    LoopbackBus(const LoopbackBus&) = delete;
    LoopbackBus& operator=(const LoopbackBus&) = delete;

    /**
     * @brief Advances the bus clock.
     * @param nowMs New monotonic time in milliseconds.
     *
     * Delivery never advances time on its own: tests move the clock explicitly
     * so delay and timeout behaviour is exact.
     */
    void setNowMs(uint32_t nowMs) noexcept;

    /**
     * @brief Returns the current bus clock.
     * @return Time in milliseconds as last set by setNowMs().
     */
    uint32_t nowMs() const noexcept;

    /**
     * @brief Installs a new fault profile and reseeds the generator.
     * @param faults Fault profile to apply.
     *
     * Reseeding from `faults.seed` makes a fault profile a self-contained
     * scenario: installing it twice restarts the same deterministic sequence.
     */
    void setFaults(const LoopbackFaults& faults) noexcept;

    /**
     * @brief Returns the active fault profile.
     * @return Const reference to the profile installed by setFaults().
     */
    const LoopbackFaults& faults() const noexcept;

    /**
     * @brief Detaches every transport and clears their queues.
     *
     * Detached transports report `isReady() == false` until init() is called
     * again. The bus clock is left untouched, because it is caller-owned time,
     * and the decision generator is reseeded from the current fault seed.
     */
    void reset() noexcept;

private:
    friend class LoopbackTransport;

    bool attach(LoopbackTransport& transport) noexcept;
    void detach(LoopbackTransport& transport) noexcept;
    LoopbackTransport* findByAddress(const NetAddress& address) noexcept;
    uint32_t nextRandom() noexcept;
    void reseed() noexcept;

    LoopbackTransport* transports[kMaxTransports]{};
    uint8_t transportCount = 0;
    uint32_t currentNowMs = 0;
    LoopbackFaults faultState{};
    uint32_t rngState = 0x12345678u;
};

/**
 * @class LoopbackTransport
 * @brief Native in-process transport with deterministic fault injection.
 *
 * Frames are copied directly between attached transports on the same bus and
 * channel; there is no socket, thread or ESP-NOW dependency. Peer
 * registration is a documented no-op because an address is resolved against
 * the attached transports, which also means `addPeer` and `removePeer` always
 * return NetResult::Ok.
 *
 * All time comes from the owning bus (@ref LoopbackBus::nowMs), so delay and
 * fairness behaviour is fully controlled by the test. Received frames and send
 * results live in fixed arrays; no heap allocation happens after construction.
 */
class LoopbackTransport final : public NetTransport {
public:
    /** @brief Number of received frames buffered before send() reports Busy. */
    static constexpr uint8_t kRxQueueCapacity = 8;

    /** @brief Number of send results buffered before send() reports Busy. */
    static constexpr uint8_t kTxResultCapacity = 4;

    /**
     * @brief Creates a transport bound to a bus and a local address.
     * @param bus Bus that carries this transport's frames; must outlive it.
     * @param localAddress Address this transport reports as its own.
     */
    LoopbackTransport(LoopbackBus& bus, const NetAddress& localAddress);

    // A transport owns a live bus attachment and queues; copying one would
    // duplicate neither safely, so both operations are forbidden.
    LoopbackTransport(const LoopbackTransport&) = delete;
    LoopbackTransport& operator=(const LoopbackTransport&) = delete;

    /** @brief Detaches from the bus if it is still attached. */
    ~LoopbackTransport() override;

    /**
     * @brief Attaches to the bus, stores the configuration and marks ready.
     * @param config Channel, node label and Wi-Fi ownership.
     * @return Ok, InvalidArgument for a channel outside 1..13, or Busy when
     *         the bus already holds @ref LoopbackBus::kMaxTransports.
     *
     * Idempotent: re-initializing an attached transport only updates the
     * stored configuration and channel.
     */
    NetResult init(const NetTransportConfig& config) override;

    /** @brief Detaches, clears both queues and marks the transport not ready. */
    void shutdown() override;

    /**
     * @brief Accepts a peer registration without storing it.
     * @param peer Ignored: the loopback resolves addresses directly.
     * @return Always NetResult::Ok.
     */
    NetResult addPeer(const NetPeerConfig& peer) override;

    /**
     * @brief Accepts a peer removal without storing it.
     * @param address Ignored: the loopback resolves addresses directly.
     * @return Always NetResult::Ok.
     */
    NetResult removePeer(const NetAddress& address) override;

    /**
     * @brief Copies a complete frame to the destination's receive queue.
     * @param to Destination address or broadcast address.
     * @param frame Complete frame bytes; may be null only when length is 0.
     * @param length Frame length, at most kNetTransportFrameSize.
     * @return Ok, NotInitialized, InvalidArgument, PayloadTooLarge,
     *         PeerNotFound or Busy.
     *
     * Broadcast reaches every other attached transport on the same channel.
     * A unicast send to an attached transport on another channel is accepted
     * but reported as an unsuccessful result and never delivered. One result
     * is queued per relevant destination; a dropout produces one unsuccessful
     * result and duplication still produces exactly one.
     */
    NetResult send(const NetAddress& to, const uint8_t* frame,
                   uint16_t length) override;

    /**
     * @brief Pops the oldest due frame.
     * @param out Receives source address, length and frame bytes.
     * @return true when a frame was popped; false when none is due.
     *
     * Entries are scanned in insertion order, so among frames already due the
     * oldest one wins. A frame becomes due when `bus.nowMs()` reaches its
     * queued deadline; comparisons tolerate 32-bit wrap.
     */
    bool pollRx(NetRxInfo& out) override;

    /**
     * @brief Pops the oldest send result.
     * @param out Receives the destination and delivery outcome.
     * @return true when a result was popped, false when the queue is empty.
     */
    bool pollTxResult(NetTxResult& out) override;

    /**
     * @brief Returns the address passed to the constructor.
     * @return Local address; never changes after construction.
     */
    NetAddress localAddress() const override;

    /**
     * @brief Sets the channel used for delivery matching.
     * @param channel Channel number, 1..13.
     * @return Ok on success, InvalidArgument outside 1..13 (the previous
     *         channel is kept on rejection).
     */
    NetResult setChannel(uint8_t channel) override;

    /**
     * @brief Reports whether the transport is initialized.
     * @return true between a successful init() and shutdown()/bus reset().
     */
    bool isReady() const override;

private:
    friend class LoopbackBus;

    struct RxEntry {
        NetRxInfo info{};
        uint32_t dueMs = 0;
    };

    void clearQueues() noexcept;
    void resetFromBus() noexcept;
    void pushRx(const NetAddress& from, const uint8_t* frame, uint16_t length,
                uint32_t dueMs) noexcept;
    bool rollPercent(uint8_t percent) noexcept;

    LoopbackBus& bus;
    NetAddress address;
    NetTransportConfig config;
    uint8_t channel = 1;
    bool ready = false;
    bool attached = false;
    RxEntry rxQueue[kRxQueueCapacity]{};
    uint8_t rxCount = 0;
    NetTxResult txQueue[kTxResultCapacity]{};
    uint8_t txCount = 0;
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)
