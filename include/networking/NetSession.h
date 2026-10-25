/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace pixelroot32::networking {

/**
 * @struct NetSessionConfig
 * @brief Values the application supplies once, when the session starts.
 *
 * The defaults come from `platforms::config`, so an application that is happy
 * with the build-time limits only has to override the fields it cares about.
 * Two fields have sentinel semantics: `nodeId == 0` derives the label from the
 * local MAC, and `bootId == 0` asks the session to generate a per-boot value.
 */
struct NetSessionConfig {
    uint8_t channel = 1;  ///< Wi-Fi channel shared by every peer, 1..13.
    uint8_t nodeId = 0;   ///< Application label; 0 derives it from the MAC.
    uint16_t bootId = 0;  ///< Boot generation; 0 generates one per boot.
    bool discovery = true;  ///< false disables HELLO and requires seeded peers.
    uint16_t helloIntervalMs = platforms::config::NetHelloIntervalMs;  ///< HELLO period.
    uint16_t peerTimeoutMs = platforms::config::NetPeerTimeoutMs;      ///< Offline threshold.
    uint16_t ackTimeoutMs = platforms::config::NetAckTimeoutMs;        ///< Reserved for reliability.
    uint8_t maxRetries = platforms::config::NetMaxRetries;  ///< Reserved for reliability.
};

/**
 * @class NetSession
 * @brief Fixed-capacity networking session: discovery, peer table and lifecycle.
 *
 * The session turns the transport's raw frames into peer-table state and
 * application events. This slice owns discovery (HELLO), shutdown notification
 * (BYE), the peer table and the offline timeout; the data send path, ACK and
 * retry, duplicate suppression, backpressure and the sync counter are added by
 * later slices.
 *
 * Ownership contract: the session never owns, initializes or shuts down the
 * transport. @ref init requires a transport that is already ready, applies the
 * configured channel to it, and reports @ref NetResult::NotInitialized
 * otherwise. @ref shutdown sends a best-effort BYE and clears session state,
 * but deliberately leaves the transport running so the application stays in
 * control of the link lifetime.
 *
 * Budget contract: every table and queue is a fixed array sized at compile
 * time. No method allocates, blocks or uses exceptions; construction reserves
 * the whole footprint.
 *
 * Threading contract: every method must be called from the main loop. The
 * receive queue is deliberately not atomic, like GameplayEventBus, because the
 * transport publishes frames only through @ref NetTransport::pollRx.
 *
 * Time contract: the session has no clock of its own. All timers and
 * timestamps come from the `nowMs` argument of @ref update, so a native test
 * can drive a whole peer lifecycle deterministically.
 */
class NetSession {
public:
    /** @brief Number of fixed peer slots, including offline ones. */
    static constexpr uint8_t kMaxPeers = platforms::config::NetMaxPeers;

    /** @brief Number of pending application events buffered before dropping. */
    static constexpr uint16_t kRxQueueCapacity =
        platforms::config::NetRxQueueCapacity;

    /**
     * @brief Binds the session to a transport.
     * @param transportRef Transport the session reads and writes frames on.
     *
     * The reference must outlive the session. The transport is not touched
     * until @ref init.
     */
    explicit NetSession(NetTransport& transportRef);

    // The session holds a live transport reference and fixed state; copying or
    // moving one would leave two sessions mutating the same link.
    NetSession(const NetSession&) = delete;
    NetSession& operator=(const NetSession&) = delete;
    NetSession(NetSession&&) = delete;
    NetSession& operator=(NetSession&&) = delete;

    /**
     * @brief Starts the session on an already initialized transport.
     * @param newConfig Channel, identity and timer configuration.
     * @return Ok on success, NotInitialized when the transport is not ready,
     *         or the transport's rejection of the channel.
     *
     * Registers the broadcast address with the transport when discovery is on.
     * Peers registered through @ref addPeer survive a re-init, because they are
     * configuration rather than live link state; pending events are cleared.
     */
    NetResult init(const NetSessionConfig& newConfig);

    /**
     * @brief Sends a best-effort BYE and clears every peer and event.
     *
     * Safe to call on a session that was never initialized, and safe twice.
     * The transport is left initialized: shutting the link down belongs to the
     * application that brought it up.
     */
    void shutdown();

    /**
     * @brief Advances discovery, drains the transport and expires silent peers.
     * @param nowMs Current time in milliseconds. May wrap; all comparisons are
     *        unsigned-difference comparisons.
     *
     * No-op before @ref init and after @ref shutdown. At most eight received
     * frames are drained per call, so a frame flood cannot monopolize the main
     * loop; the remainder stays queued in the transport.
     */
    void update(uint32_t nowMs);

    /**
     * @brief Adds a peer address without waiting for discovery.
     * @param peer Address and application label to register.
     * @return InvalidArgument for the zero or broadcast address, Ok when the
     *         address was stored or already present, PeerTableFull when every
     *         slot is taken.
     *
     * The slot is stored but marked offline, and no event is emitted: this
     * path pre-seeds a locked deployment or pairs with `discovery = false`.
     * The address is also registered with the transport.
     */
    NetResult addPeer(const NetPeerConfig& peer);

    /**
     * @brief Pops the oldest pending session event.
     * @param out Receives the event when one is queued.
     * @return true when an event was written, false when the queue is empty.
     *
     * FIFO. PeerJoined and PeerLeft carry the peer slot and its application
     * label; message events arrive with the later data path.
     */
    bool pollEvent(NetEvent& out);

    /**
     * @brief Copies one peer slot out of the table.
     * @param peerId Slot index, as reported by a PeerJoined or PeerLeft event.
     * @param out Receives the slot snapshot.
     * @return true when the slot exists and is in use; false for an
     *         out-of-range index or a never-used slot.
     */
    bool peerInfo(uint8_t peerId, NetPeerInfo& out) const;

    /**
     * @brief Counts the peers currently considered online.
     * @return Number of slots marked online, at most NetMaxPeers.
     */
    uint8_t peerCount() const;

    /**
     * @brief Returns the session's traffic counters.
     * @return Reference to the live counters; only ever increases.
     */
    const NetStats& stats() const;

    /**
     * @brief Returns the effective local application label.
     * @return The configured nodeId, or the value derived from the local MAC.
     */
    uint8_t localNodeId() const;

    /**
     * @brief Returns the effective boot generation.
     * @return The configured bootId, or the generated per-boot value.
     */
    uint16_t localBootId() const;

private:
    /**
     * @struct PeerSlot
     * @brief One fixed peer table row.
     *
     * A slot is never freed: a timeout only clears `online`, so a returning
     * peer keeps its index and its accumulated state.
     */
    struct PeerSlot {
        NetAddress address{};    ///< Identity: the peer's source MAC.
        uint32_t lastSeenMs = 0; ///< Session time of the last accepted frame.
        uint32_t uptimeMs = 0;   ///< Uptime last advertised by HELLO.
        uint16_t bootId = 0;     ///< Peer's boot generation from HELLO.
        uint8_t nodeId = 0;      ///< Application label from HELLO.
        bool used = false;       ///< false when the slot was never allocated.
        bool online = false;     ///< false after BYE or the peer timeout.
    };

    /** @brief Frames drained from the transport per update, bounding flood cost. */
    static constexpr uint8_t kMaxRxFramesPerUpdate = 8;

    void clearPeers() noexcept;
    void clearEvents() noexcept;
    void pushEvent(const NetEvent& event) noexcept;
    void handleFrame(const NetRxInfo& frame, uint32_t nowMs) noexcept;
    void handleHello(const NetAddress& from, uint8_t nodeId, uint16_t bootId,
                     uint32_t uptimeMs, uint32_t nowMs) noexcept;
    void handleBye(const NetAddress& from) noexcept;
    void expirePeers(uint32_t nowMs) noexcept;
    bool sendHello(uint32_t nowMs) noexcept;
    void sendBye() noexcept;
    int8_t findPeer(const NetAddress& address) const noexcept;
    int8_t allocatePeerSlot() noexcept;

    NetTransport& transport;        ///< Borrowed link; never owned.
    NetSessionConfig config{};      ///< Effective configuration from init.
    NetStats counters{};            ///< Monotonic counters, session lifetime.
    PeerSlot peers[kMaxPeers]{};    ///< Peer table, allocated by MAC.
    NetEvent events[kRxQueueCapacity]{};  ///< Pending application events.
    uint16_t eventHead = 0;         ///< Index of the oldest pending event.
    uint16_t eventTail = 0;         ///< Index where the next event is stored.
    uint16_t eventCount = 0;        ///< Number of pending events.
    uint16_t bootIdValue = 0;       ///< Effective boot generation.
    uint8_t nodeIdValue = 0;        ///< Effective application label.
    uint32_t nextHelloMs = 0;       ///< Session time of the next HELLO.
    bool helloPending = false;      ///< First update after init sends immediately.
    bool initialized = false;       ///< true between init() and shutdown().
    bool broadcastRegistered = false;  ///< Broadcast peer already given to the link.
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
