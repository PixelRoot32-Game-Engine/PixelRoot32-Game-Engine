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
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_diagonal_wipe_scaffold);
    RUN_TEST(test_diagonal_wipe_holdframe_keeps_active);
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
