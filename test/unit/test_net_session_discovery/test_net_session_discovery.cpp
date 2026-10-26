/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for session discovery and peer lifecycle
 * (networking/NetSession.h) as delivered by slice N4.
 *
 * These tests exercise the HELLO/BYE discovery path, the fixed peer table and
 * the offline timeout. The public data send path, ACK/retry, dedupe,
 * backpressure and the sync counter belong to later slices and are deliberately
 * not asserted here.
 *
 * The whole networking module is compiled out unless
 * PIXELROOT32_ENABLE_NETWORKING is 1, and the loopback backend only exists on
 * PLATFORM_NATIVE. This file therefore follows the dialog/net_loopback pattern:
 * the functional branch lives behind both guards, and the flag-off branch runs
 * one stub so the suite still proves the inert flags-off contract under
 * [env:native_test].
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
using pixelroot32::networking::LoopbackTransport;
using pixelroot32::networking::NetAddress;
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

// Address whose final byte is chosen explicitly, used to pin the node-id
// derivation for the two reserved last-byte values 0x00 and 0xFF.
NetAddress makeAddressWithLastByte(uint8_t tag, uint8_t lastByte) {
    NetAddress address = makeAddress(tag);
    address.bytes[kNetAddressSize - 1] = lastByte;
    return address;
}

NetAddress makeBroadcastAddress() {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = 0xFFu;
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
                                   bool discovery = true) {
    NetSessionConfig sessionConfig;
    sessionConfig.channel = 1;
    sessionConfig.nodeId = nodeId;
    sessionConfig.bootId = bootId;
    sessionConfig.discovery = discovery;
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

}  // namespace

// =============================================================================
// 1. init() requires a ready transport.
// =============================================================================

void test_net_session_init_requires_ready_transport(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);

    // The session never initializes the transport: an uninitialized transport
    // is reported, not repaired.
    assertNetResult(NetResult::NotInitialized,
                    session.init(makeSessionConfig(1, 1)));
    TEST_ASSERT_EQUAL_UINT8(0, session.localNodeId());
    TEST_ASSERT_EQUAL_UINT8(0, session.peerCount());
    TEST_ASSERT_FALSE(transport.isReady());

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(1, 1)));
    TEST_ASSERT_EQUAL_UINT8(1, session.localNodeId());
    TEST_ASSERT_EQUAL_UINT16(1, session.localBootId());
}

// =============================================================================
// 2. localNodeId(): derive from the MAC, or take the explicit label.
// =============================================================================

void test_net_session_local_node_id_derives_from_mac_and_override(void) {
    LoopbackBus bus;
    LoopbackTransport derivedTransport(bus, makeAddress(1));  // last byte 0x16
    LoopbackTransport zeroTransport(bus, makeAddressWithLastByte(2, 0x00u));
    LoopbackTransport allOnesTransport(bus, makeAddressWithLastByte(3, 0xFFu));
    LoopbackTransport explicitTransport(bus, makeAddress(4));

    NetSession derived(derivedTransport);
    NetSession zeroLastByte(zeroTransport);
    NetSession allOnesLastByte(allOnesTransport);
    NetSession explicitLabel(explicitTransport);

    assertNetResult(NetResult::Ok, derivedTransport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, zeroTransport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, allOnesTransport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    explicitTransport.init(makeTransportConfig()));

    assertNetResult(NetResult::Ok, derived.init(makeSessionConfig(0, 1)));
    TEST_ASSERT_EQUAL_UINT8(0x16, derived.localNodeId());

    assertNetResult(NetResult::Ok, zeroLastByte.init(makeSessionConfig(0, 1)));
    TEST_ASSERT_EQUAL_UINT8(1, zeroLastByte.localNodeId());

    assertNetResult(NetResult::Ok,
                    allOnesLastByte.init(makeSessionConfig(0, 1)));
    TEST_ASSERT_EQUAL_UINT8(1, allOnesLastByte.localNodeId());

    assertNetResult(NetResult::Ok, explicitLabel.init(makeSessionConfig(77, 1)));
    TEST_ASSERT_EQUAL_UINT8(77, explicitLabel.localNodeId());
}

// =============================================================================
// 3. Two sessions discover each other with exactly one PeerJoined each.
// =============================================================================

void test_net_session_two_sessions_discover_each_other(void) {
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

    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    TEST_ASSERT_EQUAL_UINT8(1, b.peerCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerJoined, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::PeerJoined, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    NetPeerInfo info{};
    TEST_ASSERT_TRUE(a.peerInfo(0, info));
    TEST_ASSERT_TRUE(info.address == makeAddress(2));
    TEST_ASSERT_EQUAL_UINT8(22, info.nodeId);
    TEST_ASSERT_EQUAL_UINT16(2, info.bootId);
    TEST_ASSERT_TRUE(info.online);

    TEST_ASSERT_TRUE(b.peerInfo(0, info));
    TEST_ASSERT_TRUE(info.address == makeAddress(1));
    TEST_ASSERT_EQUAL_UINT8(11, info.nodeId);
    TEST_ASSERT_EQUAL_UINT16(1, info.bootId);
    TEST_ASSERT_TRUE(info.online);

    // One broadcast HELLO out, one HELLO in, per session.
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().rxMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().rxDropped);
}

// =============================================================================
// 4. Repeated HELLOs refresh the peer without a second PeerJoined.
// =============================================================================

void test_net_session_repeated_hello_does_not_duplicate_join(void) {
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
    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    drainEvents(a);
    drainEvents(b);

    // The configured default interval is 1000 ms, so this exchange is due.
    discoverPair(a, b, 1000);

    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    TEST_ASSERT_EQUAL_UINT8(1, b.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));
    TEST_ASSERT_TRUE(hasNoEvent(b));

    NetPeerInfo info{};
    TEST_ASSERT_TRUE(a.peerInfo(0, info));
    TEST_ASSERT_EQUAL_UINT32(1000, info.lastSeenMs);
    TEST_ASSERT_EQUAL_UINT32(1000, info.uptimeMs);
}

// =============================================================================
// 5. Peer timeout emits PeerLeft; a later HELLO revives the slot.
// =============================================================================

void test_net_session_peer_timeout_emits_left_and_hello_revives(void) {
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

    // B stops updating, so A sees no HELLO for exactly the 3000 ms timeout.
    a.update(config::NetPeerTimeoutMs);

    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerLeft, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    NetPeerInfo info{};
    TEST_ASSERT_TRUE(a.peerInfo(0, info));
    TEST_ASSERT_FALSE(info.online);

    // The slot survives: a later HELLO marks it online and joins it again.
    b.update(config::NetPeerTimeoutMs + 1);
    a.update(config::NetPeerTimeoutMs + 1);

    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerJoined, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);
    TEST_ASSERT_TRUE(a.peerInfo(0, info));
    TEST_ASSERT_TRUE(info.online);
}

// =============================================================================
// 6. shutdown() sends a best-effort BYE that emits PeerLeft on the peer.
// =============================================================================

void test_net_session_shutdown_bye_emits_peer_left(void) {
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
    drainEvents(b);
    TEST_ASSERT_EQUAL_UINT8(1, b.peerCount());

    a.shutdown();

    // shutdown() clears the session but leaves the transport alone.
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    TEST_ASSERT_TRUE(transportA.isReady());

    b.update(0);

    TEST_ASSERT_EQUAL_UINT8(0, b.peerCount());
    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::PeerLeft, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, event.nodeId);

    NetPeerInfo info{};
    TEST_ASSERT_TRUE(b.peerInfo(0, info));
    TEST_ASSERT_FALSE(info.online);
}

// =============================================================================
// 7. discovery = false: unknown HELLOs are dropped until the peer is seeded.
// =============================================================================

void test_net_session_discovery_off_ignores_unknown_hello_until_preseeded(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1, false)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    b.update(0);
    a.update(0);

    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().rxDropped);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    // Pre-seeding registers the address but does not claim the peer is online.
    NetPeerConfig seed;
    seed.address = makeAddress(2);
    seed.nodeId = 22;
    assertNetResult(NetResult::Ok, a.addPeer(seed));
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());

    // The same HELLO is now accepted because the address is known.
    b.update(config::NetHelloIntervalMs);
    a.update(config::NetHelloIntervalMs);

    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerJoined, event);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);
}

// =============================================================================
// 8. addPeer() validation, idempotence and table-full handling.
// =============================================================================

void test_net_session_add_peer_validation_and_table_full(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(11, 1, false)));

    NetPeerConfig peer;
    peer.address = NetAddress{};
    assertNetResult(NetResult::InvalidArgument, session.addPeer(peer));

    peer.address = makeBroadcastAddress();
    assertNetResult(NetResult::InvalidArgument, session.addPeer(peer));

    peer.address = makeAddress(2);
    peer.nodeId = 22;
    assertNetResult(NetResult::Ok, session.addPeer(peer));
    // Registering the same address again is a no-op, not a second slot.
    assertNetResult(NetResult::Ok, session.addPeer(peer));
    TEST_ASSERT_EQUAL_UINT8(0, session.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(session));

    for (uint8_t i = 1; i < NetSession::kMaxPeers; ++i) {
        peer.address = makeAddress(static_cast<uint8_t>(0x20u + i));
        assertNetResult(NetResult::Ok, session.addPeer(peer));
    }

    peer.address = makeAddress(0x40u);
    assertNetResult(NetResult::PeerTableFull, session.addPeer(peer));
    TEST_ASSERT_EQUAL_UINT8(0, session.peerCount());
}

// =============================================================================
// 9. A malformed frame is counted and dropped without producing an event.
// =============================================================================

void test_net_session_malformed_frame_is_dropped(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    // Discovery is off so this session emits no traffic of its own.
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1, false)));

    uint8_t badMagic[kNetHeaderSize]{};
    assertNetResult(NetResult::Ok,
                    transportB.send(makeAddress(1), badMagic,
                                    static_cast<uint16_t>(sizeof(badMagic))));

    a.update(0);

    TEST_ASSERT_EQUAL_UINT32(1, a.stats().rxDropped);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().rxMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));

    // A frame shorter than the header is dropped for the same reason.
    const uint8_t truncated[3] = {0x50u, 0x52u, 0x01u};
    assertNetResult(NetResult::Ok,
                    transportB.send(makeAddress(1), truncated, 3));
    a.update(1);

    TEST_ASSERT_EQUAL_UINT32(2, a.stats().rxDropped);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().rxMessages);
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));
}

// =============================================================================
// 10. A HELLO payload must be exactly four uptime bytes.
// =============================================================================

void test_net_session_hello_payload_length_is_validated(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    // Discovery on: a well-formed HELLO would allocate a peer here, so a
    // wrong-length HELLO must not.
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    NetPacketHeader header{};
    header.version = config::NetProtocolVersion;
    header.type = NetPacketType::Hello;
    header.flags = 0;
    header.sourceNodeId = 22;
    header.sourceBootId = 2;
    header.sequence = 0;
    header.ackSequence = 0;
    header.payloadLength = 0;

    uint8_t frame[kNetHeaderSize]{};
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize,
                             encodeFrame(header, nullptr, frame,
                                         static_cast<uint16_t>(sizeof(frame))));
    assertNetResult(NetResult::Ok,
                    transportB.send(makeAddress(1), frame, kNetHeaderSize));

    a.update(0);

    TEST_ASSERT_EQUAL_UINT32(1, a.stats().rxDropped);
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));

    // The slot table is untouched, so a valid HELLO still joins the peer.
    b.update(config::NetHelloIntervalMs);
    a.update(config::NetHelloIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerJoined, event);
    TEST_ASSERT_EQUAL_UINT8(22, event.nodeId);
}

// =============================================================================
// 11. Timeout arithmetic is wrap-safe around 2^32.
// =============================================================================

void test_net_session_timeout_wraps_at_32_bits(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));

    // A long HELLO interval keeps discovery traffic out of the timeout window;
    // the wrap comes from the clock, not from an extra beacon.
    NetSessionConfig configA = makeSessionConfig(11, 1);
    configA.helloIntervalMs = 60000;
    NetSessionConfig configB = makeSessionConfig(22, 2);
    configB.helloIntervalMs = 60000;
    assertNetResult(NetResult::Ok, a.init(configA));
    assertNetResult(NetResult::Ok, b.init(configB));

    const uint32_t start = 0xFFFFFF00u;
    a.update(start);
    b.update(start);
    a.update(start);
    b.update(start);
    drainEvents(a);
    drainEvents(b);
    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());

    // Cross the wrap but stay inside the timeout.
    a.update(start + 2000u);
    TEST_ASSERT_EQUAL_UINT8(1, a.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));

    // Cross the timeout on the far side of the wrap.
    a.update(start + 4000u);
    TEST_ASSERT_EQUAL_UINT8(0, a.peerCount());
    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::PeerLeft, event);
}

// =============================================================================
// 12. init() and shutdown() are idempotent; update() outside them is a no-op.
// =============================================================================

void test_net_session_init_shutdown_are_idempotent(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);

    // Before init: update() must not touch anything.
    session.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, session.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(session));

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(11, 1)));
    TEST_ASSERT_EQUAL_UINT8(11, session.localNodeId());
    TEST_ASSERT_EQUAL_UINT16(1, session.localBootId());

    session.shutdown();
    session.shutdown();
    TEST_ASSERT_TRUE(transport.isReady());
    TEST_ASSERT_EQUAL_UINT8(0, session.peerCount());

    session.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, session.peerCount());
    TEST_ASSERT_TRUE(hasNoEvent(session));

    // A session can be brought back up on the same transport.
    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(11, 1)));
    TEST_ASSERT_EQUAL_UINT8(11, session.localNodeId());
}

// =============================================================================
// 13. peerInfo() rejects out-of-range and unused slots.
// =============================================================================

void test_net_session_peer_info_rejects_invalid_slots(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    session.init(makeSessionConfig(11, 1, false)));

    NetPeerInfo info{};
    TEST_ASSERT_FALSE(session.peerInfo(0, info));
    TEST_ASSERT_FALSE(session.peerInfo(NetSession::kMaxPeers, info));
    TEST_ASSERT_FALSE(session.peerInfo(255, info));

    NetPeerConfig peer;
    peer.address = makeAddress(2);
    peer.nodeId = 22;
    assertNetResult(NetResult::Ok, session.addPeer(peer));

    TEST_ASSERT_TRUE(session.peerInfo(0, info));
    TEST_ASSERT_TRUE(info.address == makeAddress(2));
    TEST_ASSERT_EQUAL_UINT8(22, info.nodeId);
    TEST_ASSERT_FALSE(info.online);
    TEST_ASSERT_FALSE(session.peerInfo(1, info));
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_session_discovery_flag_off_stub_compiles(void) {
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
    RUN_TEST(test_net_session_init_requires_ready_transport);
    RUN_TEST(test_net_session_local_node_id_derives_from_mac_and_override);
    RUN_TEST(test_net_session_two_sessions_discover_each_other);
    RUN_TEST(test_net_session_repeated_hello_does_not_duplicate_join);
    RUN_TEST(test_net_session_peer_timeout_emits_left_and_hello_revives);
    RUN_TEST(test_net_session_shutdown_bye_emits_peer_left);
    RUN_TEST(test_net_session_discovery_off_ignores_unknown_hello_until_preseeded);
    RUN_TEST(test_net_session_add_peer_validation_and_table_full);
    RUN_TEST(test_net_session_malformed_frame_is_dropped);
    RUN_TEST(test_net_session_hello_payload_length_is_validated);
    RUN_TEST(test_net_session_timeout_wraps_at_32_bits);
    RUN_TEST(test_net_session_init_shutdown_are_idempotent);
    RUN_TEST(test_net_session_peer_info_rejects_invalid_slots);
#else
    RUN_TEST(test_net_session_discovery_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
