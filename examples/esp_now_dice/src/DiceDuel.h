/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_NETWORKING

#include "networking/NetCounter.h"
#include "networking/NetSession.h"
#include "networking/NetTypes.h"

#include "DiceProtocol.h"

#include <cstdint>

/**
 * @file DiceDuel.h
 * @brief Platform-independent round state machine for the Dice Duel demo.
 *
 * One instance runs on each board. The duel owns no link, no clock and no
 * rendering: it borrows a NetSession for peer discovery and reliable
 * messaging, a NetCounter for the shared score, and receives the current time
 * through update(). That keeps the whole round flow testable on the native
 * loopback and lets the example scene wrap one instance per simulated board.
 *
 * Wire contract (little endian, all payloads sent with NetDelivery::Reliable):
 * @code
 *   Commit: [10][round u16][commit u32]   (7 bytes)
 *   Reveal: [11][round u16][seed u32]     (7 bytes)
 *   Reset:  [12][round u16 = 0]           (3 bytes)
 * @endcode
 *
 * Score convention: the lower node id is A and contributes +1 to the shared
 * NetCounter, the higher is B and contributes -1. The counter therefore holds
 * A - B on every board, and matchOver() is abs(A - B) >= kDuelTargetScore.
 *
 * Round flow:
 *   Idle --press (peer found)--> Committing --peer commit--> Revealing
 *        --peer reveal verified--> Result --press--> Idle (next round)
 *   When the shared score reaches the target, resolve goes to MatchOver
 *   instead of Result; a press there sends Reset and clears both the round
 *   state and the shared score.
 *
 * Failure flow: a conflicting commitment or an unverifiable reveal sets
 * tampered() and returns to Idle without moving the score; a round that runs
 * past kDuelRoundTimeoutMs returns to Idle with timedOut(); a PeerLeft for the
 * round peer returns to Idle without timedOut(). In every failure path the
 * round is abandoned, never half-scored.
 *
 * Budget contract: fixed members only, no heap, no exceptions, no logging, no
 * global state and no millis(). Every method is inline and every timer value
 * arrives through update(), so two boards and a native test agree on time.
 */
namespace esp_now_dice {

/** @brief Tag byte of one commitment payload. */
inline constexpr uint8_t kDuelCommitTag = 10;

/** @brief Tag byte of one seed reveal payload. */
inline constexpr uint8_t kDuelRevealTag = 11;

/** @brief Tag byte of one match reset payload. */
inline constexpr uint8_t kDuelResetTag = 12;

/** @brief Time a round may spend in Committing or Revealing before aborting. */
inline constexpr uint16_t kDuelRoundTimeoutMs = 3000;

/** @brief Shared score magnitude at which the match is over. */
inline constexpr int32_t kDuelTargetScore = 3;

/**
 * @enum DuelPhase
 * @brief Phase of one round, plus the two terminal match states.
 */
enum class DuelPhase : uint8_t {
    Idle = 0,        ///< No round in flight; a press starts one.
    Committing = 1,  ///< Local commit sent, peer commit awaited.
    Revealing = 2,   ///< Peer commit stored, peer seed awaited.
    Result = 3,      ///< Round resolved; a press starts the next round.
    MatchOver = 4    ///< Target score reached; a press sends Reset.
};

/**
 * @class DiceDuel
 * @brief One board's commit/reveal round state machine over NetSession.
 *
 * The class is a passive collaborator: it never pumps the session and never
 * reads the clock. The application calls update() once per frame before reading
 * input, then press() on a button, and forwards every session event through
 * onEvent(). The duel sends only what the protocol above describes.
 */
class DiceDuel {
public:
    /**
     * @brief Binds the duel to a session and a shared counter.
     * @param sessionRef Session used for peer discovery and reliable sends; it
     *        must outlive the duel.
     * @param counterRef Shared score, holding A - B; it must outlive the duel.
     * @param seedBaseValue Per-boot seed base mixed into every round seed.
     *
     * Construction only stores references and zeroes the round state: it does
     * not touch the session or the counter, so it is safe before either is
     * initialized. Tests pass a fixed base; the device scene passes a per-boot
     * value.
     */
    DiceDuel(pixelroot32::networking::NetSession& sessionRef,
             pixelroot32::networking::NetCounter& counterRef,
             uint32_t seedBaseValue)
        : session(sessionRef),
          counter(counterRef),
          seedBase(seedBaseValue) {}

    // A duel tracks its own round against one live link and one counter; two
    // copies would send duplicate frames and claim the same score slot.
    DiceDuel(const DiceDuel&) = delete;
    DiceDuel& operator=(const DiceDuel&) = delete;

    /**
     * @brief Performs a full match reset: state, peer cache and shared score.
     *
     * Clears every round field and rebuilds the counter's slot table through
     * shutdown() plus init(session). It sends nothing: the caller decides
     * whether the peer must also be reset (press() in MatchOver does).
     */
    void reset() {
        phaseValue = DuelPhase::Idle;
        roundValue = 0;
        localSeedValue = 0;
        peerSeedValue = 0;
        localCommitValue = 0;
        peerCommitValue = 0;
        localDieValue = 0;
        peerDieValue = 0;
        localWonValue = false;
        tieValue = false;
        tamperedValue = false;
        timedOutValue = false;
        hasPeerCommitValue = false;
        hasPeerSeedValue = false;
        localCommitSentValue = false;
        pendingScoreDeltaValue = 0;
        hasPendingPeerCommitValue = false;
        pendingPeerRoundValue = 0;
        pendingPeerCommitValue = 0;
        pendingPeerSlotValue = 0;
        pendingPeerNodeIdValue = 0;
        localIsAValue = true;
        peerSlotValue = 0;
        peerNodeIdValue = 0;
        phaseStartMs = 0;
        counter.shutdown();
        (void)counter.init(session);
    }

    /**
     * @brief Applies one button press according to the current phase.
     *
     * Idle discovers the first online peer and starts a round: it increments
     * the round, derives the seed, publishes the commitment and enters
     * Committing. The round is stamped with lastUpdateMs, so the application
     * must call update() once per frame before reading input and the timeout
     * baseline is the current frame. Committing and Revealing ignore presses.
     * Result moves to MatchOver when the target was reached, otherwise back to
     * Idle for the next round. MatchOver sends Reset to the peer and performs a
     * full local reset.
     *
     * With no online peer an Idle press does nothing at all, so a lone board
     * cannot start a round it can never complete.
     */
    void press() {
        switch (phaseValue) {
            case DuelPhase::Idle:
                // A round that resolves while the score is being restored
                // cannot bank its point; wait for the resync to finish.
                if (!counter.isResyncing()) {
                    startRound();
                }
                break;
            case DuelPhase::Result:
                phaseValue = matchOver() ? DuelPhase::MatchOver
                                         : DuelPhase::Idle;
                break;
            case DuelPhase::MatchOver:
                sendReset();
                reset();
                break;
            case DuelPhase::Committing:
            case DuelPhase::Revealing:
                break;
        }
    }

    /**
     * @brief Consumes one session event: peer lifecycle, commit, reveal, reset.
     * @param event Event just popped from the session queue.
     *
     * PeerJoined is informational and never auto-starts a round, so the match
     * begins on a deliberate press. PeerLeft for the round peer abandons an
     * in-flight round without marking it as a timeout. Message payloads from
     * any peer other than the cached round peer are ignored, which is how the
     * two-board demo ignores additional peers.
     */
    void onEvent(const pixelroot32::networking::NetEvent& event) {
        if (event.type == pixelroot32::networking::NetEventType::PeerLeft) {
            handlePeerLeft(event);
            return;
        }
        if (event.type != pixelroot32::networking::NetEventType::Message) {
            return;
        }
        if (event.length < 1) {
            return;
        }

        const uint8_t tag = event.payload[0];

        // A peer may press before this board does. Its commit arrives while we
        // are still Idle, so it is buffered; press() adopts that round instead
        // of inventing a new one, and the round then completes normally.
        if (phaseValue == DuelPhase::Idle && tag == kDuelCommitTag) {
            bufferPendingCommit(event);
            return;
        }

        if (event.peerId != peerSlotValue) {
            return;
        }

        switch (tag) {
            case kDuelCommitTag:
                handleCommit(event);
                break;
            case kDuelRevealTag:
                handleReveal(event);
                break;
            case kDuelResetTag:
                handleReset(event);
                break;
            default:
                break;
        }
    }

    /**
     * @brief Advances the round clock and expires an overdue round.
     * @param nowMs Current time in milliseconds. May wrap.
     *
     * Stores the frame timestamp first, then checks the timeout, so a press
     * later in the same frame is stamped with this exact value. In Committing
     * or Revealing, `nowMs - phaseStartMs >= kDuelRoundTimeoutMs` (unsigned,
     * wrap-safe) abandons the round with timedOut() set and no score.
     */
    void update(uint32_t nowMs) {
        lastUpdateMs = nowMs;
        if (pendingScoreDeltaValue != 0 && !counter.isResyncing()) {
            // The score op was deferred while the counter restored the match;
            // bank it as soon as the resync ends. Busy keeps it for next frame.
            const int8_t delta = pendingScoreDeltaValue;
            if (counter.add(delta) != pixelroot32::networking::NetResult::Busy) {
                pendingScoreDeltaValue = 0;
            }
        }
        if (phaseValue == DuelPhase::Result && matchOver()) {
            // The score op can land after the loser resolved its own copy of
            // the round: promote the phase so one press is enough to reset.
            phaseValue = DuelPhase::MatchOver;
        }
        if (phaseValue == DuelPhase::Committing ||
            phaseValue == DuelPhase::Revealing) {
            if ((nowMs - phaseStartMs) >=
                static_cast<uint32_t>(kDuelRoundTimeoutMs)) {
                phaseValue = DuelPhase::Idle;
                timedOutValue = true;
            }
        }
    }

    /**
     * @brief Returns the current phase.
     * @return Current DuelPhase.
     */
    DuelPhase phase() const { return phaseValue; }

    /**
     * @brief Returns the round number of the round in flight or just resolved.
     * @return Round number, starting at 1 and 0 before the first round.
     */
    uint16_t round() const { return roundValue; }

    /**
     * @brief Returns this board's die for the current round.
     * @return Face in 1..kDiceFaces, or 0 before a round resolves.
     */
    uint8_t localDie() const { return localDieValue; }

    /**
     * @brief Returns the peer's die for the current round.
     * @return Face in 1..kDiceFaces, or 0 before a round resolves.
     */
    uint8_t peerDie() const { return peerDieValue; }

    /**
     * @brief Returns the shared score as A - B (A is the lower node id).
     * @return Value read from the shared NetCounter on this board.
     */
    int32_t netScore() const { return counter.value(); }

    /**
     * @brief Reports whether this board won the last resolved round.
     * @return true only on the winning board; false on a tie or a loss.
     */
    bool localWonRound() const { return localWonValue; }

    /**
     * @brief Reports whether the last resolved round was a tie.
     * @return true on both boards when the two dice matched.
     */
    bool tieRound() const { return tieValue; }

    /**
     * @brief Reports whether a commitment or reveal failed verification.
     * @return true after the aborted round, until the next round starts.
     */
    bool tampered() const { return tamperedValue; }

    /**
     * @brief Reports whether the last round was abandoned by the timeout.
     * @return true after the aborted round, until the next round starts.
     */
    bool timedOut() const { return timedOutValue; }

    /**
     * @brief Reports whether the shared score reached the target magnitude.
     * @return true when abs(netScore()) >= kDuelTargetScore.
     *
     * Reads the counter, so it also turns true on the loser's board when the
     * winner's replicated operation arrives.
     */
    bool matchOver() const {
        const int32_t score = counter.value();
        // Compare against both bounds instead of negating: -INT32_MIN would
        // overflow, and a saturated counter must never flip the answer.
        return score >= kDuelTargetScore || score <= -kDuelTargetScore;
    }

private:
    /**
     * @brief Finds the first online peer and starts a new round.
     *
     * Caches the peer slot and node id for the whole round so a frame from any
     * other node is ignored. The canonical order is decided here, once: the
     * lower node id is A, and A's seed is always the first seed passed to
     * computeDice.
     */
    void startRound() {
        pixelroot32::networking::NetPeerInfo info{};
        const bool adoptPending = hasPendingPeerCommitValue;
        int8_t slot = -1;

        if (adoptPending) {
            // The peer pressed first. Join its round instead of starting a new
            // one; otherwise its commit would belong to a round this board
            // never acknowledges and both sides would time out.
            if (pendingPeerRoundValue == 0 ||
                !session.peerInfo(pendingPeerSlotValue, info) ||
                !info.online) {
                clearPendingCommit();
                return;
            }
            slot = static_cast<int8_t>(pendingPeerSlotValue);
            roundValue = pendingPeerRoundValue;
        } else {
            slot = findOnlinePeer(info);
            if (slot < 0) {
                return;
            }
            ++roundValue;
            if (roundValue == 0) {
                ++roundValue;
            }
        }

        peerSlotValue = static_cast<uint8_t>(slot);
        peerNodeIdValue = info.nodeId;
        if (session.localNodeId() == peerNodeIdValue) {
            // Two boards with the same label cannot agree on who is A; refuse
            // to score a round that has no canonical seed order.
            clearPendingCommit();
            return;
        }
        localIsAValue = session.localNodeId() < peerNodeIdValue;

        localSeedValue = 0;
        peerSeedValue = 0;
        localDieValue = 0;
        peerDieValue = 0;
        localWonValue = false;
        tieValue = false;
        tamperedValue = false;
        timedOutValue = false;
        hasPeerSeedValue = false;
        localCommitSentValue = false;

        if (adoptPending) {
            peerCommitValue = pendingPeerCommitValue;
            hasPeerCommitValue = true;
            clearPendingCommit();
        } else {
            peerCommitValue = 0;
            hasPeerCommitValue = false;
        }

        const uint32_t nodeId =
            static_cast<uint32_t>(session.localNodeId());
        localSeedValue =
            mix32(seedBase ^ (static_cast<uint32_t>(roundValue) << 16) ^
                  nodeId);
        localCommitValue = makeCommit(localSeedValue, roundValue);

        phaseValue = DuelPhase::Committing;
        phaseStartMs = lastUpdateMs;

        uint8_t frame[7];
        encodeCommit(frame, roundValue, localCommitValue);
        // A failed send leaves the local commitment untracked: the transition
        // to Revealing waits for "both sent and stored", so the round either
        // completes or expires instead of resolving on a half-sent exchange.
        localCommitSentValue =
            session.send(peerSlotValue,
                         pixelroot32::networking::NetDelivery::Reliable, frame,
                         sizeof(frame)) ==
            pixelroot32::networking::NetResult::Ok;

        if (localCommitSentValue && hasPeerCommitValue) {
            sendRevealAndEnterRevealing();
        }
    }

    /**
     * @brief Stores a commitment received while this board is still Idle.
     * @param event Message event carrying a commit payload.
     *
     * The pending commit is adopted by the next press(), which lets either
     * player start the round without discarding the other's commitment. A
     * second, different commitment for the same round is equivocation and is
     * flagged before it can produce a score.
     */
    void bufferPendingCommit(
        const pixelroot32::networking::NetEvent& event) {
        if (event.length < 7) {
            return;
        }
        const uint16_t pendingRound = readU16(event.payload + 1);
        if (pendingRound == 0) {
            return;
        }
        const uint32_t commit = readU32(event.payload + 3);

        if (hasPendingPeerCommitValue &&
            pendingPeerRoundValue == pendingRound) {
            if (pendingPeerCommitValue != commit) {
                tamperedValue = true;
            }
            return;
        }

        hasPendingPeerCommitValue = true;
        pendingPeerRoundValue = pendingRound;
        pendingPeerCommitValue = commit;
        pendingPeerSlotValue = event.peerId;
        pendingPeerNodeIdValue = event.nodeId;
    }

    /** @brief Drops a commitment buffered while this board was Idle. */
    void clearPendingCommit() {
        hasPendingPeerCommitValue = false;
        pendingPeerRoundValue = 0;
        pendingPeerCommitValue = 0;
        pendingPeerSlotValue = 0;
        pendingPeerNodeIdValue = 0;
    }

    /**
     * @brief Returns the first online peer slot and copies its snapshot.
     * @param out Receives the peer snapshot on success.
     * @return Slot index, or -1 when no peer is online.
     */
    int8_t findOnlinePeer(pixelroot32::networking::NetPeerInfo& out) const {
        for (uint8_t i = 0; i < pixelroot32::networking::NetSession::kMaxPeers;
             ++i) {
            pixelroot32::networking::NetPeerInfo info{};
            if (session.peerInfo(i, info) && info.online) {
                out = info;
                return static_cast<int8_t>(i);
            }
        }
        return -1;
    }

    /**
     * @brief Stores the peer commitment and, once both halves are present,
     *        sends the local reveal and enters Revealing.
     * @param event Message event carrying a commit payload.
     */
    void handleCommit(const pixelroot32::networking::NetEvent& event) {
        if (event.length < 7) {
            return;
        }
        if (phaseValue != DuelPhase::Committing) {
            return;
        }
        if (readU16(event.payload + 1) != roundValue) {
            return;
        }

        const uint32_t commit = readU32(event.payload + 3);
        if (hasPeerCommitValue) {
            if (commit == peerCommitValue) {
                return;
            }
            // The peer equivocated on the same round: abandon it before any
            // score can be derived from a commitment it can no longer honor.
            tamperedValue = true;
            phaseValue = DuelPhase::Idle;
            return;
        }

        peerCommitValue = commit;
        hasPeerCommitValue = true;

        if (localCommitSentValue) {
            sendRevealAndEnterRevealing();
        }
    }

    /**
     * @brief Sends the local reveal and moves the round to Revealing.
     */
    void sendRevealAndEnterRevealing() {
        uint8_t frame[7];
        encodeReveal(frame, roundValue, localSeedValue);
        (void)session.send(peerSlotValue,
                           pixelroot32::networking::NetDelivery::Reliable,
                           frame, sizeof(frame));
        phaseValue = DuelPhase::Revealing;
    }

    /**
     * @brief Verifies the peer reveal and resolves the round when it is valid.
     * @param event Message event carrying a reveal payload.
     */
    void handleReveal(const pixelroot32::networking::NetEvent& event) {
        if (event.length < 7) {
            return;
        }
        if (phaseValue != DuelPhase::Revealing) {
            return;
        }
        if (readU16(event.payload + 1) != roundValue) {
            return;
        }

        const uint32_t seed = readU32(event.payload + 3);
        if (hasPeerSeedValue) {
            if (seed == peerSeedValue) {
                return;
            }
            tamperedValue = true;
            phaseValue = DuelPhase::Idle;
            return;
        }

        if (!verifyCommit(seed, roundValue, peerCommitValue)) {
            // The seed cannot reproduce the commitment the peer published, so
            // the dice would not be trustworthy: abandon without scoring.
            tamperedValue = true;
            phaseValue = DuelPhase::Idle;
            return;
        }

        peerSeedValue = seed;
        hasPeerSeedValue = true;
        resolveRound();
    }

    /**
     * @brief Applies the two seeds in canonical order and scores the round.
     *
     * Both boards derive the same dice because both pass the lower node id's
     * seed first. The die mapping is mirrored per board, and only the winning
     * board calls the counter, so one round produces exactly one delta on the
     * wire.
     */
    void resolveRound() {
        const uint32_t seedA = localIsAValue ? localSeedValue : peerSeedValue;
        const uint32_t seedB = localIsAValue ? peerSeedValue : localSeedValue;
        const DicePair dice = computeDice(seedA, seedB, roundValue);

        localDieValue = localIsAValue ? dice.first : dice.second;
        peerDieValue = localIsAValue ? dice.second : dice.first;

        if (localDieValue == peerDieValue) {
            tieValue = true;
        } else if (localDieValue > peerDieValue) {
            localWonValue = true;
            const int8_t delta = localIsAValue ? 1 : -1;
            if (counter.isResyncing()) {
                // The counter is restoring the match; bank the point after it
                // finishes instead of losing it to a Busy answer.
                pendingScoreDeltaValue = delta;
            } else if (counter.add(delta) ==
                       pixelroot32::networking::NetResult::Busy) {
                pendingScoreDeltaValue = delta;
            }
        }

        phaseValue = matchOver() ? DuelPhase::MatchOver : DuelPhase::Result;
    }

    /**
     * @brief Accepts a peer Reset only at the end of this side's match.
     * @param event Message event carrying a reset payload.
     *
     * The phase gate is what keeps a stale Reset from zeroing a new match: once
     * this board has started a fresh round it is never in MatchOver or in a
     * finished Result, so the stale frame is ignored.
     */
    void handleReset(const pixelroot32::networking::NetEvent& event) {
        if (event.length < 3) {
            return;
        }
        if (phaseValue == DuelPhase::MatchOver ||
            (phaseValue == DuelPhase::Result && matchOver())) {
            reset();
        }
    }

    /**
     * @brief Abandons an in-flight round when the cached peer leaves.
     * @param event PeerLeft event.
     *
     * A departure is not a timeout: the round ended for a link reason, and the
     * game can tell the two apart when it reports the result.
     */
    void handlePeerLeft(const pixelroot32::networking::NetEvent& event) {
        if (hasPendingPeerCommitValue &&
            event.peerId == pendingPeerSlotValue) {
            clearPendingCommit();
        }
        if (event.peerId != peerSlotValue) {
            return;
        }
        if (phaseValue == DuelPhase::Committing ||
            phaseValue == DuelPhase::Revealing) {
            phaseValue = DuelPhase::Idle;
        }
    }

    /**
     * @brief Publishes the Reset frame that clears the peer's match.
     */
    void sendReset() {
        uint8_t frame[3];
        frame[0] = kDuelResetTag;
        writeU16(frame + 1, 0);
        (void)session.send(peerSlotValue,
                           pixelroot32::networking::NetDelivery::Reliable,
                           frame, sizeof(frame));
    }

    /**
     * @brief Encodes a Commit payload.
     * @param out Destination buffer of at least 7 bytes.
     * @param round Round the commitment belongs to.
     * @param commit Commitment produced by makeCommit.
     */
    static void encodeCommit(uint8_t* out, uint16_t round, uint32_t commit) {
        out[0] = kDuelCommitTag;
        writeU16(out + 1, round);
        writeU32(out + 3, commit);
    }

    /**
     * @brief Encodes a Reveal payload.
     * @param out Destination buffer of at least 7 bytes.
     * @param round Round the seed belongs to.
     * @param seed Seed to reveal.
     */
    static void encodeReveal(uint8_t* out, uint16_t round, uint32_t seed) {
        out[0] = kDuelRevealTag;
        writeU16(out + 1, round);
        writeU32(out + 3, seed);
    }

    /**
     * @brief Writes a 16-bit value little endian.
     * @param out Destination buffer of at least 2 bytes.
     * @param value Value to write.
     */
    static void writeU16(uint8_t* out, uint16_t value) {
        out[0] = static_cast<uint8_t>(value & 0xFFu);
        out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    }

    /**
     * @brief Writes a 32-bit value little endian.
     * @param out Destination buffer of at least 4 bytes.
     * @param value Value to write.
     */
    static void writeU32(uint8_t* out, uint32_t value) {
        out[0] = static_cast<uint8_t>(value & 0xFFu);
        out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
        out[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
        out[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
    }

    /**
     * @brief Reads a 16-bit little-endian value.
     * @param in Source buffer of at least 2 bytes.
     * @return Decoded value.
     */
    static uint16_t readU16(const uint8_t* in) {
        return static_cast<uint16_t>(static_cast<uint16_t>(in[0]) |
                                     (static_cast<uint16_t>(in[1]) << 8));
    }

    /**
     * @brief Reads a 32-bit little-endian value.
     * @param in Source buffer of at least 4 bytes.
     * @return Decoded value.
     */
    static uint32_t readU32(const uint8_t* in) {
        return static_cast<uint32_t>(in[0]) |
               (static_cast<uint32_t>(in[1]) << 8) |
               (static_cast<uint32_t>(in[2]) << 16) |
               (static_cast<uint32_t>(in[3]) << 24);
    }

    pixelroot32::networking::NetSession& session;  ///< Borrowed link; never owned.
    pixelroot32::networking::NetCounter& counter;  ///< Borrowed shared score.
    uint32_t seedBase;     ///< Per-boot seed base mixed into each round seed.

    uint32_t lastUpdateMs = 0;   ///< Timestamp of the last update() call.
    uint32_t phaseStartMs = 0;   ///< Timestamp stamped when the round started.
    uint32_t localSeedValue = 0; ///< This board's secret seed for the round.
    uint32_t peerSeedValue = 0;  ///< Peer seed after a verified reveal.
    uint32_t localCommitValue = 0;  ///< Commitment this board published.
    uint32_t peerCommitValue = 0;   ///< Commitment the peer published.

    uint8_t localDieValue = 0;         ///< This board's resolved face.
    uint8_t peerDieValue = 0;          ///< Peer's resolved face.
    uint16_t roundValue = 0;           ///< Round number, 0 before the first.
    uint8_t peerSlotValue = 0;         ///< Cached peer slot for the round.
    uint8_t peerNodeIdValue = 0;       ///< Cached peer node id for the round.

    uint16_t pendingPeerRoundValue = 0;   ///< Round of a commit buffered in Idle.
    uint32_t pendingPeerCommitValue = 0;  ///< Commit buffered in Idle.
    uint8_t pendingPeerSlotValue = 0;     ///< Slot that sent the buffered commit.
    uint8_t pendingPeerNodeIdValue = 0;   ///< Label of that peer.
    bool hasPendingPeerCommitValue = false;  ///< true while a commit is buffered.

    DuelPhase phaseValue = DuelPhase::Idle;  ///< Current phase.

    bool localIsAValue = true;      ///< true when this board is the lower id.
    bool localWonValue = false;     ///< true on the winning board only.
    bool tieValue = false;          ///< true when both dice matched.
    bool tamperedValue = false;     ///< true after a failed verification.
    bool timedOutValue = false;     ///< true after a timeout abort.
    int8_t pendingScoreDeltaValue = 0;  ///< Point deferred while the score resyncs.
    bool hasPeerCommitValue = false;  ///< true once a peer commit is stored.
    bool hasPeerSeedValue = false;    ///< true once a peer seed is verified.
    bool localCommitSentValue = false;  ///< true when the commit was accepted.
};

}  // namespace esp_now_dice

#endif  // PIXELROOT32_ENABLE_NETWORKING
