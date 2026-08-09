/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file test_transition_effect.cpp
 * @brief Unit tests for TransitionEffect — Fade (palette LUT) and Iris (circle wipe).
 *
 * Targets the TransitionEffect class defined in TransitionEffect.h, covering:
 * - Fade LUT computation for both Out and In directions
 * - Fade pixel dimming/brightening at various progress points
 * - Fade lifecycle (init, active, expired)
 * - Iris close (pixels outside radius cleared)
 * - Iris open with custom center offset
 * - Null buffer safety guard
 * - Default (uninitialised) state
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
// Helper: fill a buffer with a known pattern value
// =============================================================================
static void fillBuffer(uint8_t* buf, int size, uint8_t val) {
    for (int i = 0; i < size; ++i) buf[i] = val;
}

// =============================================================================
// TE-01: Fade LUT init — Fade In at progress=0: LUT[0]=0, LUT[255]=0
// =============================================================================

void test_transition_fade_lut_init_in(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::In, 500);

    // Default state: elapsed=0 → progress=0
    uint8_t buffer[8] = {0, 1, 127, 128, 129, 200, 254, 255};
    effect.apply(buffer, 8, 1);

    // Fade In at progress=0: LUT[i] = i * 0 / 256 = 0 for all i
    for (int i = 0; i < 8; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[i],
            "Fade In at progress=0 should clear all pixels to 0");
    }
}

// =============================================================================
// TE-02: Fade LUT — Fade Out at progress=0: LUT[i] = i (no change)
// =============================================================================

void test_transition_fade_lut_init_out(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);

    uint8_t buffer[8] = {0, 1, 127, 128, 129, 200, 254, 255};
    uint8_t expected[8] = {0, 1, 127, 128, 129, 200, 254, 255};
    effect.apply(buffer, 8, 1);

    // Fade Out at progress=0: LUT[i] = i * 256 / 256 = i (identity)
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected, buffer, 8,
        "Fade Out at progress=0 should preserve all pixel values");
}

// =============================================================================
// TE-03: Fade at midpoint — values should be ~50% dimmer
// =============================================================================

void test_transition_fade_halfway_dims_values(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
    effect.update(250);  // progress = 0.5

    uint8_t buffer[4] = {100, 150, 200, 255};
    effect.apply(buffer, 4, 1);

    // The 8bpp buffer is RGB332 (RRRGGGBB), not an intensity ramp, so each
    // channel is scaled on its own. Scaling the packed byte instead would
    // carry bits across channel boundaries and rotate the hue.
    //
    // At progress=0.5 the factor is 128, i.e. half of each channel:
    //   100 = 011 001 00 -> 001 000 00 =  32
    //   150 = 100 101 10 -> 010 010 01 =  73
    //   200 = 110 010 00 -> 011 001 00 = 100
    //   255 = 111 111 11 -> 011 011 01 = 109
    TEST_ASSERT_EQUAL_UINT8(32,  buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(73,  buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(100, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(109, buffer[3]);
}

// =============================================================================
// TE-04: Fade complete — isActive= false, progress=1.0
// =============================================================================

void test_transition_fade_complete(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
    TEST_ASSERT_TRUE(effect.isActive());

    effect.update(500);  // elapsed = duration
    // Consume the hold frame so isActive() returns false.
    effect.update(16);
    TEST_ASSERT_FALSE(effect.isActive());
    TEST_ASSERT_FLOAT_EQUAL(1.0f, effect.getProgress());
}

// =============================================================================
// TE-05: Fade In — values should brighten from black to full
// =============================================================================

void test_transition_fade_in_brightens(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::In, 500);

    // At progress=0: all zero
    uint8_t buffer[4] = {100, 150, 200, 255};
    effect.apply(buffer, 4, 1);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);

    // At progress=0.5 each RGB332 channel is halved — same values as the
    // Out direction at the same factor (see TE-03).
    effect.init(TransitionType::Fade, TransitionDirection::In, 500);
    uint8_t buf2[4] = {100, 150, 200, 255};
    effect.update(250);
    effect.apply(buf2, 4, 1);
    TEST_ASSERT_EQUAL_UINT8(32,  buf2[0]);
    TEST_ASSERT_EQUAL_UINT8(73,  buf2[1]);
    TEST_ASSERT_EQUAL_UINT8(100, buf2[2]);
    TEST_ASSERT_EQUAL_UINT8(109, buf2[3]);

    // At progress=1.0: full brightness
    effect.update(500);
    uint8_t buf3[4] = {100, 150, 200, 255};
    effect.apply(buf3, 4, 1);
    TEST_ASSERT_EQUAL_UINT8(100, buf3[0]);
    TEST_ASSERT_EQUAL_UINT8(150, buf3[1]);
    TEST_ASSERT_EQUAL_UINT8(200, buf3[2]);
    TEST_ASSERT_EQUAL_UINT8(255, buf3[3]);
}

// =============================================================================
// TE-06: Fade progress monotonically increases
// =============================================================================

void test_transition_fade_progress_monotonic(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 1000);

    float p0 = effect.getProgress();
    effect.update(100);
    float p1 = effect.getProgress();
    effect.update(200);
    float p2 = effect.getProgress();
    effect.update(700);
    float p3 = effect.getProgress();

    TEST_ASSERT_FLOAT_EQUAL(0.0f, p0);
    TEST_ASSERT_FLOAT_EQUAL(0.1f, p1);
    TEST_ASSERT_FLOAT_EQUAL(0.3f, p2);
    TEST_ASSERT_FLOAT_EQUAL(1.0f, p3);
}

// =============================================================================
// TE-07: Iris close — pixels outside radius are cleared (Out direction)
// =============================================================================

void test_transition_iris_close_clears_outside(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.update(250);  // progress=0.5, r2 = maxRadius2 * 0.5

    // 128x128 buffer filled with 0xFF.
    int width = 128;
    int height = 128;
    uint8_t buffer[128 * 128];
    memset(buffer, 0xFF, sizeof(buffer));

    effect.apply(buffer, width, height);

    // Center is (64, 64). At progress=0.5: r2 = 8192 * 0.5 = 4096.
    // A pixel well inside the radius should be unchanged.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[64 * width + 64],
        "Center pixel should remain unchanged inside iris radius");

    // A pixel in a far corner should be cleared (dist² > 4096).
    // Corner (0,0): dx=64, dy=64, dist2=8192 > 4096 → cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0 * width + 0],
        "Corner pixel (0,0) should be cleared outside iris radius");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0 * width + 127],
        "Corner pixel (0,127) should be cleared outside iris radius");

    // Pixel exactly at radius boundary: (0, 64): dist2=64²+0²=4096 → NOT cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFF, buffer[64 * width + 0],
        "Pixel on radius boundary should remain unchanged");
}

// =============================================================================
// TE-08: Iris open with offset center — only center pixel visible at progress=0
// =============================================================================

void test_transition_iris_open_offset(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);
    effect.setIrisCenter(32, 32);

    // 64x64 buffer filled with 0xAB.
    int width = 64;
    int height = 64;
    uint8_t buffer[64 * 64];
    memset(buffer, 0xAB, sizeof(buffer));

    // progress=0, r2=0: only exact center pixel.
    effect.apply(buffer, width, height);

    // Center pixel at (32,32) should be unchanged.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[32 * width + 32],
        "Center pixel (32,32) should remain unchanged at progress=0 for Iris In");

    // Pixel adjacent to center should be cleared (dist2 > 0).
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[31 * width + 32],
        "Pixel (31,32) should be cleared (dist2 > 0 at progress=0)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[32 * width + 33],
        "Pixel (32,33) should be cleared (dist2 > 0 at progress=0)");
}

// =============================================================================
// TE-09: Iris expand over time — radius increases (In direction)
// =============================================================================

void test_transition_iris_expand_over_time(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);

    int width = 128;
    int height = 128;
    uint8_t buffer[128 * 128];

    // At progress=0: only center visible.
    memset(buffer, 0xAB, sizeof(buffer));
    effect.apply(buffer, width, height);
    // Far corner should be cleared.
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0],
        "Iris In at progress=0: corner should be cleared");

    // At progress=1: full image visible.
    effect.init(TransitionType::Iris, TransitionDirection::In, 500);
    effect.update(500);
    memset(buffer, 0xAB, sizeof(buffer));
    effect.apply(buffer, width, height);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xAB, buffer[0],
        "Iris In at progress=1: corner should be visible");
}

// =============================================================================
// TE-10: Null buffer guard — no crash on nullptr
// =============================================================================

void test_transition_null_buffer(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
    effect.update(250);

    // Should not crash or access invalid memory.
    effect.apply(nullptr, 128, 128);

    // Also test Iris with null buffer.
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.apply(nullptr, 128, 128);

    TEST_ASSERT_TRUE(true);  // Reached here = no crash.
}

// =============================================================================
// TE-11: Default (uninitialised) state
// =============================================================================

void test_transition_default_state(void) {
    TransitionEffect effect;
    TEST_ASSERT_FALSE(effect.isActive());
    TEST_ASSERT_FLOAT_EQUAL(1.0f, effect.getProgress());

    // apply() on uninitialised effect should be a safe no-op.
    uint8_t buf[4] = {42, 43, 44, 45};
    effect.apply(buf, 4, 1);
    TEST_ASSERT_EQUAL_UINT8(42, buf[0]);
}

// =============================================================================
// TE-12: Fade Out at progress=1 — all pixels should become 0
// =============================================================================

void test_transition_fade_out_complete_clears(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
    effect.update(500);  // complete

    uint8_t buffer[4] = {100, 150, 200, 255};
    effect.apply(buffer, 4, 1);

    // At progress=1: LUT[i] = i * 0 / 256 = 0 for all i
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[3]);
}

// =============================================================================
// TE-13: Iris Out at progress=1 — entire buffer cleared
// =============================================================================

void test_transition_iris_out_complete_clears(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);
    effect.update(500);  // complete → r2 = 0

    int width = 32;
    int height = 32;
    uint8_t buffer[32 * 32];
    memset(buffer, 0xFF, sizeof(buffer));

    effect.apply(buffer, width, height);

    // All pixels should be cleared.
    for (int i = 0; i < width * height; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[i],
            "Iris Out at progress=1 should clear all pixels");
    }
}

// =============================================================================
// TE-14B: Fade and Iris should NOT use sub-step accumulator
// =============================================================================

void test_transition_fade_no_substep(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);

    // Even if someone calls setSubStepMs, Fade should NOT quantize time.
    effect.setSubStepMs(16);

    // update(8) — should advance by 8ms (no sub-step for Fade).
    effect.update(8);
    TEST_ASSERT_FLOAT_EQUAL(8.0f / 500.0f, effect.getProgress());

    // update(50) — should advance by 50ms (no quantization).
    effect.update(50);
    TEST_ASSERT_FLOAT_EQUAL(58.0f / 500.0f, effect.getProgress());
}

void test_transition_iris_no_substep(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Iris, TransitionDirection::Out, 500);

    effect.setSubStepMs(16);

    // update(8) — should advance by 8ms (no sub-step for Iris).
    effect.update(8);
    TEST_ASSERT_FLOAT_EQUAL(8.0f / 500.0f, effect.getProgress());

    effect.update(50);
    TEST_ASSERT_FLOAT_EQUAL(58.0f / 500.0f, effect.getProgress());
}

// =============================================================================
// TE-14: Re-init an effect while active — should reset state
// =============================================================================

void test_transition_reinit_resets_state(void) {
    TransitionEffect effect;
    effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
    effect.update(250);
    TEST_ASSERT_TRUE(effect.isActive());

    // Re-init with different params.
    effect.init(TransitionType::Iris, TransitionDirection::In, 300);

    // State should be fresh.
    TEST_ASSERT_TRUE(effect.isActive());
    TEST_ASSERT_FLOAT_EQUAL(0.0f, effect.getProgress());

    uint8_t buffer[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    effect.apply(buffer, 8, 1);
    // Iris In at progress=0: r2 = 0 → only center pixel visible.
    // Center is (4, 0) — index 4 in a 1-row 8-col buffer.
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
}

// =============================================================================
// TE-15: Fade must never rotate hue on the 8bpp (RGB332) buffer
//
// This is the regression that shipped: the LUT scaled the packed byte as if it
// were an intensity, so bits carried across channel boundaries and a fading
// red walked through green. On hardware a brown cave turned blue mid-fade.
// A pure channel must stay pure at every step of the fade.
// =============================================================================

void test_transition_fade_8bpp_preserves_hue(void) {
    // RGB332 primaries: RRRGGGBB.
    const uint8_t kRed   = 0xE0;  // 111 000 00
    const uint8_t kGreen = 0x1C;  // 000 111 00
    const uint8_t kBlue  = 0x03;  // 000 000 11

    for (unsigned long elapsed = 0; elapsed <= 500; elapsed += 50) {
        TransitionEffect effect;
        effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
        if (elapsed > 0) effect.update(elapsed);

        uint8_t buffer[3] = {kRed, kGreen, kBlue};
        effect.apply(buffer, 3, 1);

        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0] & 0x1F,
            "Fading red must not bleed into the green or blue channels");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[1] & 0xE3,
            "Fading green must not bleed into the red or blue channels");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[2] & 0xFC,
            "Fading blue must not bleed into the red or green channels");
    }
}

// =============================================================================
// TE-16: Fade darkens monotonically per channel — never brighter than the source
// =============================================================================

void test_transition_fade_8bpp_never_brightens_a_channel(void) {
    for (unsigned long elapsed = 0; elapsed <= 500; elapsed += 50) {
        TransitionEffect effect;
        effect.init(TransitionType::Fade, TransitionDirection::Out, 500);
        if (elapsed > 0) effect.update(elapsed);

        uint8_t buffer[1] = {0xFF};  // white
        effect.apply(buffer, 1, 1);

        const uint8_t r = (uint8_t)((buffer[0] >> 5) & 0x07);
        const uint8_t g = (uint8_t)((buffer[0] >> 2) & 0x07);
        const uint8_t b = (uint8_t)(buffer[0] & 0x03);

        // White fades to grey: R and G track each other, B follows at its own
        // depth. No channel may exceed its source value.
        TEST_ASSERT_TRUE_MESSAGE(r <= 7, "red channel out of range");
        TEST_ASSERT_TRUE_MESSAGE(g <= 7, "green channel out of range");
        TEST_ASSERT_TRUE_MESSAGE(b <= 3, "blue channel out of range");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(r, g,
            "White must stay neutral through the fade: R and G share a depth");
    }
}

// =============================================================================
// TE-17: DiagonalWipe feather blends toward black without rotating hue
// =============================================================================

void test_transition_diagonal_wipe_feather_preserves_hue(void) {
    TransitionEffect effect;
    effect.init(TransitionType::DiagonalWipe, TransitionDirection::Out, 500);
    effect.setWipeDirection(WipeDirection::NW_SE);
    effect.update(250);  // progress = 0.5

    // NW_SE gives lineValue = x for a single row; front = (4+1)*128/256 = 2,
    // so x=2 is the one pixel in the feather band.
    uint8_t buffer[4] = {0xE0, 0xE0, 0xE0, 0xE0};  // pure red
    effect.apply(buffer, 4, 1);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[0], "behind the front is wiped");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buffer[1], "behind the front is wiped");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x60, buffer[2],
        "feather halves the red channel only: 111 000 00 -> 011 000 00");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xE0, buffer[3], "ahead of the front is untouched");
}

// =============================================================================
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // TE-01, TE-02 — LUT init
    RUN_TEST(test_transition_fade_lut_init_in);
    RUN_TEST(test_transition_fade_lut_init_out);

    // TE-03, TE-04, TE-05 — Fade lifecycle
    RUN_TEST(test_transition_fade_halfway_dims_values);
    RUN_TEST(test_transition_fade_complete);
    RUN_TEST(test_transition_fade_in_brightens);

    // TE-06 — Progress
    RUN_TEST(test_transition_fade_progress_monotonic);

    // TE-07, TE-08, TE-09 — Iris
    RUN_TEST(test_transition_iris_close_clears_outside);
    RUN_TEST(test_transition_iris_open_offset);
    RUN_TEST(test_transition_iris_expand_over_time);

    // TE-10, TE-11 — Safety guards
    RUN_TEST(test_transition_null_buffer);
    RUN_TEST(test_transition_default_state);

    // TE-12, TE-13, TE-14 — Edge cases
    RUN_TEST(test_transition_fade_out_complete_clears);
    RUN_TEST(test_transition_iris_out_complete_clears);
    RUN_TEST(test_transition_reinit_resets_state);

    // TE-14B — Sub-step isolation
    RUN_TEST(test_transition_fade_no_substep);
    RUN_TEST(test_transition_iris_no_substep);

    // TE-15, TE-16, TE-17 — RGB332 channel integrity on the 8bpp path
    RUN_TEST(test_transition_fade_8bpp_preserves_hue);
    RUN_TEST(test_transition_fade_8bpp_never_brightens_a_channel);
    RUN_TEST(test_transition_diagonal_wipe_feather_preserves_hue);

    return UNITY_END();
}

#else // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

void setUp(void) {}
void tearDown(void) {}

void test_transition_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_SCENE_TRANSITIONS not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_transition_disabled);
    return UNITY_END();
}

#endif
