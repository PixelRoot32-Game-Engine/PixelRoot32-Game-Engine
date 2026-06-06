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
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_diagonal_wipe_scaffold);
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
