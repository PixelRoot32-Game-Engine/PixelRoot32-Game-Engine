/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for backpressure, busy-transport acceptance, configuration
 * validation and wrap-safe retry timing (networking/NetSession.h) as delivered
 * by slice N7.
 *
 * These tests pin one contract: a reliable message the session accepts is not
 * lost because the transport was momentarily out of room. It stays in the
 * pending queue with its original sequence and is transmitted on a later
 * update. They drive two sessions over the loopback bus with an explicit clock,
 * so no test sleeps or reads millis().
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
using pixelroot32::networking::NetPeerConfig;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetRxInfo;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransport;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::NetTxResult;
using pixelroot32::networking::kNetAddressSize;

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
                                       config::NetHelloIntervalMs) {
    NetSessionConfig sessionConfig;
    sessionConfig.channel = 1;
    sessionConfig.nodeId = nodeId;
    sessionConfig.bootId = bootId;
    sessionConfig.discovery = true;
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

// Minimal transport double for the configuration tests. It reports ready and
// counts setChannel calls instead of changing a real link, so a rejected init
// can be proven to have left the transport untouched: this double accepts any
// channel, which means an init that skipped validation would succeed here.
class RecordingTransport final : public NetTransport {
public:
    NetResult init(const NetTransportConfig&) override {
        return NetResult::Ok;
    }

    void shutdown() override {}

    NetResult addPeer(const NetPeerConfig&) override {
        return NetResult::Ok;
    }

    NetResult removePeer(const NetAddress&) override {
        return NetResult::Ok;
    }

    NetResult send(const NetAddress&, const uint8_t*, uint16_t) override {
        return NetResult::Ok;
    }

    bool pollRx(NetRxInfo&) override { return false; }

    bool pollTxResult(NetTxResult&) override { return false; }

    NetAddress localAddress() const override { return addressValue; }

    NetResult setChannel(uint8_t newChannel) override {
        ++setChannelCalls;
        lastChannel = newChannel;
        return NetResult::Ok;
    }

    bool isReady() const override { return true; }

    NetAddress addressValue = makeAddress(9);
    uint8_t setChannelCalls = 0;
    uint8_t lastChannel = 0;
};

}  // namespace

// =============================================================================
// 1. A reliable send accepted while the transport is busy is delivered exactly
//    once after the transport has room, with its original sequence.
// =============================================================================

void test_net_backpressure_reliable_busy_is_retried(void) {
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

    LoopbackFaults busy;
    busy.sendBusy = true;
    bus.setFaults(busy);

    uint8_t payload[4];
    fillPayload(payload, sizeof(payload), 0x20);

    const uint32_t txBefore = a.stats().txMessages;

    // The session accepts the message and owns the sequence even though the
    // transport has no room; Busy is not returned for a reliable send.
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, payload,
                           static_cast<uint16_t>(sizeof(payload))));
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);

    // Clearing the transport fault must not require a new send: the update
    // path is what transmits the queued frame.
    LoopbackFaults clean;
    bus.setFaults(clean);

    a.update(0);
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

    // The ACK is drained on A's next update and retires the pending entry.
    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
}

// =============================================================================
// 2. A full pending queue answers Busy without consuming a sequence, and the
//    queue drains in order once the transport recovers.
// =============================================================================

void test_net_backpressure_full_queue_drains_in_order(void) {
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

    LoopbackFaults busy;
    busy.sendBusy = true;
    bus.setFaults(busy);

    const uint16_t capacity = config::NetTxQueueCapacity;
    for (uint16_t i = 0; i < capacity; ++i) {
        const uint8_t payload = static_cast<uint8_t>(i);
        // Accepted even while the transport is busy: reliable sends are never
        // lost to transport contention.
        assertNetResult(NetResult::Ok,
                        a.send(0, NetDelivery::Reliable, &payload, 1));
        TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(i + 1),
                                 a.pendingCount());
    }

    // The session itself is now full: this is the only Busy a reliable send
    // may report, and it must not consume a sequence.
    const uint8_t overflow = 0xEEu;
    assertNetResult(NetResult::Busy,
                    a.send(0, NetDelivery::Reliable, &overflow, 1));
    TEST_ASSERT_EQUAL_UINT16(capacity, a.pendingCount());

    LoopbackFaults clean;
    bus.setFaults(clean);

    // Drain in rounds: each round transmits what the transport can accept,
    // the peer answers with ACKs, and the next round retires them and moves on.
    for (uint8_t round = 0; round < 6 && a.pendingCount() > 0; ++round) {
        a.update(0);
        b.update(0);
    }
    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());

    // Exactly one copy of each message, in sequence order, no duplicates.
    for (uint32_t expected = 1;
         expected <= static_cast<uint32_t>(capacity); ++expected) {
        NetEvent event{};
        TEST_ASSERT_TRUE(b.pollEvent(event));
        assertEventType(NetEventType::Message, event);
        TEST_ASSERT_EQUAL_UINT32(expected, event.sequence);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected - 1),
                                event.payload[0]);
    }
    TEST_ASSERT_TRUE(hasNoEvent(b));
    TEST_ASSERT_EQUAL_UINT32(0, b.stats().rxDuplicates);

    // The rejected ninth send consumed no sequence, so the next accepted
    // message continues at capacity + 1.
    uint8_t nextPayload = 0xAAu;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &nextPayload, 1));
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());

    b.update(0);
    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(
        static_cast<uint32_t>(capacity + 1), event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
}

// =============================================================================
// 3. A busy transport during a retry keeps the entry and does not cause a
//    second delivery.
// =============================================================================

void test_net_backpressure_busy_during_retry(void) {
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

    // The Data frame is already queued for B, so 100 % loss here only kills
    // the ACK that B is about to send back.
    LoopbackFaults dropAcks;
    dropAcks.lossPercent = 100;
    bus.setFaults(dropAcks);

    b.update(0);
    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    // The transport is now busy when the ACK timeout is due.
    LoopbackFaults busy;
    busy.sendBusy = true;
    bus.setFaults(busy);

    uint32_t nowMs = config::NetAckTimeoutMs;
    bus.setNowMs(nowMs);
    a.update(nowMs);

    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    // Back to a working link: the retry is a duplicate, so B must not deliver
    // it again but must still ACK it so A can retire the entry.
    LoopbackFaults clean;
    bus.setFaults(clean);

    nowMs += config::NetAckTimeoutMs;
    bus.setNowMs(nowMs);
    a.update(nowMs);
    b.update(nowMs);
    a.update(nowMs);

    TEST_ASSERT_EQUAL_UINT32(1, b.stats().rxDuplicates);
    TEST_ASSERT_TRUE(hasNoEvent(b));
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
}

// =============================================================================
// 4. An entry that was never transmitted does not consume the retry budget,
//    however long the transport stays busy.
// =============================================================================

void test_net_backpressure_unsent_does_not_consume_retries(void) {
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

    LoopbackFaults busy;
    busy.sendBusy = true;
    bus.setFaults(busy);

    uint8_t payload = 0x42u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &payload, 1));
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());

    // Far more than the retry budget in elapsed time. A never-sent message has
    // nothing to retry, so the budget must stay untouched and no SendFailed
    // may fire.
    uint32_t nowMs = 0;
    for (uint16_t window = 1;
         window <= static_cast<uint16_t>(config::NetMaxRetries + 2); ++window) {
        nowMs = static_cast<uint32_t>(window) * config::NetAckTimeoutMs;
        bus.setNowMs(nowMs);
        a.update(nowMs);
    }

    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txRetries);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    LoopbackFaults clean;
    bus.setFaults(clean);
    a.update(nowMs);
    b.update(nowMs);
    a.update(nowMs);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(b));
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
}

// =============================================================================
// 5. An unreliable send still reports Busy and queues nothing.
// =============================================================================

void test_net_backpressure_unreliable_busy_queues_nothing(void) {
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

    LoopbackFaults busy;
    busy.sendBusy = true;
    bus.setFaults(busy);

    const uint32_t txBefore = a.stats().txMessages;
    uint8_t payload = 0x77u;
    assertNetResult(NetResult::Busy,
                    a.send(0, NetDelivery::Unreliable, &payload, 1));

    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);

    b.update(0);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 6. init rejects an unusable configuration before it reaches the transport.
// =============================================================================

void test_net_backpressure_init_validates_config(void) {
    RecordingTransport transport;
    NetSession session(transport);

    NetSessionConfig invalid[6];
    for (uint8_t i = 0; i < 6; ++i) {
        invalid[i] = makeSessionConfig(11, 1);
    }
    invalid[0].channel = 0;
    invalid[1].channel = 14;
    invalid[2].helloIntervalMs = 0;
    invalid[3].peerTimeoutMs = 0;
    invalid[4].ackTimeoutMs = 0;
    invalid[5].maxRetries = 0;

    const uint8_t payload = 1;
    for (uint8_t i = 0; i < 6; ++i) {
        assertNetResult(NetResult::InvalidArgument, session.init(invalid[i]));

        // A rejected configuration never reached the transport, and the
        // session stayed uninitialized.
        TEST_ASSERT_EQUAL_UINT8(0, transport.setChannelCalls);
        assertNetResult(NetResult::NotInitialized,
                        session.send(0, NetDelivery::Unreliable, &payload, 1));
    }

    // A later valid configuration still works and is the first one to reach
    // the transport.
    assertNetResult(NetResult::Ok, session.init(makeSessionConfig(11, 1)));
    TEST_ASSERT_EQUAL_UINT8(1, transport.setChannelCalls);
    TEST_ASSERT_EQUAL_UINT8(1, transport.lastChannel);
}

// =============================================================================
// 7. Retry timing stays correct when the 32-bit millisecond clock wraps.
// =============================================================================

void test_net_backpressure_retry_timing_wraps(void) {
    LoopbackBus bus;
    LoopbackTransport transportA(bus, makeAddress(1));
    LoopbackTransport transportB(bus, makeAddress(2));
    NetSession a(transportA);
    NetSession b(transportB);

    assertNetResult(NetResult::Ok, transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, a.init(makeSessionConfig(11, 1)));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));

    // Discovery itself happens late in the 32-bit range so the next deadlines
    // cross the wrap instead of being reached numerically.
    const uint32_t startMs = 0xFFFFFFF0u;
    bus.setNowMs(startMs);
    discoverPair(a, b, startMs);
    drainEvents(a);
    drainEvents(b);

    uint8_t payload = 0x31u;
    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &payload, 1));
    TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());

    // Each window is +ackTimeout across the wrap. B is never updated, so no
    // ACK can retire the entry.
    uint32_t nowMs = startMs;
    for (uint8_t retry = 1; retry <= config::NetMaxRetries; ++retry) {
        nowMs += config::NetAckTimeoutMs;
        bus.setNowMs(nowMs);
        a.update(nowMs);

        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(retry),
                                 a.stats().txRetries);
        TEST_ASSERT_EQUAL_UINT8(1, a.pendingCount());
        TEST_ASSERT_EQUAL_UINT32(0, a.stats().txFailed);
    }
    TEST_ASSERT_TRUE(hasNoEvent(a));

    // One more timeout exhausts the budget exactly once.
    nowMs += config::NetAckTimeoutMs;
    bus.setNowMs(nowMs);
    a.update(nowMs);

    TEST_ASSERT_EQUAL_UINT32(1, a.stats().txFailed);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::SendFailed, event);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(a));
}

// =============================================================================
// 8. Give-up still fires once when the transport never reports busy.
// =============================================================================

void test_net_backpressure_gives_up_without_busy(void) {
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

    // Loss is installed before the send so the transport accepts every frame
    // (never Busy) but none reaches B.
    LoopbackFaults dropAll;
    dropAll.lossPercent = 100;
    bus.setFaults(dropAll);

    uint8_t payload = 0x51u;
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
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txBusy);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());

    NetEvent event{};
    TEST_ASSERT_TRUE(a.pollEvent(event));
    assertEventType(NetEventType::SendFailed, event);
    TEST_ASSERT_EQUAL_UINT32(1, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(a));

    b.update(nowMs);
    TEST_ASSERT_TRUE(hasNoEvent(b));
}

// =============================================================================
// 9. A non-Busy transport error keeps no entry but still spends the sequence.
// =============================================================================

void test_net_backpressure_transport_error_keeps_nothing(void) {
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

    // B leaves the link without telling A, so A's peer slot is still online
    // while the transport can no longer resolve its address: that is a real
    // transport error, not backpressure.
    transportB.shutdown();

    const uint32_t txBefore = a.stats().txMessages;
    uint8_t payload = 0x99u;
    assertNetResult(NetResult::PeerNotFound,
                    a.send(0, NetDelivery::Reliable, &payload, 1));

    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
    TEST_ASSERT_EQUAL_UINT32(txBefore, a.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT32(0, a.stats().txBusy);

    // Nothing may be retried later: the sequence was spent, not queued.
    a.update(0);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());

    // B rejoins, and the next reliable message proves the failed send consumed
    // sequence 1 even though it never reached the pending queue.
    assertNetResult(NetResult::Ok, transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, b.init(makeSessionConfig(22, 2)));
    discoverPair(a, b, 1);
    drainEvents(a);
    drainEvents(b);

    assertNetResult(NetResult::Ok,
                    a.send(0, NetDelivery::Reliable, &payload, 1));
    b.update(1);

    NetEvent event{};
    TEST_ASSERT_TRUE(b.pollEvent(event));
    assertEventType(NetEventType::Message, event);
    TEST_ASSERT_EQUAL_UINT32(2, event.sequence);
    TEST_ASSERT_TRUE(hasNoEvent(b));

    a.update(1);
    TEST_ASSERT_EQUAL_UINT8(0, a.pendingCount());
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_backpressure_flag_off_stub_compiles(void) {
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
    RUN_TEST(test_net_backpressure_reliable_busy_is_retried);
    RUN_TEST(test_net_backpressure_full_queue_drains_in_order);
    RUN_TEST(test_net_backpressure_busy_during_retry);
    RUN_TEST(test_net_backpressure_unsent_does_not_consume_retries);
    RUN_TEST(test_net_backpressure_unreliable_busy_queues_nothing);
    RUN_TEST(test_net_backpressure_init_validates_config);
    RUN_TEST(test_net_backpressure_retry_timing_wraps);
    RUN_TEST(test_net_backpressure_gives_up_without_busy);
    RUN_TEST(test_net_backpressure_transport_error_keeps_nothing);
#else
    RUN_TEST(test_net_backpressure_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
