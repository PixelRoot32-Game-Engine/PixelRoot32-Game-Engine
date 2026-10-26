/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the unreliable data path and the receive queue
 * (networking/NetSession.h) as delivered by slice N5.
 *
 * These tests exercise the public send path, the Data receive handling and the
 * event queue. Reliable delivery semantics (ACK, retry, dedupe) belong to
 * test_net_reliability; here only the fact that the reliable contract is
 * accepted is pinned, so this suite keeps owning the unreliable path.
 *
 * The whole networking module is compiled out unless
 * PIXELROOT32_ENABLE_NETWORKING is 1, and the loopback backend only exists on
 * PLATFORM_NATIVE. This file therefore follows the discovery/net_loopback
 * pattern: the functional branch lives behind both guards, and the flag-off
 * branch runs one stub so the suite still proves the inert flags-off contract
 * under [env:native_test].
 *
 * Every session is given an explicit bootId so no test depends on the
 * platform boot-id generator, and every timestamp is injected through
 * update(nowMs) so no test depends on the wall clock.
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "drivers/native/LoopbackTransport.h"
#include "networking/NetProtocol.h"
#include "networking/NetSession.h"
#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace config = pixelroot32::platforms::config;

namespace {

using pixelroot32::networking::LoopbackBus;
using pixelroot32::networking::LoopbackFaults;
using pixelroot32::networking::LoopbackTransport;
using pixelroot32::networking::NetAddress;
using pixelroot32::networking::NetDelivery;
using pixelroot32::networking::NetEvent;
using pixelroot32::networking::NetEventType;
using pixelroot32::networking::NetPacketHeader;
using pixelroot32::networking::NetPacketType;
using pixelroot32::networking::NetPeerConfig;
using pixelroot32::networking::NetPeerInfo;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::encodeFrame;
using pixelroot32::networking::kNetAddressSize;
using pixelroot32::networking::kNetHeaderSize;

// Deterministic, readable addresses. The bytes differ in every position so a
// partial copy cannot pass unnoticed. Tag 1 is the first node, tag 2 the
// second; every other tag is a distinct extra peer.
NetAddress makeAddress(uint8_t tag) {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = static_cast<uint8_t>(0x10u + tag + i);
    }
    return address;
}

NetTransportConfig makeTransportConfig(uint8_t channel = 1) {
    NetTransportConfig transportConfig;
    transportConfig.channel = channel;
    transportConfig.nodeId = 0;
    transportConfig.manageWifi = false;
    return transportConfig;
}

NetSessionConfig makeSessionConfig(uint8_t nodeId, uint16_t bootId,
                                   uint16_t helloIntervalMs =
                                       config::NetHelloIntervalMs,
                                   bool discovery = true) {
    NetSessionConfig sessionConfig;
    sessionConfig.channel = 1;
    sessionConfig.nodeId = nodeId;
    sessionConfig.bootId = bootId;
    sessionConfig.discovery = discovery;
    sessionConfig.helloIntervalMs = helloIntervalMs;
    return sessionConfig;
}

void assertNetResult(NetResult expected, NetResult actual) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),
                                    static_cast<uint8_t>(actual),
                                    "Unexpected NetResult");
}

void assertEventType(NetEventType expected, const NetEvent& event) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),
                                    static_cast<uint8_t>(event.type),
                                    "Unexpected NetEventType");
}

// True when the session's receive queue is empty.
bool hasNoEvent(NetSession& session) {
    NetEvent event{};
    return !session.pollEvent(event);
}

// Brings a pair of sessions up on one bus, on the same channel, and runs the
// three updates needed for both one-shot HELLOs to be drained by both sides.
void discoverPair(NetSession& first, NetSession& second, uint32_t nowMs) {
    first.update(nowMs);
    second.update(nowMs);
    first.update(nowMs);
    second.update(nowMs);
}

void drainEvents(NetSession& session) {
    NetEvent event{};
    while (session.pollEvent(event)) {
    }
}

// Patterned payload whose every byte is derived from its index, so a shifted
// or truncated copy cannot compare equal by accident.
void fillPayload(uint8_t* out, uint16_t length, uint8_t seed) {
    for (uint16_t i = 0; i < length; ++i) {
        out[i] = static_cast<uint8_t>(seed + i * 3u);
    }
}

}  // namespace

// =============================================================================
// 1. An unreliable payload round-trips with its identity and counters.
// =============================================================================

void test_net_data_unreliable_round_trip(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint32_t txBefore = a.stats().txMessages;
    const uint32_t rxBefore = b.stats().rxMessages;

    uint8_t payload[5];
    fillPayload(payload, sizeof(payload), 0x30);

    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Unreliable, payload,
                           static_cast<uint16_t>(sizeof(payload))));
    TEST_ASSERT_EQUAL_UINT32(txBefore + 1, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txBusy);

    b.update(0);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);
    TEST_ASSERT_EQUAL_UINT32(0, event.sequence);
    TEST_ASSERT_EQUAL_UINT16(5, event.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, event.payload, 5);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    TEST_ASSERT_EQUAL_UINT32(rxBefore + 1, b.stats().rxMessages);
    TEST_ASSERT_EQUAL_UINT32(0, b.stats().rxDropped);
    TEST_ASSERT_EQUAL_UINT8(1, b.peerCount());
}

// =============================================================================
// 2. A zero-length payload is accepted and delivered as an empty message.
// =============================================================================

void test_net_data_zero_length_payload_is_delivered(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    // A null payload is allowed exactly when the length says it is empty.
    assertNetResult(NetResult::Ok, a.send(0, NetDelivery::Unreliable, nullptr, 0));

    b.update(0);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);
    TEST_ASSERT_EQUAL_UINT16(0, event.length);
    TEST_ASSERT_TRUE(hasNoEvent(b));
    TEST_ASSERT_EQUAL_UINT32(0, b.stats().rxDropped);
}

// =============================================================================
// 3. send() reports NotInitialized, invalid slots and busy-free argument
//    errors without touching the link.
// =============================================================================

void test_net_data_send_before_init_and_invalid_slots(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);
    uint8_t payload = 1;

    // Neither a ready transport nor an initialized session is enough on its
    // own: the session state is what gates the send path.
    assertNetResult(NetResult::NotInitialized,
                    session.send(0, NetDelivery::Unreliable, &payload, 1));

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::NotInitialized,
                    session.send(0, NetDelivery::Unreliable, &payload, 1));

    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(11, 1)));

    // Out-of-range indexes and never-used slots are argument errors, not
    // "unknown peer" answers: no peer was ever described.
    assertNetResult(NetResult::InvalidArgument,
                    session.send(NetSession::kMaxPeers,
                                 NetDelivery::Unreliable, &payload, 1));
    assertNetResult(NetResult::InvalidArgument,
                    session.send(255, NetDelivery::Unreliable, &payload, 1));
    assertNetResult(NetResult::InvalidArgument,
                    session.send(0, NetDelivery::Unreliable, &payload, 1));

    TEST_ASSERT_EQUAL_UINT32(0, session.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, session.stats().txBusy);
}

// =============================================================================
// 4. send() rejects a null payload with bytes and an oversize payload.
// =============================================================================

void test_net_data_send_rejects_null_and_oversize_payload(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint32_t txBefore = a.stats().txMessages;

    assertNetResult(NetResult::InvalidArgument,
                    a.send(0, NetDelivery::Unreliable, nullptr, 1));

    uint8_t oversize[config::NetMaxPayload + 1];
    fillPayload(oversize, static_cast<uint16_t>(sizeof(oversize)), 0x10);
    assertNetResult(NetResult::PayloadTooLarge,
                    a.send(0, NetDelivery::Unreliable, oversize,
                           static_cast<uint16_t>(sizeof(oversize))));

    // Neither rejection reached the transport.
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txBusy);
    b.update(0);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 5. An offline peer, seeded but never seen, answers PeerNotFound.
// =============================================================================

void test_net_data_send_offline_peer_not_found(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, session.init(
                                       makeSessionConfig(11, 1,
                                                         config::NetHelloIntervalMs,
                                                         false)));

    NetPeerConfig peer;
    peer.address = makeAddress(2);
    peer.nodeId = 22;
    assertNetResult(NetResult::Ok, session.addPeer(peer));

    uint8_t payload = 1;
    assertNetResult(NetResult::PeerNotFound,
                    session.send(0, NetDelivery::Unreliable, &payload, 1));
    assertNetResult(NetResult::PeerNotFound,
                    session.send(0, NetDelivery::Unreliable, nullptr, 0));

    TEST_ASSERT_EQUAL_UINT32(0, session.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, session.stats().txBusy);
}

// =============================================================================
// 6. A reliable send is accepted and delivered; the details of ACK and retry
//    are owned by test_net_reliability.
// =============================================================================

void test_net_data_reliable_send_is_accepted(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint32_t txBefore = a.stats().txMessages;
    const uint32_t rxBefore = b.stats().rxMessages;

    uint8_t payload[4];
    fillPayload(payload, sizeof(payload), 0x50);
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, payload,
                           static_cast<uint16_t>(sizeof(payload))));

    TEST_ASSERT_EQUAL_UINT32(txBefore + 1, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());

    b.update(0);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_EQUAL_UINT16(4, event.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, event.payload, 4);
    TEST_ASSERT_TRUE(hasNoEvent(b));
    TEST_ASSERT_EQUAL_UINT32(rxBefore + 1, b.stats().rxMessages);
}

// =============================================================================
// 7. A Data frame from an unknown MAC is dropped, even with discovery on.
// =============================================================================

void test_net_data_from_unknown_mac_is_dropped(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport strangerTransport(bus, makeAddress(3));
    NetSession a(transportA);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    strangerTransport.init(makeTransportConfig()));
    // Discovery is on: A would accept an unknown HELLO, but not unknown data.
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));

    NetPacketHeader header{};
    header.version = config::NetProtocolVersion;
    header.type = NetPacketType::Data;
    header.flags = 0;
    header.sourceNodeId = 33;
    header.sourceBootId = 3;
    header.sequence = 7;
    header.ackSequence = 0;

    const uint8_t payload[2] = {0xA5u, 0x5Au};
    header.payloadLength = static_cast<uint16_t>(sizeof(payload));

    uint8_t frame[kNetHeaderSize + sizeof(payload)]{};
    const uint16_t frameLength =
        encodeFrame(header, payload, frame,
                    static_cast<uint16_t>(sizeof(frame)));
    TEST_ASSERT_EQUAL_UINT16(sizeof(frame), frameLength);
    assertNetResult(NetResult::Ok,
                    strangerTransport.send(makeAddress(1), frame, frameLength));

    a.update(0);

    // The frame is structurally valid, so it is counted as received, but the
    // sender is not in the peer table and data alone must not create a peer.
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().rxMessages);
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().rxDropped);
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));
}

// =============================================================================
// 8. Data traffic refreshes liveness; the timeout still expires a silent peer.
// =============================================================================

void test_net_data_refreshes_peer_liveness(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    // A long HELLO interval keeps discovery traffic out of the timeout window,
    // so only the Data frame under test can keep the peer alive.
    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    a.init(makeSessionConfig(11, 1, 60000)));
    assertNetResult(NetResult::Ok,
                    b.init(makeSessionConfig(22, 2, 60000)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    uint8_t payload = 0x42u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Unreliable, &payload, 1));

    // The frame is delivered well inside the timeout and becomes the new
    // last-seen timestamp for B.
    b.update(1500);
    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT8(1, b.peerCount());

    NetPeerInfo info{};
    TEST_ASSERT_TRUE(b.peerInfo(0, info));
    TEST_ASSERT_EQUAL_UINT32(1500, info.lastSeenMs);

    // 1500 ms later is still inside the 3000 ms timeout measured from the data.
    b.update(3000);
    TEST_ASSERT_EQUAL_UINT8(1, b.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(b));

    // Another 1500 ms without traffic reaches the timeout and expires the peer.
    b.update(4500);
    TEST_ASSERT_EQUAL_UINT8(0, b.peerCount());
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::PeerLeft, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 9. Data from an offline peer revives it before delivering the message.
// =============================================================================

void test_net_data_revives_offline_peer(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    // A times the peer out while B still believes the link is up, which is
    // exactly the window in which B can send a frame.
    a.update(config::NetPeerTimeoutMs);
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerLeft, event);

    uint8_t payload[3];
    fillPayload(payload, sizeof(payload), 0x60);
    assertNetResult(NetResult::Ok,
                    b.send(0, NetDelivery::Unreliable, payload,
                           static_cast<uint16_t>(sizeof(payload))));

    a.update(config::NetPeerTimeoutMs);

    // A frame from the slot revives it as a join, then the payload arrives.
    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerJoined, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);

    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);
    TEST_ASSERT_EQUAL_UINT16(3, event.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, event.payload, 3);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    NetPeerInfo info{};
    TEST_ASSERT_TRUE(a.peerInfo(0, info));
    TEST_ASSERT_TRUE(info.online);
}

// =============================================================================
// 10. Queue overflow drops the newest message and counts it.
// =============================================================================

void test_net_data_queue_overflow_drops_newest(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    a.init(makeSessionConfig(11, 1, 60000)));
    assertNetResult(NetResult::Ok,
                    b.init(makeSessionConfig(22, 2, 60000)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    // One transport frame per iteration: the transport queue is never the
    // limit, only the session event queue is. Each send is followed by both
    // updates so the pending send result cannot make a later send Busy.
    const uint16_t total = static_cast<uint16_t>(NetSession::kRxQueueCapacity + 1);
    for (uint16_t i = 0; i < total; ++i) {
        const uint8_t payload = static_cast<uint8_t>(i);
        assertNetResult(NetResult::Ok,
                        a.send(0, NetDelivery::Unreliable, &payload, 1));
        a.update(0);
        b.update(0);
    }

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(total),
                             b.stats().rxMessages - 1);
    TEST_ASSERT_EQUAL_UINT32(1, b.stats().rxDropped);

    // Exactly the first capacity messages survived, in FIFO order; the last
    // one was discarded.
    for (uint16_t i = 0; i < NetSession::kRxQueueCapacity; ++i) {
        NetEvent event{};
        TEST_ASSERT_TRUE(b.pollEvent(event));
        assertEventType(NetEventType::Message, event);
        TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
        TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);
        TEST_ASSERT_EQUAL_UINT16(1, event.length);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(i), event.payload[0]);
    }
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 11. The configured maximum payload survives the round trip intact.
// =============================================================================

void test_net_data_maximum_payload_is_delivered_intact(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    uint8_t payload[config::NetMaxPayload];
    fillPayload(payload, static_cast<uint16_t>(sizeof(payload)), 0x01);

    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Unreliable, payload,
                           static_cast<uint16_t>(sizeof(payload))));

    b.update(0);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT16(config::NetMaxPayload, event.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, event.payload, config::NetMaxPayload);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 12. A busy transport backpressures an unreliable send, while a reliable send
//     is accepted into the pending queue instead of being lost.
// =============================================================================

void test_net_data_busy_transport_contract(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint32_t txBefore = a.stats().txMessages;
    const uint32_t rxBefore = b.stats().rxMessages;

    LoopbackFaults faults;
    faults.sendBusy = true;
    bus.setFaults(faults);

    uint8_t payload = 0x77u;
    // Unreliable has nowhere to keep the frame, so it reports the transport's
    // backpressure to the caller and queues nothing.
    assertNetResult(NetResult::Busy,
                    a.send(0, NetDelivery::Unreliable, &payload, 1));
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);

    // Reliable accepts the message: the session owns the sequence and keeps
    // the frame for a later update, so a busy transport is not a lost message.
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &payload, 1));
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(2, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);

    b.update(0);
    TEST_ASSERT_EQUAL_UINT32(rxBefore, b.stats().rxMessages);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_data_path_flag_off_stub_compiles(void) {
    // With the flag off (or on a target without the native loopback), neither
    // NetSession nor the loopback backend is compiled. This translation unit
    // compiling and passing without referencing any networking symbol is the
    // zero-cost flags-off property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_NETWORKING=0: no session or loopback symbols are "
        "compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)
    RUN_TEST(test_net_data_unreliable_round_trip);
    RUN_TEST(test_net_data_zero_length_payload_is_delivered);
    RUN_TEST(test_net_data_send_before_init_and_invalid_slots);
    RUN_TEST(test_net_data_send_rejects_null_and_oversize_payload);
    RUN_TEST(test_net_data_send_offline_peer_not_found);
    RUN_TEST(test_net_data_reliable_send_is_accepted);
    RUN_TEST(test_net_data_from_unknown_mac_is_dropped);
    RUN_TEST(test_net_data_refreshes_peer_liveness);
    RUN_TEST(test_net_data_revives_offline_peer);
    RUN_TEST(test_net_data_queue_overflow_drops_newest);
    RUN_TEST(test_net_data_maximum_payload_is_delivered_intact);
    RUN_TEST(test_net_data_busy_transport_contract);
#else
    RUN_TEST(test_net_data_path_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
