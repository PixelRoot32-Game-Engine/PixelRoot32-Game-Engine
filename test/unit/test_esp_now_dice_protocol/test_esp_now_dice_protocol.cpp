/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the Dice Duel protocol math (examples/esp_now_dice/src/
 * DiceProtocol.h). The header is pure, platform-independent arithmetic, so the
 * suite runs unguarded under [env:native_test]: no networking, no Arduino, no
 * display.
 *
 * The tests pin behavior only: commitment round-trips, dice face ranges,
 * determinism, and the two diversity properties the game relies on (the round
 * and the seed order both reach the result). No timing assertion is made; the
 * last test only proves the functions survive thousands of calls.
 */
#include <unity.h>

#include <cstdint>

#include "../../../examples/esp_now_dice/src/DiceProtocol.h"

namespace {

using esp_now_dice::DicePair;
using esp_now_dice::computeDice;
using esp_now_dice::kDiceFaces;
using esp_now_dice::makeCommit;
using esp_now_dice::mix32;
using esp_now_dice::verifyCommit;

// Small deterministic LCG so the sample does not depend on <random>, the wall
// clock, or the implementation under test.
uint32_t nextSample(uint32_t &state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

bool sameDice(const DicePair &left, const DicePair &right) {
    return left.first == right.first && left.second == right.second;
}

}  // namespace

void setUp(void) {}
void tearDown(void) {}

// 1. mix32 is a stateless avalanche mix: deterministic, injective on the
//    sampled inputs, and never zero for a zero input.
void test_mix32_is_deterministic_and_distinct(void) {
    const uint32_t inputs[] = {0u,          1u,          2u,          3u,
                               7u,          12345u,      0x12345678u, 0x80000000u,
                               0xdeadbeefu, 0xffffffffu};
    const size_t inputCount = sizeof(inputs) / sizeof(inputs[0]);

    for (size_t i = 0; i < inputCount; ++i) {
        TEST_ASSERT_EQUAL_UINT32(mix32(inputs[i]), mix32(inputs[i]));
    }

    for (size_t i = 0; i < inputCount; ++i) {
        for (size_t j = i + 1; j < inputCount; ++j) {
            TEST_ASSERT_NOT_EQUAL_UINT32(mix32(inputs[i]), mix32(inputs[j]));
        }
    }

    TEST_ASSERT_NOT_EQUAL_UINT32(0u, mix32(0u));
}

// 2. makeCommit/verifyCommit agree for many seed/round pairs.
void test_commit_round_trips(void) {
    const uint32_t seeds[] = {0u, 1u, 0x12345678u, 0xdeadbeefu, 0xffffffffu};
    const uint16_t rounds[] = {0u, 1u, 2u, 7u, 1000u, 65535u};
    const size_t seedCount = sizeof(seeds) / sizeof(seeds[0]);
    const size_t roundCount = sizeof(rounds) / sizeof(rounds[0]);

    for (size_t i = 0; i < seedCount; ++i) {
        for (size_t j = 0; j < roundCount; ++j) {
            const uint32_t commit = makeCommit(seeds[i], rounds[j]);
            TEST_ASSERT_TRUE(verifyCommit(seeds[i], rounds[j], commit));
        }
    }
}

// 2. A commitment for a different seed or a different round is rejected.
void test_commit_rejects_different_seed_or_round(void) {
    const uint32_t seeds[] = {0u, 1u, 0x12345678u, 0xdeadbeefu, 0xffffffffu};
    const uint16_t rounds[] = {0u, 1u, 2u, 7u, 1000u, 65535u};
    const size_t seedCount = sizeof(seeds) / sizeof(seeds[0]);
    const size_t roundCount = sizeof(rounds) / sizeof(rounds[0]);

    for (size_t i = 0; i < seedCount; ++i) {
        for (size_t j = 0; j < roundCount; ++j) {
            const uint32_t commit = makeCommit(seeds[i], rounds[j]);
            TEST_ASSERT_FALSE(
                verifyCommit(seeds[i] + 1u, rounds[j], commit));
            TEST_ASSERT_FALSE(
                verifyCommit(seeds[i], static_cast<uint16_t>(rounds[j] + 1u),
                             commit));
        }
    }
}

// 2. A single flipped commit bit is rejected.
void test_commit_rejects_bit_flipped_commit(void) {
    const uint32_t commit = makeCommit(0xdeadbeefu, 7u);

    TEST_ASSERT_FALSE(verifyCommit(0xdeadbeefu, 7u, commit ^ 0x00000001u));
    TEST_ASSERT_FALSE(verifyCommit(0xdeadbeefu, 7u, commit ^ 0x80000000u));
    TEST_ASSERT_FALSE(verifyCommit(0xdeadbeefu, 7u, commit ^ 0xffffffffu));
}

// 3. Over 10,000 seed pairs both faces stay in 1..6, both faces take more than
//    one value, and every one of the 36 combinations appears.
void test_compute_dice_faces_are_in_range_and_cover_all_combinations(void) {
    bool seen[6][6] = {};
    bool firstSeen[7] = {};
    bool secondSeen[7] = {};
    uint32_t state = 0x12345678u;
    int combinations = 0;

    TEST_ASSERT_EQUAL_UINT8(6u, kDiceFaces);

    for (uint32_t i = 0; i < 10000u; ++i) {
        const uint32_t seedA = nextSample(state);
        const uint32_t seedB = nextSample(state) ^ i;
        const uint16_t round = static_cast<uint16_t>(i & 0xffffu);
        const DicePair dice = computeDice(seedA, seedB, round);

        TEST_ASSERT_TRUE(dice.first >= 1u && dice.first <= kDiceFaces);
        TEST_ASSERT_TRUE(dice.second >= 1u && dice.second <= kDiceFaces);
        firstSeen[dice.first] = true;
        secondSeen[dice.second] = true;

        bool &cell = seen[dice.first - 1u][dice.second - 1u];
        if (!cell) {
            cell = true;
            ++combinations;
        }
    }

    int firstDistinct = 0;
    int secondDistinct = 0;
    for (int face = 1; face <= kDiceFaces; ++face) {
        if (firstSeen[face]) {
            ++firstDistinct;
        }
        if (secondSeen[face]) {
            ++secondDistinct;
        }
    }

    TEST_ASSERT_TRUE(firstDistinct > 1);
    TEST_ASSERT_TRUE(secondDistinct > 1);
    TEST_ASSERT_EQUAL_INT(36, combinations);
}

// 4. The same ordered inputs always produce the same pair.
void test_compute_dice_is_deterministic(void) {
    const uint32_t seedsA[] = {0u, 1u, 0xdeadbeefu, 0xffffffffu};
    const uint32_t seedsB[] = {0u, 7u, 0x12345678u, 0x80000000u};

    for (size_t i = 0; i < sizeof(seedsA) / sizeof(seedsA[0]); ++i) {
        for (uint16_t round = 0; round < 8u; ++round) {
            const DicePair first = computeDice(seedsA[i], seedsB[i], round);
            const DicePair second = computeDice(seedsA[i], seedsB[i], round);
            TEST_ASSERT_EQUAL_UINT8(first.first, second.first);
            TEST_ASSERT_EQUAL_UINT8(first.second, second.second);
        }
    }
}

// 4. Changing the round changes the pair for at least one round in the sample.
void test_round_changes_dice_for_a_sample(void) {
    const uint32_t seedA = 0xdeadbeefu;
    const uint32_t seedB = 0x12345678u;
    const DicePair baseline = computeDice(seedA, seedB, 0u);
    int differences = 0;

    for (uint16_t round = 1; round < 64u; ++round) {
        const DicePair dice = computeDice(seedA, seedB, round);
        if (!sameDice(baseline, dice)) {
            ++differences;
        }
    }

    TEST_ASSERT_GREATER_THAN_INT(0, differences);
}

// 4. Swapping the seed order changes the pair for at least one pair in the
//    sample. Both boards must therefore agree on the canonical order.
void test_swapping_seeds_changes_dice_for_a_sample(void) {
    uint32_t state = 0xabcdef01u;
    int differences = 0;

    for (int i = 0; i < 64; ++i) {
        const uint32_t seedA = nextSample(state);
        const uint32_t seedB = nextSample(state);
        const DicePair forward = computeDice(seedA, seedB, 17u);
        const DicePair reverse = computeDice(seedB, seedA, 17u);
        if (!sameDice(forward, reverse)) {
            ++differences;
        }
    }

    TEST_ASSERT_GREATER_THAN_INT(0, differences);
}

// 5. The functions are cheap enough for thousands of calls. No timing
//    assertion: the checksum only proves the calls actually ran.
void test_compute_dice_survives_many_calls(void) {
    uint32_t checksum = 0;

    for (uint32_t i = 0; i < 100000u; ++i) {
        const DicePair dice =
            computeDice(i, i * 2654435761u, static_cast<uint16_t>(i));
        checksum += static_cast<uint32_t>(dice.first) + dice.second;
    }

    TEST_ASSERT_TRUE(checksum > 0u);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_mix32_is_deterministic_and_distinct);
    RUN_TEST(test_commit_round_trips);
    RUN_TEST(test_commit_rejects_different_seed_or_round);
    RUN_TEST(test_commit_rejects_bit_flipped_commit);
    RUN_TEST(test_compute_dice_faces_are_in_range_and_cover_all_combinations);
    RUN_TEST(test_compute_dice_is_deterministic);
    RUN_TEST(test_round_changes_dice_for_a_sample);
    RUN_TEST(test_swapping_seeds_changes_dice_for_a_sample);
    RUN_TEST(test_compute_dice_survives_many_calls);
    return UNITY_END();
}
