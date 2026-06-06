/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file test_diagonal_wipe.cpp
 * @brief Unit tests for DiagonalWipe transition effect.
 *
 * Covers the DiagonalWipe enum value, WipeDirection configuration,
 * hold-frame isActive() semantics, smoothstepQ8 curve, and the
 * full diagonal-wipe pixel-level apply() pipeline.
 */

#include <unity.h>
#include <cstring>
#include "graphics/TransitionEffect.h"
#include "../../test_config.h"

#if PIXELROOT32_ENABLE_SCENE_TRANSITIONS

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// DW-SCAFFOLD: Verify test file is discovered and compilable
// =============================================================================

void test_diagonal_wipe_scaffold(void) {
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// DW-HOLDFRAME: isActive() should extend past duration boundary by holdFrames_
// =============================================================================

void test_diagonal_wipe_holdframe_keeps_active(void) {
    TransitionEffect effect;

    // Default holdFrames_ = 1, so isActive() should stay true for 1 extra tick
    // after elapsed reaches duration.
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);

    // Advance to exactly the duration boundary.
    effect.update(500);

    // RED expectation: isActive() should still be true because the hold frame
    // has not been consumed yet. This will FAIL until hold-frame logic is
    // implemented in the GREEN step (T-05).
    TEST_ASSERT_TRUE_MESSAGE(effect.isActive(),
        "isActive() should remain true at duration boundary (hold frame)");

    // Consume the hold frame.
    effect.update(16);

    // Now isActive() should be false.
    TEST_ASSERT_FALSE_MESSAGE(effect.isActive(),
        "isActive() should be false after consuming hold frame");
}

// =============================================================================
// DW-SMOOTHSTEP: Q8.8 smoothstep curve should ease at edges, linear at midpoint
// =============================================================================

void test_diagonal_wipe_smoothstep_q8(void) {
    // smoothstepQ8 uses Q8.8 format: t=0 → 0, t=256 → 256, t=128 → 128.
    // RED step: test will fail to link because smoothstepQ8 does not exist yet.
    // Expected values (exact) will be derived from the Q8.8 implementation:
    // smoothstep(t) = t * t * (768 - t) / 262144  (scaled for Q8.8)

    // Edge cases
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0,   smoothstepQ8(0),
        "smoothstepQ8(0) should be 0");

    // Quarter points — exact values depend on Q8.8 precision:
    // smoothstepQ8(64) ≈ 40
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(40,  smoothstepQ8(64),
        "smoothstepQ8(64) should be ~40 (approx)");

    // Midpoint — guaranteed by symmetry
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(128, smoothstepQ8(128),
        "smoothstepQ8(128) should be 128 (midpoint)");

    // Three-quarter point
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(216, smoothstepQ8(192),
        "smoothstepQ8(192) should be ~216 (approx)");

    // Full
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(256, smoothstepQ8(256),
        "smoothstepQ8(256) should be 256");
}

// =============================================================================
// DW-OUT-EXTR: NE→SW wipe at progress=0 (unchanged) and progress=1 (all cleared)
// =============================================================================

void test_diagonal_wipe_ne_sw_extremes(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];

    // --- progress=0: wipe should NOT clear any pixels ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[i],
            "DiagonalWipe Out at progress=0 should leave buffer unchanged");
    }

    // --- progress=1: wipe should clear ALL pixels to 0 ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.update(500);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[i],
            "DiagonalWipe Out at progress=1 should clear all pixels to 0");
    }
}

// =============================================================================
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_diagonal_wipe_scaffold);
    RUN_TEST(test_diagonal_wipe_holdframe_keeps_active);
    RUN_TEST(test_diagonal_wipe_smoothstep_q8);
    RUN_TEST(test_diagonal_wipe_ne_sw_extremes);
    return UNITY_END();
}

#else // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

void setUp(void) {}
void tearDown(void) {}

void test_diagonal_wipe_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_SCENE_TRANSITIONS not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_diagonal_wipe_disabled);
    return UNITY_END();
}

#endif
