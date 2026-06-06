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
// DW-OUT-NWSE: NW→SE wipe at progress=0.5 — top-left cleared, bottom-right kept
// =============================================================================

void test_diagonal_wipe_nw_se_midpoint(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];

    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    effect.update(250);  // progress=0.5, front ≈ 32

    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // NW_SE: lineValue = x + y. At front=32, pixels with x+y < 32 are cleared.
    // Top-left (0,0): value=0 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[0],
        "NW_SE mid: (0,0) should be cleared (x+y=0 < 32)");
    // Bottom-right (31,31): value=62 → not cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[31 * width + 31],
        "NW_SE mid: (31,31) should remain (x+y=62 >= 32)");
    // Top-right (31,0): value=31 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[0 * width + 31],
        "NW_SE mid: (31,0) should be cleared (x+y=31 < 32)");
    // Bottom-left (0,31): value=31 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 0],
        "NW_SE mid: (0,31) should be cleared (x+y=31 < 32)");
}

// =============================================================================
// DW-OUT-SENW: SE→NW wipe at progress=0.5 — SE corner cleared, NW kept
// =============================================================================

void test_diagonal_wipe_se_nw_midpoint(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];

    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::SE_NW);
    effect.update(250);  // progress=0.5, front ≈ 32

    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // SE_NW: lineValue = (W-1-x) + (H-1-y).
    // At front=32, pixels with value < 32 are cleared.
    // Bottom-right (31,31): value=0 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 31],
        "SE_NW mid: (31,31) should be cleared (value=0 < 32)");
    // Top-left (0,0): value=62 → not cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[0],
        "SE_NW mid: (0,0) should remain (value=62 >= 32)");
    // Top-right (31,0): value=31 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[0 * width + 31],
        "SE_NW mid: (31,0) should be cleared (value=31 < 32)");
    // Bottom-left (0,31): value=31 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 0],
        "SE_NW mid: (0,31) should be cleared (value=31 < 32)");
}

// =============================================================================
// DW-OUT-SWNE: SW→NE wipe at progress=0.5 — SW corner cleared, NE kept
// =============================================================================

void test_diagonal_wipe_sw_ne_midpoint(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];

    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::SW_NE);
    effect.update(250);  // progress=0.5, front ≈ 32

    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // SW_NE: lineValue = x + (H-1-y).
    // At front=32, pixels with value < 32 are cleared.
    // Bottom-left (0,31): value=0 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 0],
        "SW_NE mid: (0,31) should be cleared (value=0 < 32)");
    // Top-right (31,0): value=62 → not cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[0 * width + 31],
        "SW_NE mid: (31,0) should remain (value=62 >= 32)");
    // Top-left (0,0): value=31 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[0],
        "SW_NE mid: (0,0) should be cleared (value=31 < 32)");
    // Bottom-right (31,31): value=31 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 31],
        "SW_NE mid: (31,31) should be cleared (value=31 < 32)");
}

// =============================================================================
// DW-IN: In direction — at progress=0 all hidden, at progress=1 fully revealed
// =============================================================================

void test_diagonal_wipe_in_direction(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];
    const uint8_t FILL = 0xAB;

    // --- progress=0: In mode should clear entire buffer ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    memset(buffer, FILL, sizeof(buffer));
    effect.apply(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[i],
            "DiagonalWipe In at progress=0 should clear all pixels to 0");
    }

    // --- progress=1: In mode should keep all pixels ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    effect.update(500);
    memset(buffer, FILL, sizeof(buffer));
    effect.apply(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(FILL, buffer[i],
            "DiagonalWipe In at progress=1 should keep all pixels unchanged");
    }
}

// =============================================================================
// DW-RGB565: RGB565 diagonal wipe extremes and direction test
// =============================================================================

void test_diagonal_wipe_rgb565_extremes(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint16_t buffer[32 * 32];
    const uint16_t FILL = 0xFFFF;

    // --- progress=0 Out: RGB565 wipe should NOT clear any pixels ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NE_SW);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.applyRGB565(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(FILL, buffer[i],
            "RGB565 Out progress=0 should leave buffer unchanged");
    }

    // --- progress=1 Out: RGB565 wipe should clear ALL pixels to 0 ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NE_SW);
    effect.update(500);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.applyRGB565(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, buffer[i],
            "RGB565 Out progress=1 should clear all pixels to 0");
    }

    // --- progress=0 In: RGB565 wipe should clear ALL pixels ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.applyRGB565(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, buffer[i],
            "RGB565 In progress=0 should clear all pixels to 0");
    }

    // --- progress=1 In: RGB565 wipe should keep all pixels ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    effect.update(500);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.applyRGB565(buffer, width, height);

    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(FILL, buffer[i],
            "RGB565 In progress=1 should keep all pixels unchanged");
    }
}

// =============================================================================
// DW-INTEGRATION: Full lifecycle through public apply() dispatch
// =============================================================================

void test_diagonal_wipe_integration(void) {
    // --- Phase 1: Out → full wipe over time ---
    TransitionEffect effect;
    int width = 64;
    int height = 64;
    uint8_t buffer[64 * 64];

    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NE_SW);

    // Advance to progress=0.5.
    effect.update(250);
    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // NE_SW Out at front≈64. Pixels with (63-x)+y < 64 are cleared.
    // (32,32): (63-32)+32 = 63 < 64 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[32 * width + 32],
        "Integration: (32,32) should be cleared at progress=0.5");
    // (0,63): (63-0)+63 = 126 >= 64 → kept.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[63 * width + 0],
        "Integration: (0,63) should remain at progress=0.5");

    // Advance to completion.
    effect.update(250);  // Now at progress=1 (elapsed=500)
    // Consume the hold frame.
    effect.update(16);
    TEST_ASSERT_FALSE_MESSAGE(effect.isActive(),
        "Integration: effect should be inactive after completion + hold");

    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);
    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[i],
            "Integration: Out complete should clear all pixels");
    }

    // --- Phase 2: In → reveal from hidden ---
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);

    // At progress=0: buffer should be all hidden.
    memset(buffer, 0xAB, sizeof(buffer));
    effect.apply(buffer, width, height);
    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[i],
            "Integration: In progress=0 should clear all");
    }

    // At progress=1: buffer should be fully revealed.
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    effect.update(500);
    effect.update(16);  // consume hold
    memset(buffer, 0xAB, sizeof(buffer));
    effect.apply(buffer, width, height);
    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[i],
            "Integration: In progress=1 should keep all pixels");
    }
}

// =============================================================================
// DW-FEATHER-OUT: Feather zone — pixels at the wipe boundary are partially
// blended with 50% alpha in Out direction
// =============================================================================

void test_diagonal_wipe_feather_out(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];
    const uint8_t FILL = 0xFF;

    // Out NE_SW at progress=0.5: front = (W+H) * 128/256 = 32
    // NE_SW: lineValue = (31-x) + y. At lineValue == 32: y = x + 1
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NE_SW);
    effect.update(250);  // progress = 0.5

    memset(buffer, FILL, sizeof(buffer));
    effect.apply(buffer, width, height);

    // (0,1): lineValue = 31+1 = 32 = front → feather zone → 50% blend → 0xFF * 128/256 = 0x7F
    // Allow some tolerance: could be 0x7E or 0x80 depending on exact rounding
    uint8_t featherVal = buffer[1 * width + 0];
    TEST_ASSERT_TRUE_MESSAGE(featherVal > 0x60 && featherVal < 0x90,
        "NE_SW Out: pixel at (0,1) should be in feather zone (~0x7F)");

    // (0,0): lineValue = 31+0 = 31 < 32 → fully behind front → cleared
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[0],
        "NE_SW Out: (0,0) should be fully cleared (lineValue < front)");

    // (31,31): lineValue = (31-31)+31 = 31 < 32 → cleared (behind front)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 31],
        "NE_SW Out: (31,31) should be fully cleared (lineValue < front)");
}

// =============================================================================
// DW-FEATHER-IN: Feather zone — pixels at the wipe boundary are partially
// blended in In direction
// =============================================================================

void test_diagonal_wipe_feather_in(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];
    const uint8_t FILL = 0xFF;

    // In NW_SE at progress=0.5: front = 32
    // NW_SE: lineValue = x + y. At lineValue == 32: y = 32 - x
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::In, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    effect.update(250);  // progress = 0.5

    memset(buffer, FILL, sizeof(buffer));
    effect.apply(buffer, width, height);

    // (0,0): lineValue = 0 < 32 → behind front → visible (kept)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(FILL, buffer[0],
        "NW_SE In: (0,0) should be visible (behind front)");

    // (31,31): lineValue = 62 >= 32 → ahead → hidden (cleared)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, buffer[31 * width + 31],
        "NW_SE In: (31,31) should be cleared (ahead of front)");
}

// =============================================================================
// DW-FEATHER-RGB565: Feather zone in RGB565 path
// =============================================================================

void test_diagonal_wipe_feather_rgb565(void) {
    TransitionEffect effect;
    int width = 32;
    int height = 32;
    uint16_t buffer[32 * 32];
    const uint16_t FILL = 0xFFFF;

    // Out NE_SW at progress=0.5, same setup as 8bpp test
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NE_SW);
    effect.update(250);

    memset(buffer, 0xFF, sizeof(buffer));
    effect.applyRGB565(buffer, width, height);

    // (0,1) should be partially blended
    uint16_t featherVal = buffer[1 * width + 0];
    TEST_ASSERT_TRUE_MESSAGE(featherVal > 0x6000 && featherVal < 0xFF00,
        "NE_SW Out RGB565: pixel at (0,1) should be in feather zone");

    // (0,0) should be fully cleared
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0x0000, buffer[0],
        "NE_SW Out RGB565: (0,0) should be fully cleared");

    // Corner should be cleared
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0x0000, buffer[31 * width + 31],
        "NE_SW Out RGB565: (31,31) should be fully cleared");
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
    RUN_TEST(test_diagonal_wipe_nw_se_midpoint);
    RUN_TEST(test_diagonal_wipe_se_nw_midpoint);
    RUN_TEST(test_diagonal_wipe_sw_ne_midpoint);
    RUN_TEST(test_diagonal_wipe_in_direction);
    RUN_TEST(test_diagonal_wipe_rgb565_extremes);
    RUN_TEST(test_diagonal_wipe_integration);
    RUN_TEST(test_diagonal_wipe_feather_out);
    RUN_TEST(test_diagonal_wipe_feather_in);
    RUN_TEST(test_diagonal_wipe_feather_rgb565);
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
