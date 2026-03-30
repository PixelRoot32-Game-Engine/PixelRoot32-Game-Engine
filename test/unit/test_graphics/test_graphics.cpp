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
#include "mocks/MockDrawSurface.h"

// Include test headers from parent directory
#include "test_graphics_primitives.h"
#include "test_renderer_draw.h"
#include "test_graphics_ownership.h"

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

    return UNITY_END();
}
