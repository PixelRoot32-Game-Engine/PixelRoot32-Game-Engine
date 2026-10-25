/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "networking/NetSession.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include "networking/NetProtocol.h"

#if defined(ESP32)
#include <esp_system.h>
#else
#include <ctime>
#endif

namespace pixelroot32::networking {

namespace {

/**
 * @brief Width of the HELLO uptime payload, in bytes.
 *
 * HELLO carries exactly one field, the sender's uptime, so a frame with a
 * different payload length describes a dialect this build cannot interpret and
 * must be rejected as a whole rather than partially trusted.
 */
constexpr uint16_t kHelloPayloadSize = 4;

/**
 * @brief Decodes the little-endian uptime stored in a HELLO payload.
 * @param payload Pointer to four payload bytes.
 * @return Uptime in milliseconds as advertised by the sender.
 */
uint32_t readUptime(const uint8_t* payload) noexcept {
    return static_cast<uint32_t>(payload[0]) |
           (static_cast<uint32_t>(payload[1]) << 8) |
           (static_cast<uint32_t>(payload[2]) << 16) |
           (static_cast<uint32_t>(payload[3]) << 24);
}

/**
 * @brief Encodes an uptime as four little-endian bytes.
 * @param out Destination of exactly four bytes.
 * @param uptimeMs Uptime to encode.
 */
void writeUptime(uint8_t* out, uint32_t uptimeMs) noexcept {
    out[0] = static_cast<uint8_t>(uptimeMs & 0xFFu);
    out[1] = static_cast<uint8_t>((uptimeMs >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((uptimeMs >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((uptimeMs >> 24) & 0xFFu);
}

/**
 * @brief Builds the ESP-NOW broadcast address.
 * @return Address with every byte set to 0xFF.
 */
NetAddress broadcastAddress() noexcept {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = 0xFFu;
    }
    return address;
}

/**
 * @brief Derives an application label from a MAC address.
 * @param address Local link address.
 * @return Last MAC byte, with 0 and 255 mapped to 1.
 *
 * 0 means "derive" in the configuration and 255 is the broadcast marker, so
 * neither may become the local label of a real node.
 */
uint8_t deriveNodeId(const NetAddress& address) noexcept {
    const uint8_t lastByte = address.bytes[kNetAddressSize - 1];
    if (lastByte == 0x00u || lastByte == 0xFFu) {
        return 1;
    }
    return lastByte;
}

/**
 * @brief Generates a non-zero boot generation.
 * @param address Local link address, mixed in on native builds.
 * @return A value that differs between boots and is never zero.
 *
 * A boot id is a generation marker, not a random number: two boots of the same
 * node must differ so peers can reset their per-peer dedupe state. ESP32 has a
 * hardware RNG for it; native mixes the wall clock, a process-local counter and
 * the local MAC because it has no RNG contract and must not allocate.
 */
uint16_t generateBootId(const NetAddress& address) noexcept {
#if defined(ESP32)
    const uint16_t value = static_cast<uint16_t>(esp_random() & 0xFFFFu);
    return (value != 0) ? value : 1;
#else
    static uint16_t bootCounter = 0;
    ++bootCounter;

    uint32_t mix = static_cast<uint32_t>(std::time(nullptr));
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        mix = mix * 31u + address.bytes[i];
    }
    mix ^= static_cast<uint32_t>(bootCounter) * 2654435761u;

    const uint16_t folded = static_cast<uint16_t>(mix ^ (mix >> 16));
    return (folded != 0) ? folded : 1;
#endif
}

}  // namespace

// =============================================================================
// Construction and lifecycle
// =============================================================================

NetSession::NetSession(NetTransport& transportRef) : transport(transportRef) {}

NetResult NetSession::init(const NetSessionConfig& newConfig) {
    // The session never brings the link up: a caller that forgot to initialize
    // the transport gets a clear answer instead of a half-working session.
    if (!transport.isReady()) {
        return NetResult::NotInitialized;
    }

    const NetResult channelResult = transport.setChannel(newConfig.channel);
    if (channelResult != NetResult::Ok) {
        return channelResult;
    }

    config = newConfig;

    nodeIdValue = (config.nodeId != 0)
                      ? config.nodeId
                      : deriveNodeId(transport.localAddress());

    // An explicit boot id always wins. When the application lets the session
    // generate one, the first generated value is kept for the object's whole
    // lifetime: a re-init reconfigures a running session, it is not a reboot.
    if (config.bootId != 0) {
        bootIdValue = config.bootId;
    } else if (bootIdValue == 0) {
        bootIdValue = generateBootId(transport.localAddress());
    }

    // Events describe live traffic, so a (re)start begins with an empty queue.
    // Peers added through addPeer are configuration and survive: a locked
    // deployment seeds them before or after init without losing them.
    clearEvents();
    nextHelloMs = 0;
    helloPending = config.discovery;
    initialized = true;

    if (config.discovery && !broadcastRegistered) {
        NetPeerConfig broadcast;
        broadcast.address = broadcastAddress();
        broadcast.nodeId = 0;
        broadcast.encrypted = false;
        transport.addPeer(broadcast);
        broadcastRegistered = true;
    }

    return NetResult::Ok;
}

void NetSession::shutdown() {
    if (!initialized) {
        return;
    }

    // Best effort: if the link is already gone the BYE is simply rejected.
    sendBye();

    clearPeers();
    clearEvents();
    initialized = false;
    helloPending = false;
    broadcastRegistered = false;
    nextHelloMs = 0;
}

void NetSession::update(uint32_t nowMs) {
    if (!initialized) {
        return;
    }

    // helloPending makes the first update after init send immediately instead
    // of waiting a full interval, so peers meet as soon as the loop starts.
    if (config.discovery &&
        (helloPending ||
         static_cast<int32_t>(nowMs - nextHelloMs) >= 0)) {
        helloPending = false;
        sendHello(nowMs);
        nextHelloMs = nowMs + static_cast<uint32_t>(config.helloIntervalMs);
    }

    // Reliability owns these results later, but they must be consumed now:
    // otherwise the transport's fixed result queue fills up and sends start
    // reporting Busy for no reason.
    NetTxResult txResult{};
    while (transport.pollTxResult(txResult)) {
    }

    // Bound the drain so a peer flooding the link cannot starve the game loop.
    // Undrained frames stay in the transport for the next update.
    uint8_t drained = 0;
    NetRxInfo rxFrame{};
    while (drained < kMaxRxFramesPerUpdate && transport.pollRx(rxFrame)) {
        ++drained;
        handleFrame(rxFrame, nowMs);
    }

    expirePeers(nowMs);
}

NetResult NetSession::addPeer(const NetPeerConfig& peer) {
    if (peer.address.isZero() || peer.address.isBroadcast()) {
        return NetResult::InvalidArgument;
    }

    // Re-registering an address is idempotent: it must not consume a second
    // slot, and it must not disturb whether the peer is currently online.
    if (findPeer(peer.address) >= 0) {
        return NetResult::Ok;
    }

    const int8_t slot = allocatePeerSlot();
    if (slot < 0) {
        return NetResult::PeerTableFull;
    }

    PeerSlot& entry = peers[static_cast<uint8_t>(slot)];
    entry.address = peer.address;
    entry.nodeId = peer.nodeId;
    entry.bootId = 0;
    entry.lastSeenMs = 0;
    entry.uptimeMs = 0;
    entry.used = true;
    entry.online = false;

    NetPeerConfig registration;
    registration.address = peer.address;
    registration.nodeId = peer.nodeId;
    transport.addPeer(registration);

    return NetResult::Ok;
}

// =============================================================================
// Queries
// =============================================================================

bool NetSession::pollEvent(NetEvent& out) {
    if (eventCount == 0) {
        return false;
    }

    out = events[eventHead];
    eventHead = static_cast<uint16_t>((eventHead + 1) % kRxQueueCapacity);
    --eventCount;
    return true;
}

bool NetSession::peerInfo(uint8_t peerId, NetPeerInfo& out) const {
    if (peerId >= kMaxPeers || !peers[peerId].used) {
        return false;
    }

    const PeerSlot& peer = peers[peerId];
    out.address = peer.address;
    out.nodeId = peer.nodeId;
    out.bootId = peer.bootId;
    out.rttMs = 0;  // RTT arrives with the reliable path.
    out.lastSeenMs = peer.lastSeenMs;
    out.uptimeMs = peer.uptimeMs;
    out.online = peer.online;
    return true;
}

uint8_t NetSession::peerCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < kMaxPeers; ++i) {
        if (peers[i].used && peers[i].online) {
            ++count;
        }
    }
    return count;
}

const NetStats& NetSession::stats() const {
    return counters;
}

uint8_t NetSession::localNodeId() const {
    return nodeIdValue;
}

uint16_t NetSession::localBootId() const {
    return bootIdValue;
}

// =============================================================================
// Outgoing frames
// =============================================================================

bool NetSession::sendHello(uint32_t nowMs) noexcept {
    uint8_t payload[kHelloPayloadSize];
    writeUptime(payload, nowMs);

    NetPacketHeader header{};
    header.version = platforms::config::NetProtocolVersion;
    header.type = NetPacketType::Hello;
    header.flags = 0;
    header.sourceNodeId = nodeIdValue;
    header.sourceBootId = bootIdValue;
    header.sequence = 0;
    header.ackSequence = 0;
    header.payloadLength = kHelloPayloadSize;

    uint8_t frame[kNetTransportFrameSize]{};
    const uint16_t length = encodeFrame(header, payload, frame, sizeof(frame));
    if (length == 0) {
        return false;
    }

    if (transport.send(broadcastAddress(), frame, length) != NetResult::Ok) {
        return false;
    }

    ++counters.txMessages;
    return true;
}

void NetSession::sendBye() noexcept {
    NetPacketHeader header{};
    header.version = platforms::config::NetProtocolVersion;
    header.type = NetPacketType::Bye;
    header.flags = 0;
    header.sourceNodeId = nodeIdValue;
    header.sourceBootId = bootIdValue;
    header.sequence = 0;
    header.ackSequence = 0;
    header.payloadLength = 0;

    uint8_t frame[kNetHeaderSize]{};
    const uint16_t length = encodeFrame(header, nullptr, frame, sizeof(frame));
    if (length == 0) {
        return;
    }

    if (transport.send(broadcastAddress(), frame, length) == NetResult::Ok) {
        ++counters.txMessages;
    }
}

// =============================================================================
// Incoming frames
// =============================================================================

void NetSession::handleFrame(const NetRxInfo& frame, uint32_t nowMs) noexcept {
    // The zero and broadcast addresses can never name a peer; a frame claiming
    // one of them is malformed at the identity level.
    if (frame.from.isZero() || frame.from.isBroadcast()) {
        ++counters.rxDropped;
        return;
    }

    NetPacketHeader header{};
    const uint8_t* payload = nullptr;
    if (decodeFrame(frame.data, frame.length, header, &payload) !=
        NetParseResult::Ok) {
        ++counters.rxDropped;
        return;
    }

    // Every structurally valid frame counts as received, even when its content
    // is later rejected or belongs to a slice that is not implemented yet.
    ++counters.rxMessages;

    switch (header.type) {
        case NetPacketType::Hello:
            if (header.payloadLength != kHelloPayloadSize ||
                payload == nullptr) {
                ++counters.rxDropped;
                return;
            }
            handleHello(frame.from, header.sourceNodeId, header.sourceBootId,
                        readUptime(payload), nowMs);
            return;

        case NetPacketType::Bye:
            if (header.payloadLength != 0) {
                ++counters.rxDropped;
                return;
            }
            handleBye(frame.from);
            return;

        case NetPacketType::Data:
        case NetPacketType::Ack:
        case NetPacketType::Snapshot:
            // Counted above and otherwise ignored: the data path, reliability
            // and the sync layer own these frames in later slices.
            return;
    }
}

void NetSession::handleHello(const NetAddress& from, uint8_t nodeId,
                             uint16_t bootId, uint32_t uptimeMs,
                             uint32_t nowMs) noexcept {
    const int8_t existing = findPeer(from);
    if (existing >= 0) {
        PeerSlot& peer = peers[static_cast<uint8_t>(existing)];
        peer.nodeId = nodeId;
        // A different boot id means the peer restarted. Later slices reset the
        // dedupe window here; the slot and its index are kept either way.
        peer.bootId = bootId;
        peer.lastSeenMs = nowMs;
        peer.uptimeMs = uptimeMs;

        if (!peer.online) {
            // Reviving a timed-out peer is a join again: it is the signal the
            // sync layer uses to decide a resync is needed.
            peer.online = true;
            NetEvent event;
            event.type = NetEventType::PeerJoined;
            event.peerId = static_cast<uint8_t>(existing);
            event.nodeId = nodeId;
            pushEvent(event);
        }
        return;
    }

    // With discovery disabled only pre-seeded addresses are allowed in.
    if (!config.discovery) {
        ++counters.rxDropped;
        return;
    }

    const int8_t slot = allocatePeerSlot();
    if (slot < 0) {
        // The table is full: drop the frame rather than evicting a peer that
        // may still be exchanging reliable traffic.
        ++counters.rxDropped;
        return;
    }

    PeerSlot& peer = peers[static_cast<uint8_t>(slot)];
    peer.address = from;
    peer.nodeId = nodeId;
    peer.bootId = bootId;
    peer.lastSeenMs = nowMs;
    peer.uptimeMs = uptimeMs;
    peer.used = true;
    peer.online = true;

    NetPeerConfig registration;
    registration.address = from;
    registration.nodeId = nodeId;
    transport.addPeer(registration);

    NetEvent event;
    event.type = NetEventType::PeerJoined;
    event.peerId = static_cast<uint8_t>(slot);
    event.nodeId = nodeId;
    pushEvent(event);
}

void NetSession::handleBye(const NetAddress& from) noexcept {
    const int8_t slot = findPeer(from);
    if (slot < 0) {
        // A peer this session never knew is leaving: not an error, and not an
        // event, because there is nothing to change.
        return;
    }

    PeerSlot& peer = peers[static_cast<uint8_t>(slot)];
    if (!peer.online) {
        // Already offline: a duplicate BYE must not emit a second PeerLeft.
        return;
    }

    peer.online = false;
    NetEvent event;
    event.type = NetEventType::PeerLeft;
    event.peerId = static_cast<uint8_t>(slot);
    event.nodeId = peer.nodeId;
    pushEvent(event);
}

void NetSession::expirePeers(uint32_t nowMs) noexcept {
    for (uint8_t i = 0; i < kMaxPeers; ++i) {
        PeerSlot& peer = peers[i];
        if (!peer.used || !peer.online) {
            continue;
        }

        // Unsigned subtraction makes the comparison correct across the 32-bit
        // millisecond wrap; a signed subtraction would invert near 2^32.
        if (static_cast<uint32_t>(nowMs - peer.lastSeenMs) >=
            static_cast<uint32_t>(config.peerTimeoutMs)) {
            peer.online = false;
            NetEvent event;
            event.type = NetEventType::PeerLeft;
            event.peerId = i;
            event.nodeId = peer.nodeId;
            pushEvent(event);
        }
    }
}

// =============================================================================
// Fixed table helpers
// =============================================================================

int8_t NetSession::findPeer(const NetAddress& address) const noexcept {
    for (uint8_t i = 0; i < kMaxPeers; ++i) {
        if (peers[i].used && peers[i].address == address) {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

int8_t NetSession::allocatePeerSlot() noexcept {
    for (uint8_t i = 0; i < kMaxPeers; ++i) {
        if (!peers[i].used) {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

void NetSession::clearPeers() noexcept {
    for (uint8_t i = 0; i < kMaxPeers; ++i) {
        peers[i] = PeerSlot{};
    }
}

void NetSession::clearEvents() noexcept {
    eventHead = 0;
    eventTail = 0;
    eventCount = 0;
}

void NetSession::pushEvent(const NetEvent& event) noexcept {
    // Drop-newest: an event that cannot fit is discarded and counted, so the
    // consumer keeps the events that were already promised FIFO order.
    if (eventCount >= kRxQueueCapacity) {
        ++counters.rxDropped;
        return;
    }

    events[eventTail] = event;
    eventTail = static_cast<uint16_t>((eventTail + 1) % kRxQueueCapacity);
    ++eventCount;
}

}  // namespace pixelroot32::networking

#endif  // PIXELROOT32_ENABLE_NETWORKING
