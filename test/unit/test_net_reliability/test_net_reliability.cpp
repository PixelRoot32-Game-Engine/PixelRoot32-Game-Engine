/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for reliable delivery (networking/NetSession.h) as delivered by
 * slice N6.
 *
 * These tests exercise the ACK/retry path, the per-peer duplicate window, the
 * pending reliable queue, SendFailed on give-up and the dedupe reset when a
 * peer reboots. They drive two sessions over the loopback bus with an explicit
 * clock: no test sleeps or reads millis(), so every timeout fires exactly at
 * the asserted millisecond.
 *
 * The whole networking module is compiled out unless
 * PIXELROOT32_ENABLE_NETWORKING is 1, and the loopback backend only exists on
 * PLATFORM_NATIVE. This file therefore follows the discovery/data-path
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
using pixelroot32::networking::NetParseResult;
using pixelroot32::networking::NetPeerConfig;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetRxInfo;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::decodeFrame;
using pixelroot32::networking::encodeFrame;
using pixelroot32::networking::kNetAddressSize;
using pixelroot32::networking::kNetFlagReliable;
using pixelroot32::networking::kNetHeaderSize;
using pixelroot32::networking::kNetTransportFrameSize;

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
// four updates needed for both one-shot HELLOs to be drained by both sides.
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

// Builds one reliable Data frame and hands it straight to `sender`, bypassing
// the session send path so a test can script exact sequences and repeats.
void injectRawReliableData(LoopbackTransport& sender, const NetAddress& to,
                           uint8_t sourceNodeId, uint16_t sourceBootId,
                           uint32_t sequence, const uint8_t* payload,
                           uint16_t length) {
    NetPacketHeader header{};
    header.version = config::NetProtocolVersion;
    header.type = NetPacketType::Data;
    header.flags = kNetFlagReliable;
    header.sourceNodeId = sourceNodeId;
    header.sourceBootId = sourceBootId;
    header.sequence = sequence;
    header.ackSequence = 0;
    header.payloadLength = length;

    uint8_t frame[kNetTransportFrameSize]{};
    const uint16_t frameLength = encodeFrame(header, payload, frame,
                                             static_cast<uint16_t>(
                                                 sizeof(frame)));
    TEST_ASSERT_EQUAL_UINT16(
        static_cast<uint16_t>(kNetHeaderSize + length), frameLength);
    assertNetResult(NetResult::Ok, sender.send(to, frame, frameLength));
}

// Pops one frame from a transport and asserts it is an ACK for exactly
// `expectedAckSequence`. The full 32-bit sequence travels in the 4-byte
// payload; the 16-bit header field is reserved.
void assertNextAck(LoopbackTransport& transport,
                   uint32_t expectedAckSequence) {
    NetRxInfo rx{};
    TEST_ASSERT_TRUE(transport.pollRx(rx));

    NetPacketHeader header{};
    const uint8_t* payload = nullptr;
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(NetParseResult::Ok),
        static_cast<uint8_t>(decodeFrame(rx.data, rx.length, header, &payload)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetPacketType::Ack),
                            static_cast<uint8_t>(header.type));
    TEST_ASSERT_EQUAL_UINT8(0, header.flags);
    TEST_ASSERT_EQUAL_UINT32(0, header.sequence);
    TEST_ASSERT_EQUAL_UINT16(0, header.ackSequence);
    TEST_ASSERT_EQUAL_UINT16(4, header.payloadLength);
    TEST_ASSERT_NOT_NULL(payload);

    const uint32_t acknowledged = static_cast<uint32_t>(payload[0]) |
                                  (static_cast<uint32_t>(payload[1]) << 8) |
                                  (static_cast<uint32_t>(payload[2]) << 16) |
                                  (static_cast<uint32_t>(payload[3]) << 24);
    TEST_ASSERT_EQUAL_UINT32(expectedAckSequence, acknowledged);
}

}  // namespace

// =============================================================================
// 1. A reliable send is ACKed, clears its pending entry and keeps txRetries
//    at zero.
// =============================================================================

void test_net_reliability_ack_clears_pending(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    uint8_t payload[4];
    fillPayload(payload, sizeof(payload), 0x20);

    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, payload,
                           static_cast<uint16_t>(sizeof(payload))));
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

    // The ACK is a separate frame, so it is drained on A's next update.
    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
}

// =============================================================================
// 2. Reliable sequences start at 1 and increase per peer.
// =============================================================================

void test_net_reliability_sequences_start_at_one(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    uint8_t firstPayload = 0x31u;
    uint8_t secondPayload = 0x32u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &firstPayload, 1));
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &secondPayload, 1));
    TEST_ASSERT_EQUAL_UINT8(2, a.pendingCount());

    b.update(0);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_EQUAL_UINT8(0x31u, event.payload[0]);

    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(2, event.sequence);
    TEST_ASSERT_EQUAL_UINT8(0x32u, event.payload[0]);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
}

// =============================================================================
// 3. A lost ACK is retried with the same sequence; the duplicate is not
//    delivered again but is ACKed again.
// =============================================================================

void test_net_reliability_lost_ack_is_retried(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    uint8_t payload = 0x5Au;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &payload, 1));

    // The Data frame is already in B's transport queue, so dropping 100 % here
    // only kills the ACK that B is about to send back.
    LoopbackFaults dropAcks;
    dropAcks.lossPercent = 100;
    bus.setFaults(dropAcks);

    b.update(0);
    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    // Restore the link and let the ACK timeout expire.
    LoopbackFaults clean;
    bus.setFaults(clean);
    bus.setNowMs(config::NetAckTimeoutMs);
    a.update(config::NetAckTimeoutMs);

    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());

    b.update(config::NetAckTimeoutMs);
    TEST_ASSERT_EQUAL_UINT32(1, b.stats().rxDuplicates);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    a.update(config::NetAckTimeoutMs);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
}

// =============================================================================
// 4. A message that is never ACKed gives up after maxRetries retries and
//    reports SendFailed with its original sequence.
// =============================================================================

void test_net_reliability_gives_up_after_max_retries(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    // Loss is installed before the send so the initial frame is dropped too:
    // the transport still accepts it, it just never reaches B.
    LoopbackFaults dropAll;
    dropAll.lossPercent = 100;
    bus.setFaults(dropAll);

    uint8_t payload = 0x42u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &payload, 1));
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());

    uint32_t nowMs = 0;
    for (uint8_t window = 1;
         window <= static_cast<uint8_t>(config::NetMaxRetries + 1);
         ++window) {
        nowMs = static_cast<uint32_t>(window) * config::NetAckTimeoutMs;
        bus.setNowMs(nowMs);
        a.update(nowMs);
    }

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(config::NetMaxRetries),
                             a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txFailed);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::SendFailed, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_EQUAL_UINT16(0, event.length);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    b.update(nowMs);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 5. Duplicate raw Data frames are delivered once and ACKed twice.
// =============================================================================

void test_net_reliability_duplicate_is_acked_but_not_redelivered(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint8_t payload[3] = {0xA1u, 0xB2u, 0xC3u};
    injectRawReliableData(transportA, makeAddress(2), 11, 1, 5, payload,
                          static_cast<uint16_t>(sizeof(payload)));
    injectRawReliableData(transportA, makeAddress(2), 11, 1, 5, payload,
                          static_cast<uint16_t>(sizeof(payload)));

    b.update(0);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(5, event.sequence);
    TEST_ASSERT_EQUAL_UINT16(3, event.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, event.payload, 3);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    TEST_ASSERT_EQUAL_UINT32(1, b.stats().rxDuplicates);

    // Both frames earned an ACK, because either one may be the lost copy.
    assertNextAck(transportA, 5);
    assertNextAck(transportA, 5);
}

// =============================================================================
// 6. Reordered reliable frames are both delivered exactly once.
// =============================================================================

void test_net_reliability_out_of_order_is_delivered_once(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    LoopbackFaults reordered;
    reordered.reorderPercent = 100;
    bus.setFaults(reordered);

    uint8_t firstPayload = 0x61u;
    uint8_t secondPayload = 0x62u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &firstPayload, 1));
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &secondPayload, 1));

    b.update(0);

    bool sawFirst = false;
    bool sawSecond = false;
    NetEvent event{};
    while (b.pollEvent(event)) {
        assertEventType(NetEventType::Message, event);
        if (event.sequence == 1) {
            TEST_ASSERT_FALSE(sawFirst);
            sawFirst = true;
        } else if (event.sequence == 2) {
            TEST_ASSERT_FALSE(sawSecond);
            sawSecond = true;
        } else {
            TEST_FAIL_MESSAGE("Unexpected sequence in reordered delivery");
        }
    }
    TEST_ASSERT_TRUE(sawFirst);
    TEST_ASSERT_TRUE(sawSecond);
    TEST_ASSERT_EQUAL_UINT32(0, b.stats().rxDuplicates);

    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
}

// =============================================================================
// 7. The 64-bit dedupe window survives the uint32 sequence wrap.
// =============================================================================

void test_net_reliability_dedupe_window_wraps(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint32_t sequences[4] = {0xFFFFFFFEu, 0xFFFFFFFFu, 1u, 2u};
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t payload = static_cast<uint8_t>(0x70u + i);
        injectRawReliableData(transportA, makeAddress(2), 11, 1, sequences[i],
                              &payload, 1);
    }

    b.update(0);

    NetEvent event{};
    for (uint8_t i = 0; i < 4; ++i) {
        TEST_ASSERT_TRUE(b.pollEvent(event));
        assertEventType(NetEventType::Message, event);
        TEST_ASSERT_EQUAL_UINT32(sequences[i], event.sequence);
    }
    TEST_ASSERT_TRUE(hasNoEvent(b));
    TEST_ASSERT_EQUAL_UINT32(0, b.stats().rxDuplicates);

    // Drain A's own send results (and the four ACKs) before injecting again:
    // the loopback transport only buffers four unread send results.
    a.update(0);

    // Sequence 1 was already accepted before the wrap: a second copy is a
    // duplicate even though its numeric value is far below the window base.
    const uint8_t replay = 0x99u;
    injectRawReliableData(transportA, makeAddress(2), 11, 1, 1u, &replay, 1);
    bus.setNowMs(1);
    b.update(1);

    TEST_ASSERT_EQUAL_UINT32(1, b.stats().rxDuplicates);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 8. A full pending queue answers Busy without consuming a sequence.
// =============================================================================

void test_net_reliability_pending_queue_full_returns_busy(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    LoopbackFaults dropAll;
    dropAll.lossPercent = 100;
    bus.setFaults(dropAll);

    const uint16_t capacity = config::NetTxQueueCapacity;
    for (uint16_t i = 0; i < capacity; ++i) {
        const uint8_t payload = static_cast<uint8_t>(i);
        assertNetResult(NetResult::Ok,
                        a.send(0, NetDelivery::Reliable, &payload, 1));
        TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(i + 1),
                                 a.pendingCount());
        // Draining the send results keeps the transport queue, not the
        // session pending queue, from becoming the limiting factor; the
        // clock does not move, so nothing retries.
        a.update(0);
    }
    TEST_ASSERT_EQUAL_UINT16(capacity, a.pendingCount());

    const uint8_t overflow = 0xFFu;
    assertNetResult(NetResult::Busy,
                    a.send(0, NetDelivery::Reliable, &overflow, 1));
    TEST_ASSERT_EQUAL_UINT16(capacity, a.pendingCount());
}

// =============================================================================
// 9. A reliable send to a known but offline peer answers PeerNotFound.
// =============================================================================

void test_net_reliability_send_to_offline_peer_not_found(void) {
    LoopbackBus bus;
    LoopbackTransport transport(bus, makeAddress(1));
    NetSession session(transport);

    assertNetResult(NetResult::Ok, transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    session.init(makeSessionConfig(11, 1,
                                                   config::NetHelloIntervalMs,
                                                   false)));

    NetPeerConfig peer;
    peer.address = makeAddress(2);
    peer.nodeId = 22;
    assertNetResult(NetResult::Ok, session.addPeer(peer));

    uint8_t payload = 1;
    assertNetResult(NetResult::PeerNotFound,
                    session.send(0, NetDelivery::Reliable, &payload, 1));
    assertNetResult(NetResult::PeerNotFound,
                    session.send(0, NetDelivery::Reliable, nullptr, 0));

    TEST_ASSERT_EQUAL_UINT8(0, session.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(0, session.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, session.stats().txBusy);
}

// =============================================================================
// 10. A peer that reboots (new bootId from the same MAC) fails every pending
//     message for that peer.
// =============================================================================

void test_net_reliability_peer_reboot_fails_pending(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    LoopbackFaults dropAll;
    dropAll.lossPercent = 100;
    bus.setFaults(dropAll);

    uint8_t firstPayload = 0x81u;
    uint8_t secondPayload = 0x82u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &firstPayload, 1));
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &secondPayload, 1));
    TEST_ASSERT_EQUAL_UINT8(2, a.pendingCount());

    a.update(0);

    // B goes away; its BYE is swallowed by the loss profile.
    b.shutdown();
    transportB.shutdown();

    // The replacement session keeps the address but advertises a new boot, so
    // A must invalidate both its dedupe window and its in-flight messages.
    LoopbackTransport transportB2(bus, makeAddress(2));
    NetSession b2(transportB2);
    LoopbackFaults clean;
    bus.setFaults(clean);
    assertNetResult(NetResult::Ok, transportB2.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, b2.init(makeSessionConfig(22, 99)));

    bus.setNowMs(1);
    b2.update(1);
    a.update(1);

    TEST_ASSERT_EQUAL_UINT32(2, a.stats().txFailed);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::SendFailed, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_EQUAL_UINT16(0, event.length);

    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::SendFailed, event);
    TEST_ASSERT_EQUAL_UINT8(0, event.peerId);
    TEST_ASSERT_EQUAL_UINT32(2, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(a));
}

// =============================================================================
// 11. An ACK for a sequence that is not pending is ignored.
// =============================================================================

void test_net_reliability_unknown_ack_is_ignored(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    bus.setNowMs(0);
    discoverPair(a, b, 0);
    drainEvents(a);
    drainEvents(b);

    const uint32_t rxBefore = a.stats().rxMessages;
    const uint32_t txBefore = a.stats().txMessages;

    NetPacketHeader header{};
    header.version = config::NetProtocolVersion;
    header.type = NetPacketType::Ack;
    header.flags = 0;
    header.sourceNodeId = 22;
    header.sourceBootId = 2;
    header.sequence = 0;
    header.ackSequence = 0;
    header.payloadLength = 4;

    const uint32_t unknownSequence = 12345u;
    uint8_t ackPayload[4];
    ackPayload[0] = static_cast<uint8_t>(unknownSequence & 0xFFu);
    ackPayload[1] = static_cast<uint8_t>((unknownSequence >> 8) & 0xFFu);
    ackPayload[2] = static_cast<uint8_t>((unknownSequence >> 16) & 0xFFu);
    ackPayload[3] = static_cast<uint8_t>((unknownSequence >> 24) & 0xFFu);

    uint8_t frame[kNetTransportFrameSize]{};
    const uint16_t frameLength =
        encodeFrame(header, ackPayload, frame, sizeof(frame));
    TEST_ASSERT_EQUAL_UINT16(kNetHeaderSize + 4, frameLength);
    assertNetResult(NetResult::Ok,
                    transportB.send(makeAddress(1), frame, frameLength));

    a.update(0);

    TEST_ASSERT_EQUAL_UINT32(rxBefore + 1, a.stats().rxMessages);
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_TRUE(hasNoEvent(a));
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_reliability_flag_off_stub_compiles(void) {
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
    RUN_TEST(test_net_reliability_ack_clears_pending);
    RUN_TEST(test_net_reliability_sequences_start_at_one);
    RUN_TEST(test_net_reliability_lost_ack_is_retried);
    RUN_TEST(test_net_reliability_gives_up_after_max_retries);
    RUN_TEST(test_net_reliability_duplicate_is_acked_but_not_redelivered);
    RUN_TEST(test_net_reliability_out_of_order_is_delivered_once);
    RUN_TEST(test_net_reliability_dedupe_window_wraps);
    RUN_TEST(test_net_reliability_pending_queue_full_returns_busy);
    RUN_TEST(test_net_reliability_send_to_offline_peer_not_found);
    RUN_TEST(test_net_reliability_peer_reboot_fails_pending);
    RUN_TEST(test_net_reliability_unknown_ack_is_ignored);
#else
    RUN_TEST(test_net_reliability_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
