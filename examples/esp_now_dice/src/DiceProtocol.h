/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

/**
 * @file DiceProtocol.h
 * @brief Pure, platform-independent commitment and dice math for Dice Duel.
 *
 * Both boards must derive the identical result from the same inputs. This
 * header is the whole wire contract: it has no global state, performs no
 * allocation, and includes no platform header, so the example and the native
 * unit tests can share it.
 *
 * Wire contract (all constants are 32-bit, unsigned arithmetic wraps modulo
 * 2^32):
 *
 *   mix32(x)        = SplitMix32 finalizer applied to (x + 0x9e3779b9)
 *   roundKey(r)     = mix32(r ^ 0x9e3779b9)
 *   commit(s, r)    = mix32(mix32(s ^ roundKey(r)) ^
 *                           mix32(s ^ (roundKey(r) + 0x434f4d4d)))
 *   die1(a, b, r)   = mix32(a ^ mix32(b ^ roundKey(r)) ^ 0x44494531) % 6 + 1
 *   die2(a, b, r)   = mix32(a ^ mix32(b ^ roundKey(r)) ^ 0x44494532) % 6 + 1
 *
 * Both dice use both seeds and the round; the two different domain constants
 * (0x44494531 is ASCII "DIE1", 0x44494532 is "DIE2") keep them independent.
 * The seed order is not symmetric: both boards must pass the seeds in the same
 * canonical order (the lower node id's seed first).
 */
namespace esp_now_dice {

/**
 * @brief Number of faces on one die.
 */
inline constexpr uint8_t kDiceFaces = 6;

/**
 * @brief The two dice derived for one round.
 */
struct DicePair {
    uint8_t first = 0;   ///< First die, 1..kDiceFaces.
    uint8_t second = 0;  ///< Second die, 1..kDiceFaces.
};

namespace detail {

// SplitMix32 seed increment: adding it makes mix32(0) nonzero and keeps every
// input away from the fixed point of the finalizer.
inline constexpr uint32_t kMixSalt = 0x9e3779b9u;

// Domain separation constants. The ASCII spellings are documentation only.
inline constexpr uint32_t kCommitDomain = 0x434f4d4du;    // "COMM"
inline constexpr uint32_t kFirstDieDomain = 0x44494531u;  // "DIE1"
inline constexpr uint32_t kSecondDieDomain = 0x44494532u; // "DIE2"

}  // namespace detail

/**
 * @brief Stateless 32-bit avalanche mix (SplitMix32 finalizer).
 *
 * Deterministic: the same input always yields the same output. Distinct inputs
 * produce unrelated-looking outputs, so callers can use it as a cheap
 * non-cryptographic mixer. This is not a cryptographic hash.
 *
 * @param value Input word.
 * @return Mixed 32-bit word; mix32(0) is nonzero.
 */
inline uint32_t mix32(uint32_t value) noexcept {
    uint32_t x = value + detail::kMixSalt;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

namespace detail {

/**
 * @brief Derive the per-round salt from the 16-bit round number.
 */
inline uint32_t roundKey(uint16_t round) noexcept {
    return mix32(static_cast<uint32_t>(round) ^ kMixSalt);
}

}  // namespace detail

/**
 * @brief Derive the commitment a board publishes before revealing its seed.
 *
 * The commitment binds the seed and the round. It is a 32-bit
 * non-cryptographic check for a friendly game, not a security primitive: a
 * determined peer can search the 32-bit seed space. It never contains the seed
 * verbatim, and the two folded mixes below avoid handing the seed back through
 * a single invertible mix.
 *
 * @param seed  Board's secret 32-bit seed.
 * @param round Round number the seed belongs to.
 * @return Commitment to publish.
 */
inline uint32_t makeCommit(uint32_t seed, uint16_t round) noexcept {
    const uint32_t salt = detail::roundKey(round);
    const uint32_t left = mix32(seed ^ salt);
    const uint32_t right = mix32(seed ^ (salt + detail::kCommitDomain));
    return mix32(left ^ right);
}

/**
 * @brief Check a revealed seed and round against a published commitment.
 *
 * Recomputes makeCommit(seed, round) and compares it with @p commit.
 *
 * @param seed  Revealed 32-bit seed.
 * @param round Round number the seed belongs to.
 * @param commit Commitment previously produced by makeCommit.
 * @return true when the commitment matches, false otherwise.
 */
inline bool verifyCommit(uint32_t seed, uint16_t round,
                         uint32_t commit) noexcept {
    return makeCommit(seed, round) == commit;
}

/**
 * @brief Derive both dice from the two revealed seeds and the round.
 *
 * Deterministic for the same ordered inputs. Both dice use both seeds and the
 * round; they are derived with different domain constants so one die does not
 * determine the other. The result is not symmetric in @p seedA and @p seedB:
 * both boards must pass the seeds in the same canonical order (the lower node
 * id's seed first).
 *
 * @param seedA Seed of the lower node id (canonical first seed).
 * @param seedB Seed of the higher node id.
 * @param round Round number.
 * @return Two faces, each in 1..kDiceFaces.
 */
inline DicePair computeDice(uint32_t seedA, uint32_t seedB,
                            uint16_t round) noexcept {
    const uint32_t salt = detail::roundKey(round);
    const uint32_t mixedB = mix32(seedB ^ salt);

    DicePair dice;
    dice.first = static_cast<uint8_t>(
        mix32(seedA ^ mixedB ^ detail::kFirstDieDomain) %
            static_cast<uint32_t>(kDiceFaces) +
        1u);
    dice.second = static_cast<uint8_t>(
        mix32(seedA ^ mixedB ^ detail::kSecondDieDomain) %
            static_cast<uint32_t>(kDiceFaces) +
        1u);
    return dice;
}

}  // namespace esp_now_dice
