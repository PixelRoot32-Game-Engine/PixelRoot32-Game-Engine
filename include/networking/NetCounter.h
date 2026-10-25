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
 * Duplicate and ordering contract: the counter never rejects an operation for
 * arriving late or twice. NetSession already suppresses duplicates with its
 * per-peer sequence window, and addition is commutative, so a second guard here
 * would only add state. An event fed to @ref onEvent twice is therefore applied
 * twice, by design; the session is the single owner of effectively-once
 * delivery.
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
 * Slice boundary: snapshot export/import, the resync state and the Busy answer
 * while resyncing belong to the follow-up slice. The per-slot @c bootId and
 * @c cursor fields are already reserved here so that slice does not have to
 * change the slot layout or the wire format.
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
     * not reset: it stays monotonic for the lifetime of the object.
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
     * @return NetResult::NotInitialized before @ref init, Ok when every send
     *         was accepted or when no peer is online, or the first non-Ok
     *         result reported by @ref NetSession::send.
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
     */
    NetResult add(int16_t delta);

    /**
     * @brief Applies one session event, if it carries a counter operation.
     * @param event Event just popped from the session queue.
     *
     * Everything that is not a well-formed counter Message is ignored without
     * touching state: other event types, a payload length other than
     * @ref kCounterOpPayloadSize, a foreign tag, and an operation whose node id
     * equals the local one (a node id collision would otherwise count the
     * local delta twice). A node id that is not in the table yet allocates a
     * free slot and learns that peer's boot id; when every slot is taken the
     * event is dropped, keeping the existing slots intact.
     */
    void onEvent(const NetEvent& event);

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
     * `bootId` and `cursor` are not read by this slice. They are stored now so
     * the snapshot/resync slice can install them without re-laying out the
     * table or the wire protocol.
     */
    struct Slot {
        int32_t value = 0;   ///< Sum of the deltas applied from this node.
        uint32_t cursor = 0; ///< Reserved: last applied sequence (resync slice).
        uint16_t bootId = 0; ///< Node boot generation, learned from peerInfo.
        uint8_t nodeId = 0;  ///< Application label that owns this slot.
        bool used = false;   ///< false when the slot was never allocated.
    };

    /** @brief Index of the slot that represents the local node. */
    static constexpr uint8_t kLocalSlotIndex = 0;

    // Slot 0 must exist for the local accumulator, and peerInfo() indexes the
    // session table with the same configured capacity.
    static_assert(kMaxSlots >= 1,
                  "NetMaxPeers must reserve at least the local counter slot");

    void clearSlots() noexcept;
    int8_t findSlot(uint8_t nodeId) const noexcept;
    int8_t allocateSlot(uint8_t nodeId, uint8_t peerId) noexcept;
    static bool applyDelta(Slot& slot, int32_t delta) noexcept;

    NetSession* session = nullptr;  ///< Borrowed session; null before init.
    Slot slots[kMaxSlots]{};        ///< Fixed node accumulators.
    uint32_t revisionValue = 0;     ///< Applied-operation counter.
};

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
