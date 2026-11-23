/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Phase 1 (H5): the native window used to be a fixed 2x of the panel, with the
 * same 2 written again in sendBuffer() and a third time implicit in the mouse
 * mapping. These tests pin the replacement: one layout computed from the actual
 * window size, and the input mapping being its exact inverse.
 *
 * No SDL here on purpose -- the geometry is pure arithmetic, so it is testable
 * headless and does not need a display to prove that a click lands where the
 * player clicked.
 */
#include <unity.h>

#include "drivers/native/WindowLayout.h"

using pixelroot32::drivers::native::WindowRect;
using pixelroot32::drivers::native::WindowLayout;
using pixelroot32::drivers::native::defaultWindowScale;
using pixelroot32::drivers::native::fitWindowLayout;
using pixelroot32::drivers::native::framebufferRect;
using pixelroot32::drivers::native::mapToFramebuffer;
using pixelroot32::drivers::native::kFallbackWindowScale;
using pixelroot32::drivers::native::kMaxDefaultWindowScale;

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// 1. An exact multiple fills the window with no letterbox.
// =============================================================================
void test_exact_fit_has_no_letterbox(void) {
    const WindowLayout layout = fitWindowLayout(480, 480, 240, 240);

    TEST_ASSERT_EQUAL_FLOAT(2.0f, layout.scale);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, layout.originX);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, layout.originY);
}

// =============================================================================
// 2. A window wider than the panel centres the panel (letterbox bars), instead
//    of pinning it to the top-left as the old hardcoded scale did.
// =============================================================================
void test_wide_window_centres_the_physical_area(void) {
    const WindowLayout layout = fitWindowLayout(800, 480, 240, 240);

    TEST_ASSERT_EQUAL_FLOAT(2.0f, layout.scale);
    TEST_ASSERT_EQUAL_FLOAT(160.0f, layout.originX);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, layout.originY);
}

// =============================================================================
// 3. Above 1x the scale is an integer: pixel art must not get rows of different
//    thickness. 500 / 240 = 2.08 fits 2 whole pixels per source pixel.
// =============================================================================
void test_scale_above_one_is_integer(void) {
    const WindowLayout layout = fitWindowLayout(500, 500, 240, 240);

    TEST_ASSERT_EQUAL_FLOAT(2.0f, layout.scale);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, layout.originX);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, layout.originY);
}

// =============================================================================
// 4. Below 1x the exact ratio is used: a shrunken window still shows the whole
//    framebuffer instead of flooring to 1 and overflowing.
// =============================================================================
void test_shrunk_window_uses_fractional_scale(void) {
    const WindowLayout layout = fitWindowLayout(120, 120, 240, 240);

    TEST_ASSERT_EQUAL_FLOAT(0.5f, layout.scale);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, layout.originX);
}

// =============================================================================
// 5. Degenerate sizes (no window yet, no panel) must not divide by zero.
// =============================================================================
void test_invalid_sizes_fall_back_to_one_to_one(void) {
    const WindowLayout zeroWindow = fitWindowLayout(0, 480, 240, 240);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, zeroWindow.scale);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, zeroWindow.originX);

    const WindowLayout zeroPanel = fitWindowLayout(480, 480, 0, 240);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, zeroPanel.scale);
}

// =============================================================================
// 6. The framebuffer rectangle carries the display offset as well as the
//    letterbox and the scale.
// =============================================================================
void test_framebuffer_rect_applies_offset_and_letterbox(void) {
    const WindowLayout layout = fitWindowLayout(800, 480, 240, 240);
    const WindowRect rect = framebufferRect(layout, 4, 6, 232, 228);

    TEST_ASSERT_EQUAL_INT(168, rect.x);   // 160 letterbox + 4 * 2 offset
    TEST_ASSERT_EQUAL_INT(12, rect.y);    //   0 letterbox + 6 * 2 offset
    TEST_ASSERT_EQUAL_INT(464, rect.width);
    TEST_ASSERT_EQUAL_INT(456, rect.height);
}

// =============================================================================
// 7. Presentation and input agree: the centre of the window maps to the centre
//    of the framebuffer at any scale, and the corners round-trip.
// =============================================================================
void test_window_point_maps_back_to_the_same_framebuffer_pixel(void) {
    const WindowLayout layout = fitWindowLayout(800, 480, 240, 240);

    int x = -1;
    int y = -1;
    mapToFramebuffer(layout, 400, 240, 0, 0, 240, 240, x, y);
    TEST_ASSERT_EQUAL_INT(120, x);
    TEST_ASSERT_EQUAL_INT(120, y);

    mapToFramebuffer(layout, 160, 0, 0, 0, 240, 240, x, y);
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(0, y);
}

// =============================================================================
// 8. Clicks in the letterbox bars are clamped into the framebuffer, so a drag
//    that ends outside the surface still delivers its release.
// =============================================================================
void test_clicks_outside_the_surface_are_clamped(void) {
    const WindowLayout layout = fitWindowLayout(800, 480, 240, 240);

    int x = -1;
    int y = -1;
    mapToFramebuffer(layout, 0, 0, 0, 0, 240, 240, x, y);
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(0, y);

    mapToFramebuffer(layout, 799, 479, 0, 0, 240, 240, x, y);
    TEST_ASSERT_EQUAL_INT(239, x);
    TEST_ASSERT_EQUAL_INT(239, y);
}

// =============================================================================
// 9. The default window size follows the screen instead of a fixed 2x.
// =============================================================================
void test_default_window_scale_follows_the_display(void) {
    // A 240x240 panel on a 1512x982 laptop: 2.4x of the usable height -> 2.
    TEST_ASSERT_EQUAL_INT(2, defaultWindowScale(240, 240, 1512, 982));

    // The same panel on a 1920x1080 desktop gets one step more.
    TEST_ASSERT_EQUAL_INT(3, defaultWindowScale(240, 240, 1920, 1080));

    // On a tiny display the window never becomes smaller than the panel.
    TEST_ASSERT_EQUAL_INT(1, defaultWindowScale(240, 240, 400, 300));

    // And on a huge one it stops at the cap rather than filling the screen.
    TEST_ASSERT_EQUAL_INT(kMaxDefaultWindowScale, defaultWindowScale(240, 240, 7680, 4320));
}

// =============================================================================
// 10. A display that cannot be queried (headless dummy driver, CI) keeps the
//     pre-H5 window size instead of collapsing to 1:1.
// =============================================================================
void test_unknown_display_keeps_the_historic_window_size(void) {
    TEST_ASSERT_EQUAL_INT(kFallbackWindowScale, defaultWindowScale(240, 240, 0, 0));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_exact_fit_has_no_letterbox);
    RUN_TEST(test_wide_window_centres_the_physical_area);
    RUN_TEST(test_scale_above_one_is_integer);
    RUN_TEST(test_shrunk_window_uses_fractional_scale);
    RUN_TEST(test_invalid_sizes_fall_back_to_one_to_one);
    RUN_TEST(test_framebuffer_rect_applies_offset_and_letterbox);
    RUN_TEST(test_window_point_maps_back_to_the_same_framebuffer_pixel);
    RUN_TEST(test_clicks_outside_the_surface_are_clamped);
    RUN_TEST(test_default_window_scale_follows_the_display);
    RUN_TEST(test_unknown_display_keeps_the_historic_window_size);
    return UNITY_END();
}
