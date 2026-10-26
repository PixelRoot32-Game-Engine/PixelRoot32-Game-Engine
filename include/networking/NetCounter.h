/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include "networking/NetSession.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace pixelroot32::networking {

/**
 * @brief Tag byte that identifies one counter operation payload.
 *
 * Application messages share the Data path with every other payload, so the
 * first byte names the dialect. A peer built with a different tag treats the
 * frame as foreign instead of applying a delta it cannot interpret.
 */
inline constexpr uint8_t kCounterOpTag = 1;

/**
 * @brief Wire size of one counter operation: the tag plus a 32-bit delta.
 *
 * The public API takes an int16_t, but the wire field is 32 bits: it is
 * sign-extended on encode, which keeps room for a wider API later without a
 * protocol change and makes the decode independent of platform int widths.
 */
inline constexpr uint8_t kCounterOpPayloadSize = 5;

/**
 * @brief Tag byte that identifies one snapshot payload.
 *
 * A snapshot is a state transfer, not an operation: the receiver replaces its
 * whole slot table instead of adding a delta. The distinct tag keeps the two
 * dialects apart even though both travel as application Message payloads.
 */
inline constexpr uint8_t kCounterSnapshotTag = 2;

/**
 * @brief Tag byte of a bare snapshot request (no payload beyond the tag).
 *
 * A node that boots after its peers asks the established node for a snapshot.
 * The request covers the case where the survivor never observed a PeerLeft
 * (the reboot was faster than the peer timeout) and therefore never entered
 * its own join path: without it, the younger node would wait until the resync
 * timeout even though the survivor is reachable.
 */
inline constexpr uint8_t kCounterSnapshotRequestTag = 3;

/** @brief Wire size of a snapshot request. */
inline constexpr uint8_t kCounterSnapshotRequestSize = 1;

/**
 * @brief Uptime difference below which two nodes are considered booted together.
 *
 * A difference wider than this is real evidence of who has been running longer,
 * so that node owns the snapshot. Inside the window neither node is trusted to
 * be newer and the tie is broken by node id, which makes the choice
 * deterministic on both sides.
 */
inline constexpr uint16_t kCounterUptimeTieMs = 1000;

/**
 * @brief How long a resyncing node waits for a snapshot before giving up.
 *
 * Matches the peer timeout: a snapshot that has not arrived by then is treated
 * as lost, and the node resumes local operation with @ref NetCounter::isOutOfSync
 * raised so the game can surface the divergence.
 */
inline constexpr uint16_t kCounterResyncTimeoutMs = 3000;

/**
 * @brief Number of counter operations buffered while a snapshot is in flight.
 *
 * Operations that arrive during resync cannot be applied yet because the
 * snapshot that will reset the table has not landed. They are kept in arrival
 * order and replayed after the import; drop-newest, so a flood cannot evict an
 * operation that arrived before the snapshot.
 */
inline constexpr uint8_t kCounterResyncBufferCapacity = 4;

/**
 * @class NetCounter
 * @brief Convergent signed accumulator replicated as idempotent operations.
 *
 * Every node owns one slot and publishes its own deltas. The shared value is
 * the sum of every used slot, so the same set of applied operations produces
 * the same value on every peer regardless of arrival order. This is the v1
 * reference implementation of "deterministic state sync": convergence, not
 * lockstep.
 *
 * Wire layout of one operation (5 bytes, little endian):
 * @code
 *   byte 0      : kCounterOpTag
 *   bytes 1..4  : int32 delta, sign extended from the int16_t API value
 * @endcode
 *
 * Wire layout of one snapshot (2 + slotCount * 11 bytes, little endian):
 * @code
 *   byte 0      : kCounterSnapshotTag
 *   byte 1      : slotCount
 *   bytes 2..   : slotCount entries of 11 bytes each:
 *                 nodeId u8, bootId u16, value i32, cursor u32
 * @endcode
 * At the default limits that is 46 bytes, inside the 64-byte payload. The
 * compile-time assert below rejects a configuration whose table cannot be
 * exported into one payload.
 *
 * Duplicate and ordering contract: the counter never rejects an operation for
 * arriving late or twice. NetSession already suppresses duplicates with its
 * per-peer sequence window, and addition is commutative, so a second guard here
 * would only add state. An event fed to @ref onEvent twice is therefore applied
 * twice, by design; the session is the single owner of effectively-once
 * delivery. Cursors are only consulted when replaying operations that were
 * buffered during resync.
 *
 * Replication contract: @ref add sends one Reliable operation to every online
 * peer, so the link, not the caller, retries. The local slot is updated before
 * the send, because the game reads its own input in the same frame; a failed
 * send is reported but never rolled back.
 *
 * Budget contract: one fixed slot table sized at compile time and no heap, no
 * exceptions and no logging. Construction reserves the whole footprint.
 *
 * Ownership contract: the counter borrows a session that outlives it. It never
 * owns, initializes or shuts the session or the transport down, and it never
 * touches the transport directly.
 *
 * Threading contract: every method must be called from the main loop, next to
 * @ref NetSession::update. The counter holds no locks and no atomics.
 *
 * Slice boundary: this class owns operations, snapshot exchange and the short
 * resync window a node enters when it boots after its peers. A snapshot is an
 * application Message payload (tag @ref kCounterSnapshotTag) sent with
 * @ref NetSession::send under @ref NetDelivery::Reliable; the session still owns
 * retries, dedupe and ACKs.
 *
 * Resync contract: a node that joins late calls @ref update once per frame. On
 * @ref NetEventType::PeerJoined the better-established node (higher uptime, or
 * the lower node id inside @ref kCounterUptimeTieMs) sends a snapshot, and the
 * younger node enters resync. While resyncing @ref add answers
 * @ref NetResult::Busy and incoming operations are buffered instead of applied.
 * A valid snapshot replaces the table, raises @ref revision once, and the
 * buffer is replayed against the imported cursors. If no snapshot arrives
 * before @ref kCounterResyncTimeoutMs the node resumes with
 * @ref isOutOfSync raised; a press made during the window may have been dropped
 * and the game can use that flag to surface the divergence.
 */
class NetCounter {
public:
    /**
     * @brief Number of fixed slots, local node included.
     *
     * The local slot takes one entry, so at most `kMaxSlots - 1` remote node
     * ids can be tracked at the same time.
     */
    static constexpr uint8_t kMaxSlots = platforms::config::NetMaxPeers;

    NetCounter() = default;

    // One counter owns live slot state tied to a single session; a copy would
    // let two counters diverge while both claim the same node identity.
    NetCounter(const NetCounter&) = delete;
    NetCounter& operator=(const NetCounter&) = delete;

    /**
     * @brief Binds the counter to a session and allocates the local slot.
     * @param session Session this counter reads and sends through; it must
     *        outlive the counter.
     * @return Always NetResult::Ok.
     *
     * The session is assumed to be initialized: this class has no way to ask,
     * and it only needs the local identity. The local slot is rebuilt with
     * value 0 at the configured @ref NetSession::localNodeId and
     * @ref NetSession::localBootId. Every other slot is cleared, so re-init is
     * idempotent for the replicated state. The revision counter is deliberately
     * not reset: it stays monotonic for the lifetime of the object. Resync
     * state, the deferred snapshot send and the operation buffer are cleared,
     * because they describe a previous session's link.
     */
    NetResult init(NetSession& session);

    /**
     * @brief Clears every slot and detaches from the session.
     *
     * Safe on a counter that was never initialized, and safe twice. @ref add
     * answers NetResult::NotInitialized afterwards and @ref value reports 0;
     * the session itself is left untouched.
     */
    void shutdown();

    /**
     * @brief Applies one delta locally and replicates it to every online peer.
     * @param delta Signed change; 0 is a no-op.
     * @return NetResult::NotInitialized before @ref init, NetResult::Busy while
     *         @ref isResyncing is true, Ok when every send was accepted or when
     *         no peer is online, or the first non-Ok result reported by
     *         @ref NetSession::send.
     *
     * The delta is clamped into the int32 range rather than wrapped, so an
     * eventually saturated counter never flips sign. The local slot and the
     * revision advance only when the value actually changes: a delta that
     * cannot move a saturated slot is neither counted nor replicated. The
     * local slot is updated before any send: a failing link must not make the
     * local node disagree with itself. A non-Ok result describes the outgoing
     * operation only; it is not a rollback signal, and the operation may still
     * have reached some peers.
     *
     * Each operation is one Reliable message, so the session owns retries and
     * dedupe. A delta of 0 sends nothing and leaves the revision alone.
     *
     * A Busy result is not an error: the local table is about to be replaced by
     * a snapshot, so applying a delta now would be lost in the import. The game
     * may drop the input that produced the call, or queue it and try again after
     * @ref isResyncing clears.
     */
    NetResult add(int16_t delta);

    /**
     * @brief Applies one session event: a counter operation, a snapshot, or a
     *        peer lifecycle change.
     * @param event Event just popped from the session queue.
     *
     * A @ref NetEventType::PeerJoined event is consumed before the Message
     * filter, because a join is the only trigger that decides who exports the
     * slot table. The peer is looked up through @ref NetSession::peerInfo; an
     * event for an unknown slot is ignored. A new boot id for a tracked node
     * keeps its accumulated value but resets its sequence cursor, because the
     * new boot starts a fresh sequence space. The uptime rule then either sends
     * a snapshot (@ref kCounterUptimeTieMs of margin, lower node id on a tie) or
     * enters resync.
     *
     * A @ref NetEventType::PeerLeft event cancels a resync or a pending snapshot
     * aimed at that peer, so a flapping link does not wait out the full timeout.
     *
     * A Message carrying a well-formed counter operation (@ref kCounterOpTag,
     * @ref kCounterOpPayloadSize bytes) is applied and the sender slot's cursor
     * is advanced to the serial maximum. Older sequences are never rejected:
     * the session owns duplicate suppression and addition is commutative. A
     * node id that is not in the table yet allocates a free slot; when every
     * slot is taken the event is dropped, keeping the existing slots intact.
     *
     * While resyncing, operations are buffered in arrival order (drop-newest at
     * @ref kCounterResyncBufferCapacity) instead of applied. A snapshot with tag
     * @ref kCounterSnapshotTag is validated and, when well formed, replaces the
     * whole slot table, raises @ref revision once, ends resync and replays the
     * buffer against the imported cursors. A second snapshot while the first is
     * still in flight is ignored, as is any malformed or foreign payload.
     */
    void onEvent(const NetEvent& event);

    /**
     * @brief Advances time-dependent counter state.
     * @param nowMs Current time in milliseconds. May wrap; comparisons are
     *        unsigned or serial-number based.
     *
     * Stores the local uptime used by the join rule, retries one deferred
     * snapshot send, and expires an overdue resync. A full reliable queue makes
     * a snapshot send answer Busy; the peer is remembered and one retry is
     * attempted per call, so an update never spins and never waits.
     *
     * Must be called once per frame, before @ref NetSession::update drains the
     * queue, so a PeerJoined handler sees a current uptime.
     */
    void update(uint32_t nowMs);

    /**
     * @brief Reports whether the node is waiting for a snapshot.
     * @return true between entering resync and the import, timeout, or peer
     *         departure that ends it.
     */
    bool isResyncing() const;

    /**
     * @brief Reports whether a resync gave up without a snapshot.
     * @return true after a resync timeout, until the next resync attempt or
     *         @ref init.
     *
     * @ref add works normally while this flag is true, but the value may have
     * diverged from the peers and the game can choose to surface that.
     */
    bool isOutOfSync() const;

    /**
     * @brief Returns the converged value.
     * @return Sum of every used slot, clamped into the int32 range.
     *
     * The sum is accumulated in 64 bits so several saturated slots can never
     * overflow the arithmetic before the final clamp.
     */
    int32_t value() const;

    /**
     * @brief Returns how many real value changes were applied.
     * @return Monotonic count of local and remote changes.
     *
     * Counts changes, not operations: an operation that lands on an already
     * saturated slot does not bump it, and an ignored operation (zero delta,
     * malformed payload, full table) does not either. A game can use two
     * samples to know whether the shared value moved since the last frame.
     */
    uint32_t revision() const;

private:
    /**
     * @struct Slot
     * @brief One node's accumulated contribution to the shared value.
     *
     * `cursor` is the serial number of the last operation applied from this
     * node. It survives normal operation, is exported in a snapshot and reset
     * when the node advertises a new boot generation, because a reboot starts
     * a fresh sequence space. The local slot's cursor stays 0: it tracks
     * inbound operations, and a node never applies its own.
     */
    struct Slot {
        int32_t value = 0;   ///< Sum of the deltas applied from this node.
        uint32_t cursor = 0; ///< Last applied sequence from this node.
        uint16_t bootId = 0; ///< Node boot generation, learned from peerInfo.
        uint8_t nodeId = 0;  ///< Application label that owns this slot.
        bool used = false;   ///< false when the slot was never allocated.
    };

    /** @brief Index of the slot that represents the local node. */
    static constexpr uint8_t kLocalSlotIndex = 0;

    /** @brief Wire size of one snapshot slot entry, in bytes. */
    static constexpr uint8_t kSnapshotEntrySize = 11;

    // Slot 0 must exist for the local accumulator, and peerInfo() indexes the
    // session table with the same configured capacity.
    static_assert(kMaxSlots >= 1,
                  "NetMaxPeers must reserve at least the local counter slot");

    // A snapshot exports every used slot into one application payload. Checking
    // this at compile time keeps the encoder from silently truncating a table
    // that the configured payload cannot carry.
    static_assert(static_cast<uint32_t>(2) +
                          static_cast<uint32_t>(kMaxSlots) *
                              static_cast<uint32_t>(kSnapshotEntrySize) <=
                      static_cast<uint32_t>(platforms::config::NetMaxPayload),
                  "A counter snapshot must fit in one NetMaxPayload");

    void clearSlots() noexcept;
    int8_t findSlot(uint8_t nodeId) const noexcept;
    int8_t allocateSlot(uint8_t nodeId, uint8_t peerId) noexcept;
    static bool applyDelta(Slot& slot, int32_t delta) noexcept;
    static bool isNewerSequence(uint32_t candidate, uint32_t current) noexcept;

    void handlePeerJoined(const NetEvent& event) noexcept;
    void handlePeerLeft(const NetEvent& event) noexcept;
    void enterResync(uint8_t peerId) noexcept;
    void clearResyncBuffer() noexcept;
    uint16_t encodeSnapshot(uint8_t* out, uint16_t capacity) const noexcept;
    NetResult sendSnapshot(uint8_t peerId) noexcept;
    void requestSnapshotSend(uint8_t peerId) noexcept;
    void sendSnapshotRequest(uint8_t peerId) noexcept;
    bool importSnapshot(const NetEvent& event) noexcept;
    void replayBufferedOps() noexcept;
    void applyCounterOp(const NetEvent& event, bool checkCursor) noexcept;

    NetSession* session = nullptr;  ///< Borrowed session; null before init.
    Slot slots[kMaxSlots]{};        ///< Fixed node accumulators.
    uint32_t revisionValue = 0;     ///< Applied-operation counter.
    uint32_t lastNowMs = 0;         ///< Local uptime from the last update().
    uint32_t resyncDeadlineMs = 0;  ///< Local time at which resync expires.
    uint8_t resyncPeerId = 0;       ///< Peer slot whose snapshot is awaited.
    uint8_t pendingSnapshotPeerId = 0;  ///< Peer that gets the deferred send.
    uint8_t pendingSnapshotRequestPeerId = 0;  ///< Peer that gets a deferred request.
    uint8_t resyncBufferCount = 0;  ///< Occupied entries in resyncBuffer.
    bool resyncing = false;         ///< true while waiting for a snapshot.
    bool outOfSync = false;         ///< true after a resync timeout.
    bool snapshotSendPending = false;  ///< true when a Busy send must retry.
    bool snapshotRequestPending = false;  ///< true when a Busy request must retry.
    NetEvent resyncBuffer[kCounterResyncBufferCapacity]{};  ///< Deferred ops.
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
