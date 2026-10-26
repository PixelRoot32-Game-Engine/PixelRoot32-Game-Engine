/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "networking/NetCounter.h"

#if PIXELROOT32_ENABLE_NETWORKING

namespace pixelroot32::networking {

namespace {

/**
 * @brief Clamps a 64-bit accumulator into the signed 32-bit value domain.
 *
 * Saturating instead of wrapping keeps a counter that reaches an edge from
 * flipping sign, which would turn a large positive total into a large negative
 * one and hide the overflow from the game.
 */
int32_t saturateToI32(int64_t value) noexcept {
    if (value > static_cast<int64_t>(INT32_MAX)) {
        return INT32_MAX;
    }
    if (value < static_cast<int64_t>(INT32_MIN)) {
        return INT32_MIN;
    }
    return static_cast<int32_t>(value);
}

/**
 * @brief Writes one signed 32-bit value in little-endian wire order.
 * @param out Receives exactly four bytes.
 * @param value Value to encode.
 *
 * Shifting the unsigned representation avoids signed-shift behaviour while the
 * bytes on the wire stay two's complement, which every peer decodes the same
 * way.
 */
void writeI32LE(uint8_t* out, int32_t value) noexcept {
    const uint32_t raw = static_cast<uint32_t>(value);
    out[0] = static_cast<uint8_t>(raw & 0xFFu);
    out[1] = static_cast<uint8_t>((raw >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((raw >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((raw >> 24) & 0xFFu);
}

/**
 * @brief Reads one little-endian signed 32-bit value.
 * @param in Pointer to exactly four bytes.
 * @return The decoded value.
 */
int32_t readI32LE(const uint8_t* in) noexcept {
    const uint32_t raw = static_cast<uint32_t>(in[0]) |
                         (static_cast<uint32_t>(in[1]) << 8) |
                         (static_cast<uint32_t>(in[2]) << 16) |
                         (static_cast<uint32_t>(in[3]) << 24);
    return static_cast<int32_t>(raw);
}

/**
 * @brief Writes one unsigned 16-bit value in little-endian wire order.
 * @param out Receives exactly two bytes.
 * @param value Value to encode.
 */
void writeU16LE(uint8_t* out, uint16_t value) noexcept {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

/**
 * @brief Reads one little-endian unsigned 16-bit value.
 * @param in Pointer to exactly two bytes.
 * @return The decoded value.
 */
uint16_t readU16LE(const uint8_t* in) noexcept {
    return static_cast<uint16_t>(static_cast<uint16_t>(in[0]) |
                                 (static_cast<uint16_t>(in[1]) << 8));
}

/**
 * @brief Writes one unsigned 32-bit value in little-endian wire order.
 * @param out Receives exactly four bytes.
 * @param value Value to encode.
 */
void writeU32LE(uint8_t* out, uint32_t value) noexcept {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

/**
 * @brief Reads one little-endian unsigned 32-bit value.
 * @param in Pointer to exactly four bytes.
 * @return The decoded value.
 */
uint32_t readU32LE(const uint8_t* in) noexcept {
    return static_cast<uint32_t>(in[0]) |
           (static_cast<uint32_t>(in[1]) << 8) |
           (static_cast<uint32_t>(in[2]) << 16) |
           (static_cast<uint32_t>(in[3]) << 24);
}

}  // namespace

NetResult NetCounter::init(NetSession& sessionRef) {
    // The reference is stored as-is: the session owns its own initialization
    // and this class only needs the local identity plus the send path.
    session = &sessionRef;
    clearSlots();

    Slot& local = slots[kLocalSlotIndex];
    local.used = true;
    local.nodeId = sessionRef.localNodeId();
    local.bootId = sessionRef.localBootId();
    local.value = 0;
    local.cursor = 0;

    // revisionValue is intentionally kept: re-init rebuilds the replicated
    // state but the object's applied-operation count stays monotonic.
    lastNowMs = 0;
    resyncDeadlineMs = 0;
    resyncPeerId = 0;
    pendingSnapshotPeerId = 0;
    clearResyncBuffer();
    resyncing = false;
    outOfSync = false;
    snapshotSendPending = false;
    snapshotRequestPending = false;
    pendingSnapshotRequestPeerId = 0;

    return NetResult::Ok;
}

void NetCounter::shutdown() {
    clearSlots();
    clearResyncBuffer();
    session = nullptr;
    lastNowMs = 0;
    resyncDeadlineMs = 0;
    resyncPeerId = 0;
    pendingSnapshotPeerId = 0;
    resyncing = false;
    outOfSync = false;
    snapshotSendPending = false;
    snapshotRequestPending = false;
    pendingSnapshotRequestPeerId = 0;
}

NetResult NetCounter::add(int16_t delta) {
    if (session == nullptr) {
        return NetResult::NotInitialized;
    }

    // The table is about to be replaced by the snapshot. Applying now would be
    // discarded by the import, so the caller is told to retry (or drop the
    // press) instead of silently losing it.
    if (resyncing) {
        return NetResult::Busy;
    }

    // A zero delta is not an operation: replicating it would burn a reliable
    // sequence on the wire for a value that cannot change.
    if (delta == 0) {
        return NetResult::Ok;
    }

    // Local first: the input that produced the delta already happened, so the
    // game must see its own effect even if the link is down. There is no
    // rollback on a send failure for the same reason.
    if (!applyDelta(slots[kLocalSlotIndex], delta)) {
        // The local slot is saturated: replicating a delta that cannot move the
        // origin would let a peer move while this node stays pinned, so the
        // clamped shared value is left alone.
        return NetResult::Ok;
    }
    ++revisionValue;

    uint8_t payload[kCounterOpPayloadSize];
    payload[0] = kCounterOpTag;
    writeI32LE(&payload[1], static_cast<int32_t>(delta));

    const uint8_t localNodeId = slots[kLocalSlotIndex].nodeId;
    NetResult firstFailure = NetResult::Ok;

    // One operation per online peer: Reliable delivery already gives per-peer
    // retries and dedupe, so the counter has no sequence of its own to keep.
    for (uint8_t i = 0; i < kMaxSlots; ++i) {
        NetPeerInfo info{};
        if (!session->peerInfo(i, info)) {
            continue;
        }
        if (!info.online) {
            continue;
        }
        if (info.nodeId == localNodeId) {
            continue;
        }

        const NetResult result = session->send(
            i, NetDelivery::Reliable, payload, kCounterOpPayloadSize);
        if (result != NetResult::Ok && firstFailure == NetResult::Ok) {
            firstFailure = result;
        }
    }

    return firstFailure;
}

void NetCounter::onEvent(const NetEvent& event) {
    if (session == nullptr) {
        return;
    }

    // Lifecycle events are consumed before the Message filter: a join decides
    // who exports the table, and a leave aborts a wait that can no longer end.
    if (event.type == NetEventType::PeerJoined) {
        handlePeerJoined(event);
        return;
    }
    if (event.type == NetEventType::PeerLeft) {
        handlePeerLeft(event);
        return;
    }
    if (event.type != NetEventType::Message) {
        return;
    }

    if (resyncing) {
        // A snapshot is a state transfer: the first well-formed one wins and
        // replaces the whole table, so nothing may be applied before it lands.
        if (event.length >= 1 && event.payload[0] == kCounterSnapshotTag) {
            if (importSnapshot(event)) {
                resyncing = false;
                outOfSync = false;
                replayBufferedOps();
                clearResyncBuffer();
            }
            return;
        }

        // Operations are held in arrival order, not applied: the incoming
        // snapshot will reset the cursors they must be compared against.
        if (event.length == kCounterOpPayloadSize &&
            event.payload[0] == kCounterOpTag &&
            event.nodeId != slots[kLocalSlotIndex].nodeId) {
            if (resyncBufferCount < kCounterResyncBufferCapacity) {
                resyncBuffer[resyncBufferCount] = event;
                ++resyncBufferCount;
            }
            // Drop-newest: a full buffer keeps the operations that arrived
            // before the snapshot, which are the ones the snapshot may not
            // already include.
            return;
        }

        // Anything else while resyncing is foreign or malformed; ignore it and
        // keep waiting.
        return;
    }

    if (event.length == kCounterSnapshotRequestSize &&
        event.payload[0] == kCounterSnapshotRequestTag) {
        // A younger node that did not get a proactive snapshot asks for one.
        requestSnapshotSend(event.peerId);
        return;
    }

    applyCounterOp(event, false);
}

void NetCounter::applyCounterOp(const NetEvent& event, bool checkCursor) noexcept {
    if (event.length != kCounterOpPayloadSize) {
        return;
    }
    if (event.payload[0] != kCounterOpTag) {
        return;
    }
    if (event.nodeId == slots[kLocalSlotIndex].nodeId) {
        // A node id collision would apply our own operation a second time;
        // ignoring it keeps the local slot equal to what this node originated.
        return;
    }

    int8_t index = findSlot(event.nodeId);
    if (index < 0) {
        index = allocateSlot(event.nodeId, event.peerId);
        if (index < 0) {
            // The table is fixed, so a new node id is dropped instead of
            // evicting a slot that other operations already contributed to.
            return;
        }
    }

    Slot& slot = slots[static_cast<uint8_t>(index)];

    // Normal operation never rejects a late or duplicate sequence: Reliable
    // delivery is not ordered and the session already suppressed duplicates.
    // The cursor is a replay guard for buffered operations only, where the
    // snapshot may have moved the slot past an operation already applied.
    if (checkCursor && !isNewerSequence(event.sequence, slot.cursor)) {
        return;
    }

    if (applyDelta(slot, readI32LE(&event.payload[1]))) {
        ++revisionValue;
    }

    // Advance to the serial maximum: an old operation does not move the cursor
    // backwards, and a wrapped comparison keeps this correct across 2^32.
    if (isNewerSequence(event.sequence, slot.cursor)) {
        slot.cursor = event.sequence;
    }
}

void NetCounter::handlePeerJoined(const NetEvent& event) noexcept {
    NetPeerInfo info{};
    if (!session->peerInfo(event.peerId, info)) {
        // A queued event can outlive its slot; there is nothing to compare.
        return;
    }

    if (info.nodeId == slots[kLocalSlotIndex].nodeId) {
        // A duplicate application label is a collision the game must resolve;
        // treating the peer as a second copy of us would corrupt the local slot.
        return;
    }

    const int8_t existing = findSlot(info.nodeId);
    if (existing >= 0 && slots[existing].bootId != info.bootId) {
        // The accumulated value is history and survives the reboot; the cursor
        // belongs to the old boot's sequence space and must start over.
        slots[existing].cursor = 0;
        slots[existing].bootId = info.bootId;
    }

    // The node that has been running longer owns the authoritative table. A
    // difference inside the tie window is too small to trust, so node id makes
    // the choice identical on both sides.
    if (lastNowMs > info.uptimeMs + kCounterUptimeTieMs) {
        requestSnapshotSend(event.peerId);
    } else if (info.uptimeMs > lastNowMs + kCounterUptimeTieMs) {
        enterResync(event.peerId);
    } else if (slots[kLocalSlotIndex].nodeId < info.nodeId) {
        requestSnapshotSend(event.peerId);
    } else {
        enterResync(event.peerId);
    }
}

void NetCounter::handlePeerLeft(const NetEvent& event) noexcept {
    if (snapshotSendPending && pendingSnapshotPeerId == event.peerId) {
        snapshotSendPending = false;
    }
    if (snapshotRequestPending &&
        pendingSnapshotRequestPeerId == event.peerId) {
        snapshotRequestPending = false;
    }

    if (resyncing && resyncPeerId == event.peerId) {
        // The next join starts a fresh attempt, so a departure is not a
        // divergence: leave outOfSync alone instead of raising it.
        resyncing = false;
        outOfSync = false;
        clearResyncBuffer();
    }
}

void NetCounter::enterResync(uint8_t peerId) noexcept {
    if (resyncing) {
        // The first snapshot in flight wins; a second join must not restart the
        // deadline or discard a buffer that is already collecting operations.
        return;
    }
    resyncing = true;
    outOfSync = false;
    resyncPeerId = peerId;
    resyncDeadlineMs = lastNowMs + kCounterResyncTimeoutMs;
    clearResyncBuffer();
    // Ask explicitly: the survivor may still consider us online (a reboot
    // faster than its peer timeout) and therefore never run its own join path.
    sendSnapshotRequest(peerId);
}

void NetCounter::clearResyncBuffer() noexcept {
    // The copies hold no resources; only the count has to go. Clearing each
    // entry keeps a stale payload from being readable after the count resets.
    for (uint8_t i = 0; i < kCounterResyncBufferCapacity; ++i) {
        resyncBuffer[i] = NetEvent{};
    }
    resyncBufferCount = 0;
}

uint16_t NetCounter::encodeSnapshot(uint8_t* out,
                                    uint16_t capacity) const noexcept {
    if (out == nullptr || capacity < 2) {
        return 0;
    }

    out[0] = kCounterSnapshotTag;
    uint8_t count = 0;
    for (uint8_t i = 0; i < kMaxSlots; ++i) {
        if (!slots[i].used) {
            continue;
        }

        const uint16_t offset =
            static_cast<uint16_t>(2 + count * kSnapshotEntrySize);
        if (static_cast<uint32_t>(offset) + kSnapshotEntrySize > capacity) {
            break;
        }

        uint8_t* entry = &out[offset];
        entry[0] = slots[i].nodeId;
        writeU16LE(&entry[1], slots[i].bootId);
        writeI32LE(&entry[3], slots[i].value);
        writeU32LE(&entry[7], slots[i].cursor);
        ++count;
    }

    out[1] = count;
    return static_cast<uint16_t>(2 + count * kSnapshotEntrySize);
}

NetResult NetCounter::sendSnapshot(uint8_t peerId) noexcept {
    if (session == nullptr) {
        return NetResult::NotInitialized;
    }

    // One payload per send on the stack: the snapshot is bounded by the
    // configured payload size and nothing here allocates.
    uint8_t payload[platforms::config::NetMaxPayload]{};
    const uint16_t length = encodeSnapshot(payload, sizeof(payload));
    return session->send(peerId, NetDelivery::Reliable, payload, length);
}

void NetCounter::requestSnapshotSend(uint8_t peerId) noexcept {
    const NetResult result = sendSnapshot(peerId);
    if (result == NetResult::Busy) {
        // Accepted-but-deferred is retried from update(); a second Busy keeps
        // the first peer so neither request is dropped silently.
        if (!snapshotSendPending) {
            snapshotSendPending = true;
            pendingSnapshotPeerId = peerId;
        }
        return;
    }

    // Ok, PeerNotFound or any other terminal rejection: there is nothing to
    // retry, because the peer is gone or the frame cannot be sent at all.
    snapshotSendPending = false;
}

void NetCounter::sendSnapshotRequest(uint8_t peerId) noexcept {
    if (session == nullptr) {
        return;
    }

    const uint8_t payload[kCounterSnapshotRequestSize] = {
        kCounterSnapshotRequestTag};
    const NetResult result = session->send(
        peerId, NetDelivery::Reliable, payload, kCounterSnapshotRequestSize);
    if (result == NetResult::Busy) {
        // One deferred request is enough: a later update retries the same peer.
        if (!snapshotRequestPending) {
            snapshotRequestPending = true;
            pendingSnapshotRequestPeerId = peerId;
        }
        return;
    }

    snapshotRequestPending = false;
}

bool NetCounter::importSnapshot(const NetEvent& event) noexcept {
    if (event.length < 2 || event.payload[0] != kCounterSnapshotTag) {
        return false;
    }

    const uint8_t count = event.payload[1];
    if (count < 1 || count > kMaxSlots) {
        return false;
    }
    if (event.length != static_cast<uint16_t>(2 + count * kSnapshotEntrySize)) {
        return false;
    }

    // A snapshot replaces the table wholesale: the sender's view is the
    // authority, not a merge of two partially known histories.
    clearSlots();

    const uint8_t localNodeId = session->localNodeId();
    bool localInstalled = false;
    uint8_t nextRemote = 1;

    for (uint8_t i = 0; i < count; ++i) {
        const uint8_t* entry = &event.payload[2 + i * kSnapshotEntrySize];

        Slot slot;
        slot.used = true;
        slot.nodeId = entry[0];
        slot.bootId = readU16LE(&entry[1]);
        slot.value = readI32LE(&entry[3]);
        slot.cursor = readU32LE(&entry[7]);

        if (slot.nodeId == localNodeId) {
            // Keep our current identity even if the sender's copy is stale,
            // and keep the local cursor at zero: we never apply our own ops.
            slot.bootId = session->localBootId();
            slot.cursor = 0;
            slots[kLocalSlotIndex] = slot;
            localInstalled = true;
        } else if (nextRemote < kMaxSlots) {
            slots[nextRemote] = slot;
            ++nextRemote;
        }
        // A snapshot with more remote entries than free slots is malformed in
        // spirit; the extra entries are dropped, never overwriting the local
        // slot or an entry already installed.
    }

    if (!localInstalled) {
        // The sender may not know this node yet (a first boot). The historical
        // contribution is zero, and the slot exists so add() can use it.
        Slot& local = slots[kLocalSlotIndex];
        local = Slot{};
        local.used = true;
        local.nodeId = localNodeId;
        local.bootId = session->localBootId();
    }

    // One revision bump for the import as a whole, not one per slot: the game
    // sees a single state replacement.
    ++revisionValue;
    return true;
}

void NetCounter::replayBufferedOps() noexcept {
    for (uint8_t i = 0; i < resyncBufferCount; ++i) {
        // Replay is the only path with a cursor check: the snapshot may already
        // include an operation that was buffered before it arrived.
        applyCounterOp(resyncBuffer[i], true);
    }
}

void NetCounter::update(uint32_t nowMs) {
    lastNowMs = nowMs;

    if (snapshotSendPending) {
        // Exactly one attempt per call: a full reliable queue must not turn
        // update() into a spin loop.
        const NetResult result = sendSnapshot(pendingSnapshotPeerId);
        if (result != NetResult::Busy) {
            snapshotSendPending = false;
        }
    }

    if (snapshotRequestPending) {
        // Same one-attempt rule for a deferred snapshot request.
        const uint8_t payload[kCounterSnapshotRequestSize] = {
            kCounterSnapshotRequestTag};
        const NetResult result = session->send(
            pendingSnapshotRequestPeerId, NetDelivery::Reliable, payload,
            kCounterSnapshotRequestSize);
        if (result != NetResult::Busy) {
            snapshotRequestPending = false;
        }
    }

    if (resyncing &&
        static_cast<int32_t>(nowMs - resyncDeadlineMs) >= 0) {
        // Wrap-safe "now has reached the deadline". The value may be stale now,
        // so the flag tells the game to resurface whatever it displays.
        resyncing = false;
        outOfSync = true;
        clearResyncBuffer();
    }
}

bool NetCounter::isResyncing() const {
    return resyncing;
}

bool NetCounter::isOutOfSync() const {
    return outOfSync;
}

bool NetCounter::isNewerSequence(uint32_t candidate,
                                 uint32_t current) noexcept {
    // Serial-number arithmetic: a small positive forward distance means newer.
    // This is wrap-safe for the 32-bit sequence space.
    return static_cast<int32_t>(candidate - current) > 0;
}

int32_t NetCounter::value() const {
    int64_t sum = 0;
    for (uint8_t i = 0; i < kMaxSlots; ++i) {
        if (slots[i].used) {
            sum += static_cast<int64_t>(slots[i].value);
        }
    }
    return saturateToI32(sum);
}

uint32_t NetCounter::revision() const {
    return revisionValue;
}

void NetCounter::clearSlots() noexcept {
    for (uint8_t i = 0; i < kMaxSlots; ++i) {
        slots[i] = Slot{};
    }
}

int8_t NetCounter::findSlot(uint8_t nodeId) const noexcept {
    for (uint8_t i = 0; i < kMaxSlots; ++i) {
        if (slots[i].used && slots[i].nodeId == nodeId) {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

int8_t NetCounter::allocateSlot(uint8_t nodeId, uint8_t peerId) noexcept {
    for (uint8_t i = 0; i < kMaxSlots; ++i) {
        if (slots[i].used) {
            continue;
        }

        Slot& slot = slots[i];
        slot = Slot{};
        slot.used = true;
        slot.nodeId = nodeId;

        // peerInfo() validates the index, so an event from a peer slot that is
        // already gone simply leaves the boot id at 0 for the resync slice.
        NetPeerInfo info{};
        if (session->peerInfo(peerId, info)) {
            slot.bootId = info.bootId;
        }
        return static_cast<int8_t>(i);
    }
    return -1;
}

bool NetCounter::applyDelta(Slot& slot, int32_t delta) noexcept {
    const int32_t previous = slot.value;
    slot.value = saturateToI32(static_cast<int64_t>(slot.value) +
                               static_cast<int64_t>(delta));
    return slot.value != previous;
}

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING

