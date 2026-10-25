/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the Dice Duel round state machine
 * (examples/esp_now_dice/src/DiceDuel.h) as delivered by slice D2.
 *
 * The duel is a platform-independent state machine layered on NetSession,
 * NetCounter and DiceProtocol. These tests drive two real sessions over the
 * loopback bus with two independent counters and duels, so every rule that
 * involves the wire (commit/reveal/reset exchange, reliable delivery, score
 * replication) is exercised end to end instead of through mocks.
 *
 * The networking module compiles out unless PIXELROOT32_ENABLE_NETWORKING is
 * 1, and the loopback backend only exists on PLATFORM_NATIVE. The functional
 * branch therefore lives behind both guards, and the flag-off branch proves the
 * inert contract under [env:native_test].
 *
 * Every timestamp is injected through the explicit clock arguments; neither the
 * wall clock nor millis() is read, so each run is reproducible.
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#include "../../../examples/esp_now_dice/src/DiceDuel.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "drivers/native/LoopbackTransport.h"
#include "networking/NetCounter.h"
#include "networking/NetSession.h"
#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace {

using esp_now_dice::DiceDuel;
using esp_now_dice::DicePair;
using esp_now_dice::DuelPhase;
using esp_now_dice::computeDice;
using esp_now_dice::makeCommit;
using esp_now_dice::mix32;

using pixelroot32::networking::LoopbackBus;
using pixelroot32::networking::LoopbackTransport;
using pixelroot32::networking::NetAddress;
using pixelroot32::networking::NetCounter;
using pixelroot32::networking::NetDelivery;
using pixelroot32::networking::NetEvent;
using pixelroot32::networking::NetEventType;
using pixelroot32::networking::NetPeerInfo;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::kNetAddressSize;

// Fixed identities. The lower node id is A, so the score convention under test
// maps node 1 to "+1" and node 2 to "-1".
constexpr uint8_t kNodeIdA = 1;
constexpr uint8_t kNodeIdB = 2;
constexpr uint16_t kBootIdA = 1;
constexpr uint16_t kBootIdB = 2;

// Deterministic, readable addresses: every byte differs so a partial copy
// cannot pass unnoticed.
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

void assertPhase(DuelPhase expected, const DiceDuel& duel) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),
                                    static_cast<uint8_t>(duel.phase()),
                                    "Unexpected DuelPhase");
}

int32_t absScore(int32_t value) {
    return value < 0 ? -value : value;
}

// Two full sessions plus their counters and duels, wired to one loopback bus.
//
// Construction only wires references; initPair performs initialization so a
// failed setup is reported by Unity instead of escaping a constructor.
struct DuelPair {
    LoopbackBus bus;
    LoopbackTransport transportA;
    LoopbackTransport transportB;
    NetSession sessionA;
    NetSession sessionB;
    NetCounter counterA;
    NetCounter counterB;
    DiceDuel duelA;
    DiceDuel duelB;
    uint32_t seedBaseA;
    uint32_t seedBaseB;

    DuelPair(uint32_t baseA, uint32_t baseB)
        : transportA(bus, makeAddress(1)), transportB(bus, makeAddress(2)),
          sessionA(transportA), sessionB(transportB),
          duelA(sessionA, counterA, baseA), duelB(sessionB, counterB, baseB),
          seedBaseA(baseA), seedBaseB(baseB) {}
};

void initPair(DuelPair& pair) {
    assertNetResult(NetResult::Ok, pair.transportA.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok, pair.transportB.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    pair.sessionA.init(makeSessionConfig(kNodeIdA, kBootIdA)));
    assertNetResult(NetResult::Ok,
                    pair.sessionB.init(makeSessionConfig(kNodeIdB, kBootIdB)));
    assertNetResult(NetResult::Ok, pair.counterA.init(pair.sessionA));
    assertNetResult(NetResult::Ok, pair.counterB.init(pair.sessionB));
}

// The duel never owns the session queue: the caller drains it and hands each
// event to both the counter and the duel, exactly as the scene will.
void drain(NetSession& session, NetCounter& counter, DiceDuel& duel) {
    NetEvent event{};
    while (session.pollEvent(event)) {
        counter.onEvent(event);
        duel.onEvent(event);
    }
}

// One frame of work: the duels see time first, then the sessions move frames,
// then every event reaches the matching counter and duel.
void pump(DuelPair& pair, uint32_t nowMs) {
    pair.duelA.update(nowMs);
    pair.duelB.update(nowMs);
    pair.counterA.update(nowMs);
    pair.counterB.update(nowMs);
    pair.bus.setNowMs(nowMs);
    pair.sessionA.update(nowMs);
    pair.sessionB.update(nowMs);
    drain(pair.sessionA, pair.counterA, pair.duelA);
    drain(pair.sessionB, pair.counterB, pair.duelB);
}

// Repeated exchanges at one timestamp. The extra rounds are headroom so a test
// never depends on the exact order discovery, commits, reveals, ACKs and the
// replicated counter operation are queued in.
void settle(DuelPair& pair, uint32_t nowMs, uint8_t rounds = 8) {
    for (uint8_t i = 0; i < rounds; ++i) {
        pump(pair, nowMs);
    }
}

// Returns the first online peer slot, the same rule the duel uses, or 0xFF.
uint8_t onlinePeerSlot(NetSession& session) {
    for (uint8_t i = 0; i < NetSession::kMaxPeers; ++i) {
        NetPeerInfo info{};
        if (session.peerInfo(i, info) && info.online) {
            return i;
        }
    }
    return 0xFF;
}

void putU16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

void putU32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

void encodeCommit(uint8_t* out, uint16_t round, uint32_t commit) {
    out[0] = esp_now_dice::kDuelCommitTag;
    putU16(out + 1, round);
    putU32(out + 3, commit);
}

void encodeReveal(uint8_t* out, uint16_t round, uint32_t seed) {
    out[0] = esp_now_dice::kDuelRevealTag;
    putU16(out + 1, round);
    putU32(out + 3, seed);
}

// Builds a synthetic Message event with an exact payload, as the session would
// deliver it. Used to inject a duplicate, a conflicting commitment or a forged
// reveal that a correct peer would never send.
NetEvent makeMessageEvent(uint8_t peerId, const uint8_t* bytes,
                          uint16_t length) {
    NetEvent event{};
    event.type = NetEventType::Message;
    event.peerId = peerId;
    event.nodeId = 0;
    event.length = length;
    for (uint16_t i = 0; i < length; ++i) {
        event.payload[i] = bytes[i];
    }
    return event;
}

// Mirrors the seed derivation the duel performs, so a test can predict a
// peer's commitment without reading private state.
uint32_t seedFor(uint32_t base, uint8_t nodeId, uint16_t round) {
    return mix32(base ^ (static_cast<uint32_t>(round) << 16) ^
                 static_cast<uint32_t>(nodeId));
}

// Acknowledges a finished round on one board: a press in Result only moves the
// duel to Idle; a second press actually starts the next round.
void acknowledgeResult(DiceDuel& duel) {
    if (duel.phase() == DuelPhase::Result) {
        duel.press();
    }
}

}  // namespace

// =============================================================================
// 1. One round: both boards press, the wire carries the commits and reveals,
//    and both derive the same two dice from the canonical seed order.
// =============================================================================

void test_esp_now_dice_single_round_mirrors_dice(void) {
    DuelPair pair(0x1234ABCDu, 0x5678EF01u);
    initPair(pair);
    settle(pair, 0);

    TEST_ASSERT_EQUAL_UINT8(1, pair.sessionA.peerCount());
    TEST_ASSERT_EQUAL_UINT8(1, pair.sessionB.peerCount());

    pair.duelA.press();
    pair.duelB.press();
    settle(pair, 0, 12);

    assertPhase(DuelPhase::Result, pair.duelA);
    assertPhase(DuelPhase::Result, pair.duelB);
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelA.round());
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelB.round());

    // Canonical seed order: A (lower node id) derives die one first, B sees the
    // mirrored pair. A's local die is B's peer die and vice versa.
    TEST_ASSERT_EQUAL_UINT8(pair.duelA.localDie(), pair.duelB.peerDie());
    TEST_ASSERT_EQUAL_UINT8(pair.duelA.peerDie(), pair.duelB.localDie());
    TEST_ASSERT_TRUE(pair.duelA.localDie() >= 1);
    TEST_ASSERT_TRUE(pair.duelA.localDie() <= esp_now_dice::kDiceFaces);
    TEST_ASSERT_TRUE(pair.duelA.peerDie() >= 1);
    TEST_ASSERT_TRUE(pair.duelA.peerDie() <= esp_now_dice::kDiceFaces);

    const bool tie = pair.duelA.localDie() == pair.duelA.peerDie();
    TEST_ASSERT_EQUAL_UINT8(tie ? 1 : 0, pair.duelA.tieRound() ? 1 : 0);
    TEST_ASSERT_EQUAL_UINT8(tie ? 1 : 0, pair.duelB.tieRound() ? 1 : 0);
    if (tie) {
        TEST_ASSERT_FALSE(pair.duelA.localWonRound());
        TEST_ASSERT_FALSE(pair.duelB.localWonRound());
    } else {
        // Exactly one board must claim the win; the mirrored dice make the
        // loser's localWonRound stay false.
        TEST_ASSERT_TRUE(pair.duelA.localWonRound() !=
                         pair.duelB.localWonRound());
    }

    // The shared score converges on both boards and only one of them applied
    // it: the winner's add replicates to the loser's counter.
    TEST_ASSERT_EQUAL_INT32(pair.counterA.value(), pair.counterB.value());
    TEST_ASSERT_EQUAL_INT32(pair.counterA.value(), pair.duelA.netScore());
    TEST_ASSERT_EQUAL_INT32(pair.counterB.value(), pair.duelB.netScore());

    int32_t expected = 0;
    if (!tie) {
        expected = pair.duelA.localWonRound() ? 1 : -1;
    }
    TEST_ASSERT_EQUAL_INT32(expected, pair.counterA.value());
}

// =============================================================================
// 1b. Asymmetric press: A presses first. B receives the commitment while Idle
//     and adopts that round on its own press instead of dropping the frame and
//     letting both sides time out.
// =============================================================================

void test_esp_now_dice_second_player_joins_pending_round(void) {
    DuelPair pair(0x0BADF00Du, 0x0DDBA11u);
    initPair(pair);
    settle(pair, 0);

    pair.duelA.press();
    pump(pair, 0);

    // B buffered A's commitment but has not started a round of its own yet.
    assertPhase(DuelPhase::Idle, pair.duelB);
    TEST_ASSERT_EQUAL_UINT16(0, pair.duelB.round());

    pair.duelB.press();
    settle(pair, 0, 12);

    assertPhase(DuelPhase::Result, pair.duelA);
    assertPhase(DuelPhase::Result, pair.duelB);
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelA.round());
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelB.round());
    TEST_ASSERT_EQUAL_UINT8(pair.duelA.localDie(), pair.duelB.peerDie());
    TEST_ASSERT_EQUAL_UINT8(pair.duelA.peerDie(), pair.duelB.localDie());
    TEST_ASSERT_EQUAL_INT32(pair.counterA.value(), pair.counterB.value());
}

// =============================================================================
// 2. Many rounds: the round number advances, every round moves the shared score
//    by at most one, and both boards always agree. A match ends at the target,
//    so the loop stops there instead of pretending ten rounds fit in one match.
// =============================================================================

void test_esp_now_dice_ten_rounds_converge(void) {
    DuelPair pair(0x2468ACE0u, 0x13579BDFu);
    initPair(pair);
    settle(pair, 0);

    uint8_t roundsPlayed = 0;
    uint8_t expectedRound = 0;

    for (uint8_t iteration = 0; iteration < 12; ++iteration) {
        if (pair.duelA.matchOver() && pair.duelB.matchOver()) {
            break;
        }

        // A press in Result only acknowledges it; both boards must be Idle
        // before the next round can start.
        acknowledgeResult(pair.duelA);
        acknowledgeResult(pair.duelB);
        settle(pair, 0, 2);

        if (pair.duelA.phase() != DuelPhase::Idle ||
            pair.duelB.phase() != DuelPhase::Idle) {
            break;
        }

        const int32_t before = pair.counterA.value();

        pair.duelA.press();
        pair.duelB.press();
        settle(pair, 0, 12);

        ++roundsPlayed;
        ++expectedRound;

        TEST_ASSERT_EQUAL_UINT16(expectedRound, pair.duelA.round());
        TEST_ASSERT_EQUAL_UINT16(expectedRound, pair.duelB.round());
        TEST_ASSERT_EQUAL_INT32(pair.counterA.value(), pair.counterB.value());

        const int32_t delta = pair.counterA.value() - before;
        TEST_ASSERT_TRUE(delta >= -1);
        TEST_ASSERT_TRUE(delta <= 1);
    }

    TEST_ASSERT_TRUE_MESSAGE(roundsPlayed >= 3,
                             "a match must need at least three rounds");
    TEST_ASSERT_TRUE(pair.duelA.matchOver());
    TEST_ASSERT_TRUE(pair.duelB.matchOver());
    TEST_ASSERT_EQUAL_INT32(esp_now_dice::kDuelTargetScore,
                            absScore(pair.counterA.value()));
}

// =============================================================================
// 3. Tie handling: a forced tie leaves the shared score untouched and raises
//    the tie flag on both boards.
// =============================================================================

void test_esp_now_dice_tie_round_scores_nothing(void) {
    uint32_t tieBaseA = 0;
    uint32_t tieBaseB = 0;
    bool found = false;

    for (uint32_t a = 1; a <= 256 && !found; ++a) {
        for (uint32_t b = 1; b <= 256 && !found; ++b) {
            const DicePair dice = computeDice(seedFor(a, kNodeIdA, 1),
                                              seedFor(b, kNodeIdB, 1), 1);
            if (dice.first == dice.second) {
                tieBaseA = a;
                tieBaseB = b;
                found = true;
            }
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(found, "no seed-base pair produced a round-one tie");

    DuelPair pair(tieBaseA, tieBaseB);
    initPair(pair);
    settle(pair, 0);

    pair.duelA.press();
    pair.duelB.press();
    settle(pair, 0, 12);

    assertPhase(DuelPhase::Result, pair.duelA);
    assertPhase(DuelPhase::Result, pair.duelB);
    TEST_ASSERT_TRUE(pair.duelA.tieRound());
    TEST_ASSERT_TRUE(pair.duelB.tieRound());
    TEST_ASSERT_FALSE(pair.duelA.localWonRound());
    TEST_ASSERT_FALSE(pair.duelB.localWonRound());
    TEST_ASSERT_EQUAL_UINT8(pair.duelA.localDie(), pair.duelA.peerDie());
    TEST_ASSERT_EQUAL_UINT8(pair.duelB.localDie(), pair.duelB.peerDie());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelA.netScore());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelB.netScore());
}

// =============================================================================
// 4. Tampering: a reveal whose seed does not match the published commitment
//    aborts the round with tampered set and no score.
// =============================================================================

void test_esp_now_dice_tampered_reveal_aborts(void) {
    DuelPair pair(0x01020304u, 0x05060708u);
    initPair(pair);
    settle(pair, 0);

    // Only A starts a round. Its commit goes to B, which is still Idle and
    // ignores it, so B never sends a competing reveal.
    pair.duelA.press();
    assertPhase(DuelPhase::Committing, pair.duelA);

    const uint8_t slotA = onlinePeerSlot(pair.sessionA);
    TEST_ASSERT_NOT_EQUAL(0xFF, slotA);

    // Feed B's real commitment so A advances to Revealing and publishes its
    // own reveal, exactly as a real exchange would.
    const uint32_t seedB = seedFor(pair.seedBaseB, kNodeIdB, 1);
    uint8_t commitFrame[7];
    encodeCommit(commitFrame, 1, makeCommit(seedB, 1));
    pair.duelA.onEvent(makeMessageEvent(slotA, commitFrame, 7));
    assertPhase(DuelPhase::Revealing, pair.duelA);

    // A raw Reveal frame with a seed that does not match that commitment is
    // injected through B's transport, so A receives it as a real event.
    uint8_t forgedReveal[7];
    encodeReveal(forgedReveal, 1, seedB ^ 0x1u);
    const uint8_t slotB = onlinePeerSlot(pair.sessionB);
    TEST_ASSERT_NOT_EQUAL(0xFF, slotB);
    assertNetResult(NetResult::Ok,
                    pair.sessionB.send(slotB, NetDelivery::Reliable,
                                       forgedReveal, sizeof(forgedReveal)));

    settle(pair, 0, 4);

    TEST_ASSERT_TRUE(pair.duelA.tampered());
    assertPhase(DuelPhase::Idle, pair.duelA);
    TEST_ASSERT_FALSE(pair.duelA.timedOut());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelA.netScore());
}

// =============================================================================
// 5. Duplicate messages: the same commit and reveal delivered twice resolve the
//    round once and move the shared score exactly once. A conflicting commit
//    for the same round is tampering.
// =============================================================================

void test_esp_now_dice_duplicate_messages_apply_once(void) {
    DuelPair pair(0x0BADC0DEu, 0x0BADF00Du);
    initPair(pair);
    settle(pair, 0);

    const uint32_t revisionA = pair.counterA.revision();
    const uint32_t revisionB = pair.counterB.revision();

    pair.duelA.press();
    pair.duelB.press();

    const uint8_t slotA = onlinePeerSlot(pair.sessionA);
    const uint8_t slotB = onlinePeerSlot(pair.sessionB);
    TEST_ASSERT_NOT_EQUAL(0xFF, slotA);
    TEST_ASSERT_NOT_EQUAL(0xFF, slotB);

    const uint32_t seedA = seedFor(pair.seedBaseA, kNodeIdA, 1);
    const uint32_t seedB = seedFor(pair.seedBaseB, kNodeIdB, 1);

    uint8_t commitA[7];
    uint8_t commitB[7];
    uint8_t revealA[7];
    uint8_t revealB[7];
    encodeCommit(commitA, 1, makeCommit(seedA, 1));
    encodeCommit(commitB, 1, makeCommit(seedB, 1));
    encodeReveal(revealA, 1, seedA);
    encodeReveal(revealB, 1, seedB);

    // Deliver each commitment twice: the first is stored, the second is a
    // duplicate that must not open a second round or change the commitment.
    const NetEvent commitEventA = makeMessageEvent(slotA, commitB, 7);
    const NetEvent commitEventB = makeMessageEvent(slotB, commitA, 7);
    pair.duelA.onEvent(commitEventA);
    pair.duelA.onEvent(commitEventA);
    pair.duelB.onEvent(commitEventB);
    pair.duelB.onEvent(commitEventB);

    assertPhase(DuelPhase::Revealing, pair.duelA);
    assertPhase(DuelPhase::Revealing, pair.duelB);

    // Deliver each reveal twice: the first resolves the round, the second is
    // ignored because the phase already left Revealing.
    const NetEvent revealEventA = makeMessageEvent(slotA, revealB, 7);
    const NetEvent revealEventB = makeMessageEvent(slotB, revealA, 7);
    pair.duelA.onEvent(revealEventA);
    pair.duelA.onEvent(revealEventA);
    pair.duelB.onEvent(revealEventB);
    pair.duelB.onEvent(revealEventB);

    assertPhase(DuelPhase::Result, pair.duelA);
    assertPhase(DuelPhase::Result, pair.duelB);

    const bool tie = pair.duelA.localDie() == pair.duelA.peerDie();
    const int32_t expected = tie ? 0 : (pair.duelA.localWonRound() ? 1 : -1);
    const int32_t beforeReplicationA = pair.duelA.localWonRound() ? 1 : 0;
    const int32_t beforeReplicationB = pair.duelB.localWonRound() ? -1 : 0;

    // Before replication only the winning board holds the delta.
    TEST_ASSERT_EQUAL_INT32(beforeReplicationA, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(beforeReplicationB, pair.counterB.value());

    settle(pair, 0, 8);

    TEST_ASSERT_EQUAL_INT32(expected, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(expected, pair.counterB.value());
    TEST_ASSERT_EQUAL_UINT32(revisionA + (tie ? 0u : 1u),
                             pair.counterA.revision());
    TEST_ASSERT_EQUAL_UINT32(revisionB + (tie ? 0u : 1u),
                             pair.counterB.revision());
}

void test_esp_now_dice_conflicting_commit_is_tampering(void) {
    DuelPair pair(0x0BADCAFEu, 0x0BADBEEFu);
    initPair(pair);
    settle(pair, 0);

    // Fill A's reliable pending queue so its own commit cannot be sent and the
    // board stays in Committing long enough to see two peer commitments. This
    // is the only state in which the second commit is inspected instead of
    // being ignored by the phase gate.
    const uint8_t capacity =
        static_cast<uint8_t>(pixelroot32::platforms::config::NetTxQueueCapacity);
    const uint8_t slotA = onlinePeerSlot(pair.sessionA);
    TEST_ASSERT_NOT_EQUAL(0xFF, slotA);

    const uint8_t filler[2] = {0x7Eu, 0x00u};
    while (pair.sessionA.pendingCount() < capacity) {
        (void)pair.sessionA.send(slotA, NetDelivery::Reliable, filler,
                                 sizeof(filler));
    }
    TEST_ASSERT_EQUAL_UINT8(capacity, pair.sessionA.pendingCount());

    pair.duelA.press();
    assertPhase(DuelPhase::Committing, pair.duelA);

    const uint32_t seedB = seedFor(pair.seedBaseB, kNodeIdB, 1);
    const uint32_t commit = makeCommit(seedB, 1);

    uint8_t sameCommit[7];
    encodeCommit(sameCommit, 1, commit);
    const NetEvent sameEvent = makeMessageEvent(slotA, sameCommit, 7);

    pair.duelA.onEvent(sameEvent);
    pair.duelA.onEvent(sameEvent);
    TEST_ASSERT_FALSE(pair.duelA.tampered());
    assertPhase(DuelPhase::Committing, pair.duelA);

    uint8_t otherCommit[7];
    encodeCommit(otherCommit, 1, commit ^ 0x1u);
    pair.duelA.onEvent(makeMessageEvent(slotA, otherCommit, 7));

    TEST_ASSERT_TRUE(pair.duelA.tampered());
    assertPhase(DuelPhase::Idle, pair.duelA);
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
}

// =============================================================================
// 6. Timeout: a round that never receives the peer half aborts to Idle with
//    timedOut set and no score.
// =============================================================================

void test_esp_now_dice_timeout_aborts_round(void) {
    DuelPair pair(0x0A0B0C0Du, 0x0E0F1011u);
    initPair(pair);
    settle(pair, 0);

    pair.duelA.press();
    assertPhase(DuelPhase::Committing, pair.duelA);

    pump(pair, 0);
    assertPhase(DuelPhase::Committing, pair.duelA);
    TEST_ASSERT_FALSE(pair.duelA.timedOut());

    pump(pair, static_cast<uint32_t>(esp_now_dice::kDuelRoundTimeoutMs));

    assertPhase(DuelPhase::Idle, pair.duelA);
    TEST_ASSERT_TRUE(pair.duelA.timedOut());
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelA.round());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelA.netScore());

    // The peer never started a round.
    assertPhase(DuelPhase::Idle, pair.duelB);
    TEST_ASSERT_FALSE(pair.duelB.timedOut());
}

// =============================================================================
// 7. PeerLeft mid-round aborts to Idle without the timeout flag and without a
//    score, because the round can no longer complete.
// =============================================================================

void test_esp_now_dice_peer_left_aborts_without_timeout(void) {
    DuelPair pair(0x11121314u, 0x15161718u);
    initPair(pair);
    settle(pair, 0);

    pair.duelA.press();

    const uint8_t slotA = onlinePeerSlot(pair.sessionA);
    TEST_ASSERT_NOT_EQUAL(0xFF, slotA);
    const uint32_t seedB = seedFor(pair.seedBaseB, kNodeIdB, 1);
    uint8_t commitFrame[7];
    encodeCommit(commitFrame, 1, makeCommit(seedB, 1));

    // The commitment moves A to Revealing; B never sent a reveal, so the round
    // is still in flight when B leaves.
    pair.duelA.onEvent(makeMessageEvent(slotA, commitFrame, 7));
    assertPhase(DuelPhase::Revealing, pair.duelA);

    pair.sessionB.shutdown();
    pair.transportB.shutdown();

    // Only A remains on the bus: the BYE queued by B's shutdown is a PeerLeft.
    pair.duelA.update(0);
    pair.counterA.update(0);
    pair.bus.setNowMs(0);
    pair.sessionA.update(0);
    drain(pair.sessionA, pair.counterA, pair.duelA);

    assertPhase(DuelPhase::Idle, pair.duelA);
    TEST_ASSERT_FALSE(pair.duelA.timedOut());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelA.netScore());
}

// =============================================================================
// 8. Rematch: once the match is over, a press sends Reset and clears the shared
//    score on both boards, and a new round can start. A stale Reset must not be
//    able to zero a fresh match, which the phase gate handles.
// =============================================================================

void test_esp_now_dice_rematch_resets_both_boards(void) {
    DuelPair pair(0xDEADBEEFu, 0xFEEDFACEu);
    initPair(pair);
    settle(pair, 0);

    uint8_t guard = 0;
    while (!(pair.duelA.matchOver() && pair.duelB.matchOver()) && guard < 64) {
        ++guard;

        acknowledgeResult(pair.duelA);
        acknowledgeResult(pair.duelB);
        settle(pair, 0, 2);

        if (pair.duelA.phase() != DuelPhase::Idle ||
            pair.duelB.phase() != DuelPhase::Idle) {
            break;
        }

        pair.duelA.press();
        pair.duelB.press();
        settle(pair, 0, 12);
    }

    TEST_ASSERT_TRUE_MESSAGE(pair.duelA.matchOver(),
                             "the match did not reach the target score");
    TEST_ASSERT_TRUE(pair.duelB.matchOver());
    TEST_ASSERT_EQUAL_INT32(esp_now_dice::kDuelTargetScore,
                            absScore(pair.counterA.value()));

    // The loser is still in Result: one press moves it to MatchOver, exactly
    // like the winner that reached MatchOver while resolving the last round.
    acknowledgeResult(pair.duelA);
    acknowledgeResult(pair.duelB);
    assertPhase(DuelPhase::MatchOver, pair.duelA);
    assertPhase(DuelPhase::MatchOver, pair.duelB);

    // A press in MatchOver sends Reset and resets the local state and counter.
    pair.duelA.press();
    pair.duelB.press();
    settle(pair, 0, 8);

    assertPhase(DuelPhase::Idle, pair.duelA);
    assertPhase(DuelPhase::Idle, pair.duelB);
    TEST_ASSERT_EQUAL_UINT16(0, pair.duelA.round());
    TEST_ASSERT_EQUAL_UINT16(0, pair.duelB.round());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterA.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.counterB.value());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelA.netScore());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelB.netScore());

    // A new match starts from round one. Slip a stale Reset (round 0, from the
    // finished match) into the board while the new round is in flight: the
    // phase gate must ignore it instead of zeroing the fresh round and score.
    pair.duelA.press();
    pair.duelB.press();
    assertPhase(DuelPhase::Committing, pair.duelA);

    uint8_t staleReset[3];
    staleReset[0] = esp_now_dice::kDuelResetTag;
    putU16(staleReset + 1, 0);
    pair.duelA.onEvent(
        makeMessageEvent(onlinePeerSlot(pair.sessionA), staleReset,
                         sizeof(staleReset)));
    assertPhase(DuelPhase::Committing, pair.duelA);
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelA.round());

    settle(pair, 0, 12);

    TEST_ASSERT_EQUAL_UINT16(1, pair.duelA.round());
    TEST_ASSERT_EQUAL_UINT16(1, pair.duelB.round());
    TEST_ASSERT_TRUE(absScore(pair.counterA.value()) <= 1);
    TEST_ASSERT_EQUAL_INT32(pair.counterA.value(), pair.counterB.value());
}

// =============================================================================
// 9. A press in Idle with no online peer does nothing: no round, no frame, no
//    score change.
// =============================================================================

void test_esp_now_dice_press_without_peer_stays_idle(void) {
    DuelPair pair(0x21222324u, 0x25262728u);
    initPair(pair);

    TEST_ASSERT_EQUAL_UINT8(0, pair.sessionA.peerCount());
    TEST_ASSERT_EQUAL_UINT8(0, pair.sessionB.peerCount());

    pair.duelA.press();
    pair.duelB.press();

    assertPhase(DuelPhase::Idle, pair.duelA);
    assertPhase(DuelPhase::Idle, pair.duelB);
    TEST_ASSERT_EQUAL_UINT16(0, pair.duelA.round());
    TEST_ASSERT_EQUAL_UINT16(0, pair.duelB.round());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelA.netScore());
    TEST_ASSERT_EQUAL_INT32(0, pair.duelB.netScore());
    TEST_ASSERT_EQUAL_UINT32(0, pair.sessionA.stats().txMessages);
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_esp_now_dice_flag_off_stub_compiles(void) {
    // With the flag off (or on a target without the native loopback), neither
    // DiceDuel nor NetSession is compiled. This translation unit compiling and
    // passing without referencing any networking symbol is the zero-cost
    // flags-off property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_NETWORKING=0: no duel, counter or session symbols "
        "are compiled, zero bytes reserved.");
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
    RUN_TEST(test_esp_now_dice_single_round_mirrors_dice);
    RUN_TEST(test_esp_now_dice_second_player_joins_pending_round);
    RUN_TEST(test_esp_now_dice_ten_rounds_converge);
    RUN_TEST(test_esp_now_dice_tie_round_scores_nothing);
    RUN_TEST(test_esp_now_dice_tampered_reveal_aborts);
    RUN_TEST(test_esp_now_dice_duplicate_messages_apply_once);
    RUN_TEST(test_esp_now_dice_conflicting_commit_is_tampering);
    RUN_TEST(test_esp_now_dice_timeout_aborts_round);
    RUN_TEST(test_esp_now_dice_peer_left_aborts_without_timeout);
    RUN_TEST(test_esp_now_dice_rematch_resets_both_boards);
    RUN_TEST(test_esp_now_dice_press_without_peer_stays_idle);
#else
    RUN_TEST(test_esp_now_dice_flag_off_stub_compiles);
#endif

    return UNITY_END();
}

