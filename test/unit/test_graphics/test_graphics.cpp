/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unified test runner for test_graphics - includes all graphics tests
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Font5x7.h"
#include "graphics/FontManager.h"
#include "mocks/MockDrawSurface.h"

// Include test headers from parent directory
#include "test_graphics_primitives.h"
#include "test_renderer_draw.h"
#include "test_graphics_ownership.h"
#include "test_renderer_sprite1bpp.h"

using namespace pixelroot32::graphics;

// Global variables for test_graphics_primitives tests
MockDrawSurface* mockSurface = nullptr;
Renderer* renderer = nullptr;
int mock_surface_instances = 0;

bool wasDrawCalled(const std::string& type) {
    for (const auto& call : mockSurface->calls) {
        if (call.type == type) return true;
    }
    return false;
}

MockDrawSurface::DrawCall getLastCallOfType(const std::string& type) {
    for (auto it = mockSurface->calls.rbegin(); it != mockSurface->calls.rend(); ++it) {
        if (it->type == type) return *it;
    }
    return MockDrawSurface::DrawCall{"", 0, 0, 0, 0, 0, 0, 0, 0, ""};
}

void setUp(void) {
    test_setup();
    // Setup for test_graphics_primitives tests
    mockSurface = new MockDrawSurface();
    DisplayConfig config = pixelroot32::graphics::DisplayConfig::createCustom(mockSurface, 240, 240);
    renderer = new Renderer(config);
    renderer->setDisplaySize(240, 240);
    // Update counter for ownership tests reference
    mock_surface_instances = MockDrawSurface::instances;
    // Default font required by the 1bpp text-rendering parity test.
    FontManager::setDefaultFont(&FONT_5X7);
}

void tearDown(void) {
    delete renderer;
    renderer = nullptr;
    mockSurface = nullptr;  // Renderer owns and deletes the surface
    test_teardown();
}

int main() {
    UNITY_BEGIN();

    // Graphics Ownership Tests - Run first when instances counter is clean
    MockDrawSurface::instances = 0;
    mock_surface_instances = 0;
    RUN_TEST(test_display_config_ownership);
    MockDrawSurface::instances = 0;
    mock_surface_instances = 0;
    RUN_TEST(test_display_config_move_semantics);
    MockDrawSurface::instances = 0;
    mock_surface_instances = 0;
    RUN_TEST(test_renderer_ownership_transfer);

    // Graphics Primitives Tests
    RUN_TEST(test_draw_rectangle_basic);
    RUN_TEST(test_draw_rectangle_with_offset);
    RUN_TEST(test_draw_filled_rectangle);
    RUN_TEST(test_draw_rectangle_zero_size);
    RUN_TEST(test_draw_line_basic);
    RUN_TEST(test_draw_line_horizontal);
    RUN_TEST(test_draw_line_vertical);
    RUN_TEST(test_draw_line_with_offset);
    RUN_TEST(test_draw_circle_basic);
    RUN_TEST(test_draw_filled_circle);
    RUN_TEST(test_draw_circle_zero_radius);
    RUN_TEST(test_draw_circle_with_offset);
    RUN_TEST(test_set_display_offset);
    RUN_TEST(test_display_offset_affects_all_primitives);
    RUN_TEST(test_negative_display_offset);
    RUN_TEST(test_draw_with_color_red);
    RUN_TEST(test_draw_with_transparent_color);
    RUN_TEST(test_draw_outside_logical_bounds);
    RUN_TEST(test_draw_at_exact_bounds);
    RUN_TEST(test_draw_pixel_basic);
    RUN_TEST(test_draw_pixel_with_offset);
    RUN_TEST(test_horizontal_layout_with_spacing);
    RUN_TEST(test_vertical_layout_with_spacing);
    RUN_TEST(test_ui_component_state);
    RUN_TEST(test_ui_container_hierarchy);
    RUN_TEST(test_sibling_order_and_z_index);

    // Renderer Draw Tests
    RUN_TEST(test_renderer_initialization);
    RUN_TEST(test_renderer_draw_rectangle);
    RUN_TEST(test_renderer_draw_filled_rectangle);
    RUN_TEST(test_renderer_draw_pixel);
    RUN_TEST(test_renderer_draw_line);
    RUN_TEST(test_renderer_draw_circle);
    RUN_TEST(test_renderer_draw_filled_circle);
    RUN_TEST(test_renderer_set_display_size);
    RUN_TEST(test_renderer_set_display_offset);
    RUN_TEST(test_renderer_offset_bypass);
    RUN_TEST(test_renderer_draw_text);
    RUN_TEST(test_renderer_draw_text_centered);
    RUN_TEST(test_renderer_begin_end_frame);
    RUN_TEST(test_renderer_set_contrast);
    RUN_TEST(test_renderer_draw_bitmap);
    RUN_TEST(test_renderer_draw_filled_rectangle_w);

    // 1bpp Sprite Fast Path Tests
    RUN_TEST(test_sprite1bpp_fast_path_writes_framebuffer);
    RUN_TEST(test_sprite1bpp_fast_path_bypasses_draw_pixel);
    RUN_TEST(test_sprite1bpp_fast_path_flip_x);
    RUN_TEST(test_sprite1bpp_fast_path_skips_empty_rows);
    RUN_TEST(test_sprite1bpp_fast_path_respects_display_offset);
    RUN_TEST(test_sprite1bpp_fast_path_honours_offset_bypass);

    RUN_TEST(test_sprite1bpp_clips_left_edge);
    RUN_TEST(test_sprite1bpp_clips_right_edge);
    RUN_TEST(test_sprite1bpp_clips_top_edge);
    RUN_TEST(test_sprite1bpp_clips_bottom_edge);
    RUN_TEST(test_sprite1bpp_fully_offscreen_writes_nothing);

    RUN_TEST(test_sprite1bpp_fallback_uses_draw_pixel);
    RUN_TEST(test_sprite1bpp_branches_match_basic);
    RUN_TEST(test_sprite1bpp_branches_match_flip_x);
    RUN_TEST(test_sprite1bpp_branches_match_empty_rows);
    RUN_TEST(test_sprite1bpp_branches_match_all_edges);
    RUN_TEST(test_sprite1bpp_branches_match_wide_sprite);
    RUN_TEST(test_sprite1bpp_branches_match_text_rendering);

    // These were added in Phases 3-4 but never registered here (only
    // exercised via an uncommitted manual driver); registering them now
    // closes that gap so a real `pio test` run actually executes them.
    RUN_TEST(test_renderer_draw_text_ascii_golden_positions);
    RUN_TEST(test_renderer_draw_text_extended_glyph_shares_baseline_size1);
    RUN_TEST(test_renderer_draw_text_extended_glyph_shares_baseline_size2);
    RUN_TEST(test_renderer_draw_text_extended_glyph_clips_above_screen);

#if PIXELROOT32_ENABLE_FONT_LATIN1
    RUN_TEST(test_renderer_draw_text_latin1_n_tilde_bit_order_not_mirrored);
    RUN_TEST(test_renderer_draw_text_latin1_A_acute_distinguishable_from_A);
    RUN_TEST(test_renderer_draw_text_latin1_N_tilde_distinguishable_from_N);
    RUN_TEST(test_renderer_draw_text_latin1_i_acute_replaces_tittle_not_stacks);
#endif

    return UNITY_END();
}
