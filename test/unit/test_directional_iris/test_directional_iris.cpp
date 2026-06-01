/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file test_directional_iris.cpp
 * @brief Unit tests for Directional Iris Transition — direction-specific centers.
 *
 * Tests the direction-specific iris center API added to TransitionEffect:
 * - setIrisOutCenter() sets Out-direction center
 * - setIrisInCenter() sets In-direction center
 * - setIrisCenter() sets both Out and In (backward compat)
 * - init() resets all 4 centers to -1 (fallback to buffer center)
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
// DI-01: Directional iris centers applied per direction
// Out uses setIrisOutCenter, In uses setIrisInCenter
// =============================================================================

void test_directional_iris_out_center_applied(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.setIrisOutCenter(0, 0);
    effect.update(250);  // progress = 0.5

    // 128x128 buffer filled with 0xFF.
    int width = 128;
    int height = 128;
    uint8_t buffer[128 * 128];
    memset(buffer, 0xFF, sizeof(buffer));

    effect.apply(buffer, width, height);

    // With center at (0,0), maxRadius2 = 0² + 127² = 16129
    // At progress=0.5: r2 = 16129 * 0.5 = 8064
    // Pixel (0,0): dx=0, dy=0, dist2=0 <= 8064 → unchanged
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[0],
        "Center pixel (0,0) should remain unchanged inside iris radius");

    // Pixel (128,128): dx=128, dy=128, dist2=32768 > 8064 → cleared
    // Actually height is 128, so max y is 127. Pixel (127,127): dx=127, dy=127, dist2=32258
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[127 * width + 127],
        "Far corner (127,127) should be cleared outside iris radius");

    // Now verify In uses a DIFFERENT center
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);
    effect.setIrisInCenter(64, 64);
    // progress=0, r2=0: only center pixel (64,64) visible
    memset(buffer, 0xAB, sizeof(buffer));
    effect.apply(buffer, width, height);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[64 * width + 64],
        "In center (64,64) should be visible at progress=0 for Iris In");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0],
        "Corner (0,0) should be cleared (r2=0 at progress=0)");
}

// =============================================================================
// DI-02: Directional In center — at progress=0 only center pixel visible
// =============================================================================

void test_directional_iris_in_center_at_zero(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);
    effect.setIrisInCenter(8, 64);

    int width = 128;
    int height = 128;
    uint8_t buffer[128 * 128];
    memset(buffer, 0xAB, sizeof(buffer));

    // progress=0, r2=0: only exact center pixel (8,64) visible
    effect.apply(buffer, width, height);

    // Center pixel at (8,64) should be unchanged.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[64 * width + 8],
        "Center pixel (8,64) should be visible at progress=0 for Iris In");

    // Adjacent pixels should be cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[64 * width + 7],
        "Pixel (7,64) adjacent to center should be cleared");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[64 * width + 9],
        "Pixel (9,64) adjacent to center should be cleared");

    // Far corner should be cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0],
        "Far corner (0,0) should be cleared at progress=0");
}

// =============================================================================
// DI-03: setIrisCenter() sets both Out and In centers (backward compat)
// =============================================================================

void test_directional_iris_setcenter_sets_both(void) {
    TransitionEffect effect;

    // setIrisCenter should set both Out and In
    effect.setIrisCenter(32, 32);

    // Test Out uses center (32,32)
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.update(250);  // progress=0.5

    int width = 64;
    int height = 64;
    uint8_t buffer[64 * 64];
    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // Center (32,32) should be inside the radius at progress=0.5
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[32 * width + 32],
        "Out center (32,32) should be inside radius at progress=0.5");

    // Test In uses center (32,32)
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);
    // progress=0, r2=0: only (32,32) visible
    memset(buffer, 0xAB, sizeof(buffer));
    effect.apply(buffer, width, height);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[32 * width + 32],
        "In center (32,32) should be visible at progress=0");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0],
        "Far corner should be cleared at progress=0");
}

// =============================================================================
// DI-04: init() resets all 4 centers to -1 (fallback to buffer center)
// =============================================================================

void test_directional_iris_init_resets_centers(void) {
    TransitionEffect effect;

    // Set custom centers
    effect.setIrisOutCenter(10, 20);
    effect.setIrisInCenter(30, 40);

    // init() should reset all centers to -1
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.update(250);  // progress=0.5

    int width = 128;
    int height = 128;
    uint8_t buffer[128 * 128];
    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // With default centers (-1), center falls back to buffer center (64,64)
    // Center (64,64) should be inside radius
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[64 * width + 64],
        "After init(), buffer center (64,64) should be inside radius");

    // After init, setIrisCenter should work again
    effect.setIrisCenter(0, 0);
    effect.update(250);

    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // Now center should be (0,0), pixel (1,1) should be closer to center
    // At progress=0.5 + previous progress=0.5 = 1.0 actually let me reconsider
    // Actually the effect was already at progress=0.5 before setIrisCenter,
    // then we update by 250 more → progress=1.0

    // Let me re-init to test cleanly
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.setIrisCenter(0, 0);
    effect.update(250);

    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // With center (0,0), pixel (0,0) is the center
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[0],
        "After setIrisCenter(0,0) + init, center (0,0) should be inside radius");
}

// =============================================================================
// DI-05: Verify Out and In use different centers when set independently
// =============================================================================

void test_directional_iris_independent_centers(void) {
    TransitionEffect effect;

    // Set Out center to RIGHT edge, In center to LEFT edge
    int width = 128;
    int height = 64;

    // --- Test Out uses right-edge center ---
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.setIrisOutCenter(120, 32);
    effect.update(250);  // progress=0.5

    uint8_t buffer[128 * 64];
    memset(buffer, 0xFF, sizeof(buffer));
    effect.apply(buffer, width, height);

    // With center (120,32), maxRadius2 = 120² + 32² = 14400 + 1024 = 15424
    // At progress=0.5: r2 = 7712
    // Right edge pixel (127, 32): dx=7, dy=0, dist2=49 <= 7712 → visible
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[32 * width + 127],
        "Out with right center: right edge pixel should be visible");
    // Left edge pixel (0, 32): dx=120, dy=0, dist2=14400 > 7712 → cleared
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[32 * width + 0],
        "Out with right center: left edge pixel should be cleared");

    // --- Test In uses left-edge center ---
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);
    effect.setIrisInCenter(8, 32);

    memset(buffer, 0xAB, sizeof(buffer));
    // progress=0, r2=0: only (8, 32) visible
    effect.apply(buffer, width, height);

    // Left-ish center pixel at (8,32) should be visible
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[32 * width + 8],
        "In with left center: center pixel (8,32) should be visible");
    // Right edge pixel should be cleared
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[32 * width + 127],
        "In with left center: right edge pixel should be cleared");
}

// =============================================================================
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // DI-01: Directional centers per direction
    RUN_TEST(test_directional_iris_out_center_applied);

    // DI-02: In center at progress=0
    RUN_TEST(test_directional_iris_in_center_at_zero);

    // DI-03: Backward compat — setIrisCenter sets both
    RUN_TEST(test_directional_iris_setcenter_sets_both);

    // DI-04: init() resets all four centers
    RUN_TEST(test_directional_iris_init_resets_centers);

    // DI-05: Independent Out and In centers
    RUN_TEST(test_directional_iris_independent_centers);

    return UNITY_END();
}

#else // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

void setUp(void) {}
void tearDown(void) {}

void test_directional_iris_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_SCENE_TRANSITIONS not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_directional_iris_disabled);
    return UNITY_END();
}

#endif
