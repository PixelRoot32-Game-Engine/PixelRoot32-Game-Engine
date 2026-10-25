/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the convergent counter (networking/NetCounter.h) as delivered
 * by slice N11.
 *
 * A NetCounter replicates one shared signed value: every node owns a slot and
 * broadcasts idempotent add operations, so the sum of the slots is the shared
 * value. These tests drive two sessions over the loopback bus with an explicit
 * clock and assert convergence under lost ACKs, reordering and duplicates.
 *
 * The whole networking module is compiled out unless
 * PIXELROOT32_ENABLE_NETWORKING is 1, and the loopback backend only exists on
 * PLATFORM_NATIVE. This file therefore follows the discovery/reliability
 * pattern: the functional branch lives behind both guards, and the flag-off
 * branch runs one stub so the suite still proves the inert flags-off contract
 * under [env:native_test].
 *
 * Every session is given an explicit bootId, and every timestamp is injected
 * through update(nowMs), so no test depends on the wall clock or the platform
 * boot-id generator.
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "drivers/native/LoopbackTransport.h"
#include "networking/NetCounter.h"
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
using pixelroot32::networking::NetCounter;
using pixelroot32::networking::NetEvent;
using pixelroot32::networking::NetEventType;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::kCounterOpPayloadSize;
using pixelroot32::networking::kCounterOpTag;
using pixelroot32::networking::kNetAddressSize;

// Deterministic, readable addresses: the bytes differ in every position so a
// partial copy cannot pass unnoticed. Tag 1 is the first node, tag 2 the second.
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

// Brings a pair of sessions up on one bus, on the same channel, and runs the
// four updates needed for both one-shot HELLOs to be drained by both sides.
void discoverPair(NetSession& first, NetSession& second, uint32_t nowMs) {
    first.update(nowMs);
    second.update(nowMs);
    first.update(nowMs);
    second.update(nowMs);
}

// Pushes every queued session event through a counter. This is exactly what
// Engine::update does for a scene: the counter is a consumer of the session
// queue, never its owner.
void feedEvents(NetSession& session, NetCounter& counter) {
    NetEvent event{};
    while (session.pollEvent(event)) {
        counter.onEvent(event);
    }
}

// Two full sessions plus their counters, wired to one loopback bus.
//
// Construction only wires references; @ref start performs the initialization so
// failures are reported by Unity instead of escaping a constructor.
struct CounterPair {
    LoopbackBus bus;
    LoopbackTransport transportA;
    LoopbackTransport transportB;
    NetSession sessionA;
    NetSession sessionB;
    NetCounter counterA;
    NetCounter counterB;

    CounterPair()
        : transportA(bus, makeAddress(1)), transportB(bus, makeAddress(2)),
          sessionA(transportA), sessionB(transportB) {}
};

// Initializes both transports, both sessions and both counters, then lets
// discovery finish so the peer table is online before any counter operation.
void startPair(CounterPair& pair, uint8_t nodeIdA, uint16_t bootIdA,
               uint8_t nodeIdB, uint16_t bootIdB, uint32_t nowMs) {
    assertNetResult(NetResult::Ok, pair.transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, pair.transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    pair.sessionA.init(makeSessionConfig(nodeIdA, bootIdA)));
    assertNetResult(NetResult::Ok,
                    pair.sessionB.init(makeSessionConfig(nodeIdB, bootIdB)));

    pair.bus.setNowMs(nowMs);
    discoverPair(pair.sessionA, pair.sessionB, nowMs);

    // Discovery emits PeerJoined, which the counter deliberately ignores;
    // draining it here keeps the per-test assertions focused on counter state.
    feedEvents(pair.sessionA, pair.counterA);
    feedEvents(pair.sessionB, pair.counterB);

    assertNetResult(NetResult::Ok, pair.counterA.init(pair.sessionA));
    assertNetResult(NetResult::Ok, pair.counterB.init(pair.sessionB));
}

// One counter exchange: both sessions drain their transport, both counters
// drain their session. A second call at the same timestamp carries the ACKs
// back, so no retry ever fires while a test is only waiting for a reply.
void pump(CounterPair& pair, uint32_t nowMs) {
    pair.bus.setNowMs(nowMs);
    pair.sessionA.update(nowMs);
    pair.sessionB.update(nowMs);
    feedEvents(pair.sessionA, pair.counterA);
    feedEvents(pair.sessionB, pair.counterB);
}

// Runs enough exchanges at one fixed timestamp for data and ACKs to settle.
void settle(CounterPair& pair, uint32_t nowMs) {
    for (uint8_t i = 0; i < 4; ++i) {
        pump(pair, nowMs);
    }
}

// Builds one counter operation payload: tag byte, then the delta as a
// little-endian signed 32-bit value. The wire carries 32 bits even though the
// public API takes int16_t, so the delta is sign-extended rather than truncated.
void fillOpPayload(uint8_t* payload, int16_t delta) {
    const int32_t wide = static_cast<int32_t>(delta);
    payload[0] = kCounterOpTag;
    payload[1] = static_cast<uint8_t>(static_cast<uint32_t>(wide) & 0xFFu);
    payload[2] = static_cast<uint8_t>((static_cast<uint32_t>(wide) >> 8) & 0xFFu);
    payload[3] = static_cast<uint8_t>((static_cast<uint32_t>(wide) >> 16) & 0xFFu);
    payload[4] = static_cast<uint8_t>((static_cast<uint32_t>(wide) >> 24) & 0xFFu);
}

// Builds a synthetic Message event as the session would deliver it.
NetEvent makeOpEvent(uint8_t peerId, uint8_t nodeId, int16_t delta) {
    NetEvent event{};
    event.type = NetEventType::Message;
    event.peerId = peerId;
    event.nodeId = nodeId;
    event.sequence = 0;
    event.length = kCounterOpPayloadSize;
    fillOpPayload(event.payload, delta);
    return event;
}

// A counter alone on a session with discovery off: no online peers, so add()
// never sends and the test observes local slot behaviour only.
struct LoneCounter {
    LoopbackBus bus;
    LoopbackTransport transport;
    NetSession session;
    NetCounter counter;

    LoneCounter()
        : transport(bus, makeAddress(1)), session(transport) {}
};

void startLoneCounter(LoneCounter& lone) {
    assertNetResult(NetResult::Ok,
                    lone.transport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    lone.session.init(makeSessionConfig(11, 1, false)));
    assertNetResult(NetResult::Ok, lone.counter.init(lone.session));
}

}  // namespace

// =============================================================================
// 1. One local +1 reaches the peer: both counters read 1 and both revisions
//    advanced exactly once.
// =============================================================================

void test_net_counter_local_add_reaches_peer(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(1));

    // The local change is immediate: a game reads its own input this frame.
    TEST_ASSERT_EQUAL_INT32(1, pair.counterA.value());
    TEST_ASSERT_EQUAL_UINT32(1, pair.counterA.revision());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());

    settle(pair, 0);

    TEST_ASSERT_EQUAL_INT32(1, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(1, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(1, pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(1, pair.counterB.revision());
    TEST_ASSERT_EQUAL_UINT8(0, pair.sessionA.pendingCount());
}

// =============================================================================
// 2. The peer's -1 brings both counters back to 0.
// =============================================================================

void test_net_counter_peer_delta_returns_to_zero(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(1));
    settle(pair, 0);
    TEST_ASSERT_EQUAL_INT32(1, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(1, pair.counterB.value());

    assertNetResult(NetResult::Ok, pair.counterB.add(-1));
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());

    settle(pair, 0);

    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterB.revision());
}

// =============================================================================
// 3. Concurrent opposite deltas cancel: addition is commutative, so neither
//    side's ordering decision can leave the pair disagreeing.
// =============================================================================

void test_net_counter_concurrent_opposite_deltas_converge(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(1));
    assertNetResult(NetResult::Ok, pair.counterB.add(-1));

    TEST_ASSERT_EQUAL_INT32(1, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(-1, pair.counterB.value());

    settle(pair, 0);

    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterB.revision());
}

// =============================================================================
// 4. A mixed sequence of deltas on both sides converges to the arithmetic sum.
// =============================================================================

void test_net_counter_mixed_sequence_converges_to_sum(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(3));
    assertNetResult(NetResult::Ok, pair.counterA.add(-1));
    assertNetResult(NetResult::Ok, pair.counterB.add(2));
    assertNetResult(NetResult::Ok, pair.counterB.add(5));

    settle(pair, 0);

    // 3 - 1 + 2 + 5 = 9, independent of which op landed first on either side.
    TEST_ASSERT_EQUAL_INT32(9, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(9, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(4, pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(4, pair.counterB.revision());
}

// =============================================================================
// 5. A dropped ACK is retried with the same sequence: the peer applies the op
//    exactly once because the session's dedupe window suppresses the retry.
// =============================================================================

void test_net_counter_dropped_ack_applies_once(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    // The Data frame is queued in B's transport before the loss profile is
    // installed, so 100 % loss here only kills the ACK that B is about to send.
    assertNetResult(NetResult::Ok, pair.counterA.add(1));

    LoopbackFaults dropAcks;
    dropAcks.lossPercent = 100;
    pair.bus.setFaults(dropAcks);
    pair.bus.setNowMs(0);

    pair.sessionB.update(0);
    feedEvents(pair.sessionB, pair.counterB);
    TEST_ASSERT_EQUAL_UINT32(1, pair.counterB.revision());
    TEST_ASSERT_EQUAL_INT32(1, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT8(1, pair.sessionA.pendingCount());

    // Restore the link and let the ACK timeout expire: A retransmits the same
    // sequence, so B re-ACKs without re-delivering.
    LoopbackFaults clean;
    pair.bus.setFaults(clean);
    const uint32_t retryAt = config::NetAckTimeoutMs;
    pair.bus.setNowMs(retryAt);
    pair.sessionA.update(retryAt);
    TEST_ASSERT_EQUAL_UINT32(1, pair.sessionA.stats().txRetries);

    pair.sessionB.update(retryAt);
    TEST_ASSERT_EQUAL_UINT32(1, pair.sessionB.stats().rxDuplicates);
    feedEvents(pair.sessionB, pair.counterB);

    pair.sessionA.update(retryAt);
    TEST_ASSERT_EQUAL_UINT8(0, pair.sessionA.pendingCount());

    // Exactly one application of the op on both sides.
    TEST_ASSERT_EQUAL_UINT32(1, pair.counterB.revision());
    TEST_ASSERT_EQUAL_INT32(1, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(1, pair.counterB.value());
}

// =============================================================================
// 6. Two operations delivered in reverse order are both applied: the counter
//    never rejects an older sequence, because it relies on session dedupe and
//    addition is order independent.
// =============================================================================

void test_net_counter_out_of_order_operations_converge(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    LoopbackFaults reordered;
    reordered.reorderPercent = 100;
    pair.bus.setFaults(reordered);

    assertNetResult(NetResult::Ok, pair.counterA.add(1));
    assertNetResult(NetResult::Ok, pair.counterA.add(2));

    settle(pair, 0);

    TEST_ASSERT_EQUAL_UINT32(0, pair.sessionB.stats().rxDuplicates);
    TEST_ASSERT_EQUAL_INT32(3, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(3, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterB.revision());
}

// =============================================================================
// 7. Contract boundary: a duplicate event handed straight to onEvent is
//    applied twice. The session, not the counter, owns duplicate suppression;
//    this test pins that split so a future change cannot silently move it.
// =============================================================================

void test_net_counter_duplicate_event_applies_twice(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    const NetEvent event = makeOpEvent(0, 22, 4);
    pair.counterA.onEvent(event);
    pair.counterA.onEvent(event);

    TEST_ASSERT_EQUAL_INT32(8, pair.counterA.value());
    TEST_ASSERT_EQUAL_UINT32(2, pair.counterA.revision());
}

// =============================================================================
// 8. add() before init answers NotInitialized; add(0) answers Ok without a
//    revision bump or a send.
// =============================================================================

void test_net_counter_add_requires_init_and_ignores_zero(void) {
    CounterPair pair;
    startPair(pair, 11, 1, 22, 2, 0);

    const uint32_t txBefore = pair.sessionA.stats().txMessages;

    NetCounter neverInitialized;
    assertNetResult(NetResult::NotInitialized, neverInitialized.add(1));
    TEST_ASSERT_EQUAL_INT32(0, neverInitialized.value());
    TEST_ASSERT_EQUAL_UINT32(0, neverInitialized.revision());
    TEST_ASSERT_EQUAL_UINT32(txBefore, pair.sessionA.stats().txMessages);

    assertNetResult(NetResult::Ok, pair.counterA.add(0));
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_UINT32(0, pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(txBefore, pair.sessionA.stats().txMessages);
    TEST_ASSERT_EQUAL_UINT8(0, pair.sessionA.pendingCount());

    settle(pair, 0);
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(0, pair.counterB.revision());
}

// =============================================================================
// 9. Saturation is defined at both int32 edges. net::add saturates the slot, so
//    an unbounded game counter can never wrap into a wrong sign.
// =============================================================================

void test_net_counter_saturates_at_int32_edges(void) {
    {
        LoneCounter lone;
        startLoneCounter(lone);

        bool allOk = true;
        for (int32_t i = 0; i < 70000; ++i) {
            if (lone.counter.add(32767) != NetResult::Ok) {
                allOk = false;
                break;
            }
        }
        TEST_ASSERT_TRUE(allOk);
        TEST_ASSERT_EQUAL_INT32(INT32_MAX, lone.counter.value());

        // A delta that cannot move a saturated slot is not an applied change.
        const uint32_t saturatedRevision = lone.counter.revision();
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetResult::Ok),
                                static_cast<uint8_t>(lone.counter.add(1)));
        TEST_ASSERT_EQUAL_INT32(INT32_MAX, lone.counter.value());
        TEST_ASSERT_EQUAL_UINT32(saturatedRevision, lone.counter.revision());
    }

    {
        LoneCounter lone;
        startLoneCounter(lone);

        bool allOk = true;
        for (int32_t i = 0; i < 70000; ++i) {
            if (lone.counter.add(-32767) != NetResult::Ok) {
                allOk = false;
                break;
            }
        }
        TEST_ASSERT_TRUE(allOk);
        TEST_ASSERT_EQUAL_INT32(INT32_MIN, lone.counter.value());

        const uint32_t saturatedRevision = lone.counter.revision();
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NetResult::Ok),
                                static_cast<uint8_t>(lone.counter.add(-1)));
        TEST_ASSERT_EQUAL_INT32(INT32_MIN, lone.counter.value());
        TEST_ASSERT_EQUAL_UINT32(saturatedRevision, lone.counter.revision());
    }
}

// =============================================================================
// 10. The slot table is fixed at NetMaxPeers and the local slot consumes one
//     entry, so only NetMaxPeers - 1 remote node ids are tracked. The next
//     distinct node id is ignored instead of evicting a live slot.
// =============================================================================

void test_net_counter_slot_table_limit_ignores_extra_nodes(void) {
    LoneCounter lone;
    startLoneCounter(lone);

    const uint8_t remoteCapacity =
        static_cast<uint8_t>(NetCounter::kMaxSlots - 1);
    for (uint8_t i = 0; i < remoteCapacity; ++i) {
        lone.counter.onEvent(
            makeOpEvent(0, static_cast<uint8_t>(100 + i), 1));
    }

    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(remoteCapacity),
                            lone.counter.value());
    TEST_ASSERT_EQUAL_UINT32(remoteCapacity, lone.counter.revision());

    // One more distinct node id finds no free slot and must be dropped whole.
    lone.counter.onEvent(makeOpEvent(0, 200, 1));
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(remoteCapacity),
                            lone.counter.value());
    TEST_ASSERT_EQUAL_UINT32(remoteCapacity, lone.counter.revision());
}

// =============================================================================
// 11. Events the counter cannot interpret leave value and revision untouched:
//     wrong length, wrong tag, a non-Message event, and an operation carrying
//     the local node id (which would double-count into the local slot).
// =============================================================================

void test_net_counter_ignores_uninterpretable_events(void) {
    LoneCounter lone;
    startLoneCounter(lone);

    NetEvent wrongLength = makeOpEvent(0, 100, 7);
    wrongLength.length = static_cast<uint16_t>(kCounterOpPayloadSize - 1);
    lone.counter.onEvent(wrongLength);

    NetEvent wrongTag = makeOpEvent(0, 101, 7);
    wrongTag.payload[0] = static_cast<uint8_t>(kCounterOpTag + 1);
    lone.counter.onEvent(wrongTag);

    NetEvent peerJoined = makeOpEvent(0, 102, 7);
    peerJoined.type = NetEventType::PeerJoined;
    lone.counter.onEvent(peerJoined);

    NetEvent localEcho = makeOpEvent(0, lone.session.localNodeId(), 7);
    lone.counter.onEvent(localEcho);

    TEST_ASSERT_EQUAL_INT32(0, lone.counter.value());
    TEST_ASSERT_EQUAL_UINT32(0, lone.counter.revision());
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_counter_flag_off_stub_compiles(void) {
    // With the flag off (or on a target without the native loopback), neither
    // NetCounter nor NetSession is compiled. This translation unit compiling
    // and passing without referencing any networking symbol is the zero-cost
    // flags-off property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_NETWORKING=0: no counter or session symbols are "
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
    RUN_TEST(test_net_counter_local_add_reaches_peer);
    RUN_TEST(test_net_counter_peer_delta_returns_to_zero);
    RUN_TEST(test_net_counter_concurrent_opposite_deltas_converge);
    RUN_TEST(test_net_counter_mixed_sequence_converges_to_sum);
    RUN_TEST(test_net_counter_dropped_ack_applies_once);
    RUN_TEST(test_net_counter_out_of_order_operations_converge);
    RUN_TEST(test_net_counter_duplicate_event_applies_twice);
    RUN_TEST(test_net_counter_add_requires_init_and_ignores_zero);
    RUN_TEST(test_net_counter_saturates_at_int32_edges);
    RUN_TEST(test_net_counter_slot_table_limit_ignores_extra_nodes);
    RUN_TEST(test_net_counter_ignores_uninterpretable_events);
#else
    RUN_TEST(test_net_counter_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
