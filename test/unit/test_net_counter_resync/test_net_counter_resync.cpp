/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for snapshot export/import and reboot resync
 * (networking/NetCounter.h) as delivered by slice N12.
 *
 * A node that boots after its peers must not restart the shared counter at
 * zero. On a join one side sends its whole slot table as a reliable snapshot
 * while the other buffers the operations that arrive during the wait, imports
 * the snapshot and replays the buffer. These tests drive two real sessions over
 * the loopback bus with independent session clocks, so a reboot is a fresh
 * near-zero clock opposite a survivor that keeps the large one.
 *
 * The networking module compiles out unless PIXELROOT32_ENABLE_NETWORKING is
 * 1, and the loopback backend only exists on PLATFORM_NATIVE. The functional
 * branch therefore lives behind both guards, and the flag-off branch proves the
 * inert contract under [env:native_test].
 *
 * Every timestamp is injected through the explicit clock arguments; neither
 * the wall clock nor millis() is read, so each run is reproducible.
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
using pixelroot32::networking::kCounterResyncTimeoutMs;
using pixelroot32::networking::kCounterSnapshotTag;
using pixelroot32::networking::kNetAddressSize;

// Deterministic, readable addresses: every byte differs so a partial copy
// cannot pass unnoticed. Tag 1 is node A, tag 2 is node B.
NetAddress makeAddress(uint8_t tag) {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = static_cast<uint8_t>(0x10u + tag + i);
    }
    return address;
}

NetTransportConfig makeTransportConfig() {
    NetTransportConfig transportConfig;
    transportConfig.channel = 1;
    transportConfig.nodeId = 0;
    transportConfig.manageWifi = false;
    return transportConfig;
}

NetSessionConfig makeSessionConfig(uint8_t nodeId, uint16_t bootId) {
    NetSessionConfig sessionConfig;
    sessionConfig.channel = 1;
    sessionConfig.nodeId = nodeId;
    sessionConfig.bootId = bootId;
    sessionConfig.discovery = true;
    return sessionConfig;
}

void assertNetResult(NetResult expected, NetResult actual) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),
                                    static_cast<uint8_t>(actual),
                                    "Unexpected NetResult");
}

// Pushes every queued session event through a counter. This mirrors what
// Engine::update does for a scene: the counter consumes the session queue and
// never owns it.
void feedEvents(NetSession& session, NetCounter& counter) {
    NetEvent event{};
    while (session.pollEvent(event)) {
        counter.onEvent(event);
    }
}

// Two full sessions plus their counters, wired to one loopback bus.
//
// Construction only wires references; @ref initPair performs initialization so
// a failed setup is reported by Unity instead of escaping a constructor.
struct ResyncPair {
    LoopbackBus bus;
    LoopbackTransport transportA;
    LoopbackTransport transportB;
    NetSession sessionA;
    NetSession sessionB;
    NetCounter counterA;
    NetCounter counterB;

    ResyncPair()
        : transportA(bus, makeAddress(1)), transportB(bus, makeAddress(2)),
          sessionA(transportA), sessionB(transportB) {}
};

void initPair(ResyncPair& pair, uint8_t nodeIdA, uint16_t bootIdA,
              uint8_t nodeIdB, uint16_t bootIdB) {
    assertNetResult(NetResult::Ok, pair.transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, pair.transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    pair.sessionA.init(makeSessionConfig(nodeIdA, bootIdA)));
    assertNetResult(NetResult::Ok,
                    pair.sessionB.init(makeSessionConfig(nodeIdB, bootIdB)));
    assertNetResult(NetResult::Ok, pair.counterA.init(pair.sessionA));
    assertNetResult(NetResult::Ok, pair.counterB.init(pair.sessionB));
}

// One retimed exchange. Both counters tick first because a PeerJoined handler
// compares the peer's advertised uptime against the local lastNowMs.
void pumpPair(ResyncPair& pair, uint32_t nowA, uint32_t nowB) {
    pair.counterA.update(nowA);
    pair.counterB.update(nowB);
    pair.bus.setNowMs(nowA > nowB ? nowA : nowB);
    pair.sessionA.update(nowA);
    pair.sessionB.update(nowB);
    feedEvents(pair.sessionA, pair.counterA);
    feedEvents(pair.sessionB, pair.counterB);
}

// Repeated exchanges at one timestamp. Four pumps carry data, ACKs and a
// snapshot; the extra rounds are headroom so a test never depends on the exact
// order a snapshot and its ACK are queued in.
void settlePair(ResyncPair& pair, uint32_t nowA, uint32_t nowB,
                uint8_t rounds = 8) {
    for (uint8_t i = 0; i < rounds; ++i) {
        pumpPair(pair, nowA, nowB);
    }
}

// Reboots one node with a graceful farewell: the survivor sees PeerLeft and
// will emit PeerJoined when the peer returns, which is the resync trigger.
//
// The survivor's clock is advanced only after the rebooted transport is
// detached, so a large survivor uptime cannot expire a live peer.
void gracefulReboot(NetSession& rebootedSession,
                    LoopbackTransport& rebootedTransport,
                    NetCounter& rebootedCounter, uint8_t rebootedNodeId,
                    uint16_t newBootId, NetSession& survivorSession,
                    NetCounter& survivorCounter, LoopbackBus& bus,
                    uint32_t survivorNow) {
    rebootedSession.shutdown();
    rebootedTransport.shutdown();

    survivorCounter.update(survivorNow);
    bus.setNowMs(survivorNow);
    survivorSession.update(survivorNow);
    feedEvents(survivorSession, survivorCounter);

    rebootedTransport.init(makeTransportConfig());
    rebootedSession.init(makeSessionConfig(rebootedNodeId, newBootId));
    rebootedCounter.init(rebootedSession);
}

// Reboots one node like a power cut: the transport is detached before the
// session shuts down, so the BYE never reaches the survivor. The survivor keeps
// the peer online and therefore runs no join path of its own; the rebooted node
// must ask for a snapshot instead of waiting for a proactive one.
void hardReboot(NetSession& rebootedSession,
                LoopbackTransport& rebootedTransport,
                NetCounter& rebootedCounter, uint8_t rebootedNodeId,
                uint16_t newBootId) {
    rebootedTransport.shutdown();
    rebootedSession.shutdown();
    rebootedTransport.init(makeTransportConfig());
    rebootedSession.init(makeSessionConfig(rebootedNodeId, newBootId));
    rebootedCounter.init(rebootedSession);
}

// Builds a synthetic Message event with an exact payload, as the session would
// deliver it. Used to inject malformed snapshots that a real peer would never
// send but a corrupted link could.
NetEvent makeMessageEvent(const uint8_t* bytes, uint16_t length) {
    NetEvent event{};
    event.type = NetEventType::Message;
    event.peerId = 0;
    event.nodeId = 11;
    event.length = length;
    for (uint16_t i = 0; i < length; ++i) {
        event.payload[i] = bytes[i];
    }
    return event;
}

}  // namespace

// =============================================================================
// 1. Survivor A -> rebooted B. A already holds 5; B boots fresh at 0 with a
//    new boot id and a near-zero clock, so A sends the snapshot and B restores
//    the shared value.
// =============================================================================

void test_net_counter_resync_survivor_sends_to_rebooted_peer(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(5));
    settlePair(pair, 0, 0);
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());

    gracefulReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3,
                   pair.sessionA, pair.counterA, pair.bus, 20000);

    // The fresh boot starts at zero: this is the failure the snapshot fixes.
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_FALSE(pair.counterB.isResyncing());

    pumpPair(pair, 21000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());

    settlePair(pair, 21000, 0);

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isOutOfSync());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());
}

// =============================================================================
// 2. The same exchange in reverse: B was the survivor and A is the node that
//    reboots. The direction of the snapshot must follow uptime, not node id.
// =============================================================================

void test_net_counter_resync_rebooted_peer_sends_live_value(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterB.add(5));
    settlePair(pair, 0, 0);
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());

    gracefulReboot(pair.sessionA, pair.transportA, pair.counterA, 11, 3,
                   pair.sessionB, pair.counterB, pair.bus, 20000);

    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());

    settlePair(pair, 0, 21000);

    TEST_ASSERT_FALSE(pair.counterA.isResyncing());
    TEST_ASSERT_FALSE(pair.counterA.isOutOfSync());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());
}

// =============================================================================
// 3. Simultaneous boot: equal uptimes put the decision in the tie window, where
//    only the lower node id sends. Both stay at zero and no resync loop starts.
// =============================================================================

void test_net_counter_resync_simultaneous_boot_lower_id_sends(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0, 12);

    TEST_ASSERT_FALSE(pair.counterA.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_FALSE(pair.counterA.isOutOfSync());
    TEST_ASSERT_FALSE(pair.counterB.isOutOfSync());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());

    // A still further batch at the same tie timestamp must not reopen resync.
    settlePair(pair, 0, 0, 8);
    TEST_ASSERT_FALSE(pair.counterA.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
}

// =============================================================================
// 4. add() while resyncing is Busy and leaves value and revision untouched;
//    after the snapshot lands the same call succeeds.
// =============================================================================

void test_net_counter_resync_add_busy_and_recovers(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(5));
    settlePair(pair, 0, 0);

    gracefulReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3,
                   pair.sessionA, pair.counterA, pair.bus, 20000);

    // B enters resync on the join; A joins on the next pump and queues the
    // snapshot, so B is still waiting when add() is attempted.
    pumpPair(pair, 21000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());
    pumpPair(pair, 21000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());

    const int32_t valueBefore = pair.counterB.value();
    const uint32_t revisionBefore = pair.counterB.revision();
    assertNetResult(NetResult::Busy, pair.counterB.add(1));
    TEST_ASSERT_EQUAL_INT32(valueBefore, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(revisionBefore, pair.counterB.revision());

    settlePair(pair, 21000, 0);

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());

    assertNetResult(NetResult::Ok, pair.counterB.add(1));
    TEST_ASSERT_EQUAL_INT32(6, pair.counterB.value());
}

// =============================================================================
// 5. Snapshot loss: the first reliable snapshot is dropped, and the session's
//    pending retry delivers it once the link recovers.
// =============================================================================

void test_net_counter_resync_lost_snapshot_is_retried(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(5));
    settlePair(pair, 0, 0);

    gracefulReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3,
                   pair.sessionA, pair.counterA, pair.bus, 20000);

    // B joins and waits; its HELLO to A is already queued before the loss
    // profile goes in, so only A's outgoing snapshot is dropped.
    pumpPair(pair, 21000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());

    LoopbackFaults lossy;
    lossy.lossPercent = 100;
    lossy.seed = 7;
    pair.bus.setFaults(lossy);

    pair.counterA.update(22000);
    pair.bus.setNowMs(22000);
    pair.sessionA.update(22000);
    feedEvents(pair.sessionA, pair.counterA);

    TEST_ASSERT_TRUE(pair.counterB.isResyncing());
    // The proactive snapshot and the explicit request can both be in flight;
    // the retry contract only requires at least one reliable snapshot pending.
    TEST_ASSERT_TRUE(pair.sessionA.pendingCount() >= 1);

    LoopbackFaults clean;
    pair.bus.setFaults(clean);

    // The ACK timeout elapses at the survivor, which retransmits the same
    // snapshot sequence and B finally imports it.
    pumpPair(pair, 22020, 20);
    settlePair(pair, 22020, 20);

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT8(0, pair.sessionA.pendingCount());
}

// =============================================================================
// 6. An operation that arrives while B is resyncing is buffered, not applied.
//    The reordered link delivers it before the snapshot, so the import happens
//    last and the replay adds it exactly once.
// =============================================================================

void test_net_counter_resync_buffered_op_replays_once(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(5));
    settlePair(pair, 0, 0);

    gracefulReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3,
                   pair.sessionA, pair.counterA, pair.bus, 20000);
    pumpPair(pair, 21000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());

    // Let A learn the join without emitting another frame, then queue the
    // snapshot and the +1 op back to back under a reordering link. The op
    // overtakes the snapshot at B's transport.
    pair.counterA.update(21000);
    pair.bus.setNowMs(21000);
    pair.sessionA.update(21000);

    LoopbackFaults reordered;
    reordered.reorderPercent = 100;
    reordered.seed = 11;
    pair.bus.setFaults(reordered);

    feedEvents(pair.sessionA, pair.counterA);
    assertNetResult(NetResult::Ok, pair.counterA.add(1));

    LoopbackFaults clean;
    pair.bus.setFaults(clean);

    pair.counterB.update(0);
    pair.bus.setNowMs(21000);
    pair.sessionB.update(0);
    feedEvents(pair.sessionB, pair.counterB);

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_EQUAL_INT32(6, pair.counterB.value());

    settlePair(pair, 21000, 0);

    TEST_ASSERT_EQUAL_INT32(6, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(6, pair.counterB.value());
}

// =============================================================================
// 7. No snapshot ever arrives: resync times out, the flag is raised so the game
//    can surface it, and add() works again on the possibly divergent value.
// =============================================================================

void test_net_counter_resync_timeout_reports_out_of_sync(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    // A power-cut reboot leaves the survivor believing the peer is still
    // online, so no PeerJoined event asks it to send a snapshot. Its counter is
    // shut down here so the explicit snapshot request also goes unanswered:
    // this is the case where the wait can only end in a timeout.
    hardReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3);
    pair.counterA.shutdown();

    pumpPair(pair, 1000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isOutOfSync());

    // lastNowMs is 0 at B, so the 3 s deadline is 3000; 3001 must expire it.
    pumpPair(pair, 1000, static_cast<uint32_t>(kCounterResyncTimeoutMs + 1));

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_TRUE(pair.counterB.isOutOfSync());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());

    assertNetResult(NetResult::Ok, pair.counterB.add(1));
    TEST_ASSERT_EQUAL_INT32(1, pair.counterB.value());
}

// =============================================================================
// 7b. A power-cut reboot where the survivor is alive: the rebooted node asks
//     for a snapshot explicitly, so the exchange converges even though the
//     survivor never emitted PeerJoined.
// =============================================================================

void test_net_counter_resync_request_recovers_without_peer_left(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterA.add(5));
    settlePair(pair, 0, 0);
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());

    hardReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3);
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());

    settlePair(pair, 20000, 0, 12);

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isOutOfSync());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());
}

// =============================================================================
// 8. A peer reboot resets the cursor but keeps the accumulated value. B sends
//    three operations (session sequences 1..3), reboots, then sends a new
//    operation whose fresh sequence 1 could look older than cursor 3.
// =============================================================================

void test_net_counter_resync_boot_id_change_resets_cursor(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    assertNetResult(NetResult::Ok, pair.counterB.add(1));
    assertNetResult(NetResult::Ok, pair.counterB.add(1));
    assertNetResult(NetResult::Ok, pair.counterB.add(1));
    settlePair(pair, 0, 0);
    TEST_ASSERT_EQUAL_INT32(3, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(3, pair.counterB.value());

    gracefulReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3,
                   pair.sessionA, pair.counterA, pair.bus, 20000);
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());

    // The snapshot restores B's own historical contribution of 3.
    settlePair(pair, 21000, 0);
    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_EQUAL_INT32(3, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(3, pair.counterB.value());

    // Without the cursor reset this new sequence 1 would be rejected as older
    // than the pre-reboot cursor 3 and the value would stay at 3 on A.
    assertNetResult(NetResult::Ok, pair.counterB.add(2));
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());

    settlePair(pair, 21000, 0);
    TEST_ASSERT_EQUAL_INT32(5, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(5, pair.counterB.value());
}

// =============================================================================
// 9. Malformed snapshots are ignored and leave the node resyncing: wrong
//    length, wrong tag, zero slots and a slot count that disagrees with the
//    payload length.
// =============================================================================

void test_net_counter_resync_malformed_snapshot_ignored(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    hardReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3);
    pumpPair(pair, 1000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());

    const int32_t valueBefore = pair.counterB.value();
    const uint32_t revisionBefore = pair.counterB.revision();

    // Tag and one slot declared, but one entry byte short.
    uint8_t wrongLength[12] = {kCounterSnapshotTag, 1, 0x16, 3, 0,
                               0,                  0, 0, 0, 0,
                               0,                  0};
    pair.counterB.onEvent(makeMessageEvent(wrongLength, sizeof(wrongLength)));

    // Correct snapshot length, foreign tag.
    uint8_t wrongTag[13] = {kCounterOpTag, 1, 0x16, 3, 0, 0, 0,
                            0,            0, 0,    0, 0, 0};
    pair.counterB.onEvent(makeMessageEvent(wrongTag, sizeof(wrongTag)));

    // A snapshot with no slots is not a usable table.
    uint8_t zeroSlots[2] = {kCounterSnapshotTag, 0};
    pair.counterB.onEvent(makeMessageEvent(zeroSlots, sizeof(zeroSlots)));

    // Two slots declared but only one entry's worth of bytes.
    uint8_t shortCount[13] = {kCounterSnapshotTag, 2, 0x16, 3, 0, 0, 0,
                              0,                  0, 0,    0, 0, 0};
    pair.counterB.onEvent(makeMessageEvent(shortCount, sizeof(shortCount)));

    TEST_ASSERT_TRUE(pair.counterB.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isOutOfSync());
    TEST_ASSERT_EQUAL_INT32(valueBefore, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(revisionBefore, pair.counterB.revision());
}

// =============================================================================
// 10. The resyncing peer leaves: the wait is cancelled without declaring the
//     node out of sync, because the next join starts a fresh attempt.
// =============================================================================

void test_net_counter_resync_peer_left_cancels(void) {
    ResyncPair pair;
    initPair(pair, 11, 1, 22, 2);
    settlePair(pair, 0, 0);

    hardReboot(pair.sessionB, pair.transportB, pair.counterB, 22, 3);
    pumpPair(pair, 1000, 0);
    TEST_ASSERT_TRUE(pair.counterB.isResyncing());

    // A graceful shutdown sends the BYE that B's session turns into PeerLeft.
    pair.sessionA.shutdown();

    pair.counterB.update(0);
    pair.bus.setNowMs(1000);
    pair.sessionB.update(0);
    feedEvents(pair.sessionB, pair.counterB);

    TEST_ASSERT_FALSE(pair.counterB.isResyncing());
    TEST_ASSERT_FALSE(pair.counterB.isOutOfSync());

    assertNetResult(NetResult::Ok, pair.counterB.add(1));
    TEST_ASSERT_EQUAL_INT32(1, pair.counterB.value());
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_counter_resync_flag_off_stub_compiles(void) {
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
    RUN_TEST(test_net_counter_resync_survivor_sends_to_rebooted_peer);
    RUN_TEST(test_net_counter_resync_rebooted_peer_sends_live_value);
    RUN_TEST(test_net_counter_resync_simultaneous_boot_lower_id_sends);
    RUN_TEST(test_net_counter_resync_add_busy_and_recovers);
    RUN_TEST(test_net_counter_resync_lost_snapshot_is_retried);
    RUN_TEST(test_net_counter_resync_buffered_op_replays_once);
    RUN_TEST(test_net_counter_resync_timeout_reports_out_of_sync);
    RUN_TEST(test_net_counter_resync_request_recovers_without_peer_left);
    RUN_TEST(test_net_counter_resync_boot_id_change_resets_cursor);
    RUN_TEST(test_net_counter_resync_malformed_snapshot_ignored);
    RUN_TEST(test_net_counter_resync_peer_left_cancels);
#else
    RUN_TEST(test_net_counter_resync_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
