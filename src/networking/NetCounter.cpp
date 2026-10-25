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
    return NetResult::Ok;
}

void NetCounter::shutdown() {
    clearSlots();
    session = nullptr;
}

NetResult NetCounter::add(int16_t delta) {
    if (session == nullptr) {
        return NetResult::NotInitialized;
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
    if (event.type != NetEventType::Message) {
        return;
    }
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

    // No sequence check: reliable delivery is not ordered, addition is
    // commutative, and the session already suppressed duplicates. Rejecting an
    // older sequence here would drop a legitimate operation after a reorder.
    if (applyDelta(slots[static_cast<uint8_t>(index)],
                   readI32LE(&event.payload[1]))) {
        ++revisionValue;
    }
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
