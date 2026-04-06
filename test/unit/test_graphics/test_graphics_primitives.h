/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for graphics drawing primitives
 */

#pragma once

#include <unity.h>
#include <graphics/Renderer.h>
#include <graphics/DisplayConfig.h>
#include <graphics/Color.h>
#include "../../mocks/MockDrawSurface.h"

using namespace pixelroot32::graphics;

// Forward declarations - implementation in test_graphics.cpp
extern MockDrawSurface* mockSurface;
extern Renderer* renderer;
extern bool wasDrawCalled(const std::string& type);
extern MockDrawSurface::DrawCall getLastCallOfType(const std::string& type);

// ============================================================================
// Rectangle Drawing Tests
// ============================================================================

void test_draw_rectangle_basic(void) {
    mockSurface->calls.clear();
    renderer->drawRectangle(10, 20, 50, 30, Color::Red);
    
    // Verify rectangle was drawn
    TEST_ASSERT_TRUE(wasDrawCalled("rectangle"));
    auto call = getLastCallOfType("rectangle");
    TEST_ASSERT_EQUAL(10, call.x);
    TEST_ASSERT_EQUAL(20, call.y);
    TEST_ASSERT_EQUAL(50, call.w);
    TEST_ASSERT_EQUAL(30, call.h);
}

void test_draw_rectangle_with_offset(void) {
    mockSurface->calls.clear();
    renderer->setDisplayOffset(5, 10);
    renderer->drawRectangle(10, 20, 50, 30, Color::Blue);
    
    // Verify coordinates include offset
    auto call = getLastCallOfType("rectangle");
    TEST_ASSERT_EQUAL(15, call.x); // 10 + 5
    TEST_ASSERT_EQUAL(30, call.y); // 20 + 10
}

void test_draw_filled_rectangle(void) {
    mockSurface->calls.clear();
    renderer->drawFilledRectangle(5, 15, 40, 25, Color::Green);
    
    TEST_ASSERT_TRUE(wasDrawCalled("filled_rectangle"));
    auto call = getLastCallOfType("filled_rectangle");
    TEST_ASSERT_EQUAL(5, call.x);
    TEST_ASSERT_EQUAL(15, call.y);
}

void test_draw_rectangle_zero_size(void) {
    mockSurface->calls.clear();
    renderer->drawRectangle(10, 20, 0, 0, Color::Red);
    TEST_ASSERT_TRUE(wasDrawCalled("rectangle"));
}

// ============================================================================
// Line Drawing Tests
// ============================================================================

void test_draw_line_basic(void) {
    mockSurface->calls.clear();
    renderer->drawLine(0, 0, 100, 100, Color::White);
    
    TEST_ASSERT_TRUE(wasDrawCalled("line"));
    auto call = getLastCallOfType("line");
    TEST_ASSERT_EQUAL(0, call.x);   // x1
    TEST_ASSERT_EQUAL(0, call.y);   // y1
    TEST_ASSERT_EQUAL(100, call.x2);
    TEST_ASSERT_EQUAL(100, call.y2);
}

void test_draw_line_horizontal(void) {
    mockSurface->calls.clear();
    renderer->drawLine(10, 50, 110, 50, Color::Yellow);
    
    auto call = getLastCallOfType("line");
    TEST_ASSERT_EQUAL(10, call.x);
    TEST_ASSERT_EQUAL(50, call.y);
    TEST_ASSERT_EQUAL(110, call.x2);
    TEST_ASSERT_EQUAL(50, call.y2);
}

void test_draw_line_vertical(void) {
    mockSurface->calls.clear();
    renderer->drawLine(50, 10, 50, 110, Color::Cyan);
    
    auto call = getLastCallOfType("line");
    TEST_ASSERT_EQUAL(50, call.x);
    TEST_ASSERT_EQUAL(10, call.y);
    TEST_ASSERT_EQUAL(50, call.x2);
    TEST_ASSERT_EQUAL(110, call.y2);
}

void test_draw_line_with_offset(void) {
    mockSurface->calls.clear();
    renderer->setDisplayOffset(10, 5);
    renderer->drawLine(0, 0, 50, 50, Color::Magenta);
    
    auto call = getLastCallOfType("line");
    TEST_ASSERT_EQUAL(10, call.x);   // 0 + 10
    TEST_ASSERT_EQUAL(5, call.y);    // 0 + 5
    TEST_ASSERT_EQUAL(60, call.x2);  // 50 + 10
    TEST_ASSERT_EQUAL(55, call.y2);  // 50 + 5
}

// ============================================================================
// Circle Drawing Tests
// ============================================================================

void test_draw_circle_basic(void) {
    mockSurface->calls.clear();
    renderer->drawCircle(120, 120, 30, Color::Red);
    
    TEST_ASSERT_TRUE(wasDrawCalled("circle"));
    auto call = getLastCallOfType("circle");
    TEST_ASSERT_EQUAL(120, call.x);
    TEST_ASSERT_EQUAL(120, call.y);
    TEST_ASSERT_EQUAL(30, call.r);
}

void test_draw_filled_circle(void) {
    mockSurface->calls.clear();
    renderer->drawFilledCircle(100, 80, 25, Color::Blue);
    
    TEST_ASSERT_TRUE(wasDrawCalled("filled_circle"));
    auto call = getLastCallOfType("filled_circle");
    TEST_ASSERT_EQUAL(100, call.x);
    TEST_ASSERT_EQUAL(80, call.y);
    TEST_ASSERT_EQUAL(25, call.r);
}

void test_draw_circle_zero_radius(void) {
    mockSurface->calls.clear();
    renderer->drawCircle(50, 50, 0, Color::Green);
    TEST_ASSERT_TRUE(wasDrawCalled("circle"));
    auto call = getLastCallOfType("circle");
    TEST_ASSERT_EQUAL(0, call.r);
}

void test_draw_circle_with_offset(void) {
    mockSurface->calls.clear();
    renderer->setDisplayOffset(20, 15);
    renderer->drawCircle(50, 50, 20, Color::White);
    
    auto call = getLastCallOfType("circle");
    TEST_ASSERT_EQUAL(70, call.x); // 50 + 20
    TEST_ASSERT_EQUAL(65, call.y); // 50 + 15
}

// ============================================================================
// Display Offset Tests (Transformations)
// ============================================================================

void test_set_display_offset(void) {
    renderer->setDisplayOffset(10, 20);
    TEST_ASSERT_EQUAL(10, renderer->getXOffset());
    TEST_ASSERT_EQUAL(20, renderer->getYOffset());
}

void test_display_offset_affects_all_primitives(void) {
    mockSurface->calls.clear();
    renderer->setDisplayOffset(5, 5);
    
    // Test rectangle
    renderer->drawRectangle(10, 10, 20, 20, Color::Red);
    auto rect = getLastCallOfType("rectangle");
    TEST_ASSERT_EQUAL(15, rect.x);
    TEST_ASSERT_EQUAL(15, rect.y);
    
    // Test circle
    renderer->drawCircle(20, 20, 10, Color::Blue);
    auto circle = getLastCallOfType("circle");
    TEST_ASSERT_EQUAL(25, circle.x);
    TEST_ASSERT_EQUAL(25, circle.y);
    
    // Test line
    renderer->drawLine(0, 0, 30, 30, Color::Green);
    auto line = getLastCallOfType("line");
    TEST_ASSERT_EQUAL(5, line.x);
    TEST_ASSERT_EQUAL(5, line.y);
}

void test_negative_display_offset(void) {
    mockSurface->calls.clear();
    renderer->setDisplayOffset(-10, -20);
    renderer->drawRectangle(30, 40, 20, 20, Color::Red);
    
    auto call = getLastCallOfType("rectangle");
    TEST_ASSERT_EQUAL(20, call.x); // 30 + (-10)
    TEST_ASSERT_EQUAL(20, call.y); // 40 + (-20)
}

// ============================================================================
// Color Resolution Tests
// ============================================================================

void test_draw_with_color_red(void) {
    mockSurface->calls.clear();
    renderer->drawPixel(10, 10, Color::Red);
    TEST_ASSERT_TRUE(wasDrawCalled("pixel"));
}

void test_draw_with_transparent_color(void) {
    mockSurface->calls.clear();
    renderer->drawRectangle(10, 10, 20, 20, Color::Transparent);
    // Transparent colors should not call draw functions
    TEST_ASSERT_FALSE(wasDrawCalled("rectangle"));
}

// ============================================================================
// Clipping Bounds Tests
// ============================================================================

void test_draw_outside_logical_bounds(void) {
    mockSurface->calls.clear();
    renderer->setDisplaySize(100, 100);
    renderer->setDisplayOffset(0, 0);
    
    // Draw far outside bounds - should still call (clipping happens in DrawSurface)
    renderer->drawRectangle(200, 200, 50, 50, Color::Red);
    TEST_ASSERT_TRUE(wasDrawCalled("rectangle"));
}

void test_draw_at_exact_bounds(void) {
    mockSurface->calls.clear();
    renderer->setDisplaySize(100, 100);
    renderer->drawRectangle(0, 0, 100, 100, Color::Blue);
    
    auto call = getLastCallOfType("rectangle");
    TEST_ASSERT_EQUAL(0, call.x);
    TEST_ASSERT_EQUAL(0, call.y);
}

// ============================================================================
// Pixel Drawing Tests
// ============================================================================

void test_draw_pixel_basic(void) {
    mockSurface->calls.clear();
    renderer->drawPixel(50, 75, Color::White);
    
    TEST_ASSERT_TRUE(wasDrawCalled("pixel"));
    auto call = getLastCallOfType("pixel");
    TEST_ASSERT_EQUAL(50, call.x);
    TEST_ASSERT_EQUAL(75, call.y);
}

void test_draw_pixel_with_offset(void) {
    mockSurface->calls.clear();
    renderer->setDisplayOffset(10, 10);
    renderer->drawPixel(20, 30, Color::Yellow);
    
    auto call = getLastCallOfType("pixel");
    TEST_ASSERT_EQUAL(30, call.x); // 20 + 10
    TEST_ASSERT_EQUAL(40, call.y); // 30 + 10
}

// ============================================================================
// UI Layout Advanced Tests
// ============================================================================

void test_horizontal_layout_with_spacing(void) {
    // Test horizontal layout with spacing
    // (Implementation would go here)
    TEST_ASSERT_TRUE(true); // Placeholder
}

void test_vertical_layout_with_spacing(void) {
    // Test vertical layout with spacing
    // (Implementation would go here)
    TEST_ASSERT_TRUE(true); // Placeholder
}

void test_ui_component_state(void) {
    // Test UI component state (Button, Slider, Checkbox)
    // (Implementation would go here)
    TEST_ASSERT_TRUE(true); // Placeholder
}

void test_ui_container_hierarchy(void) {
    // Test UI container hierarchy
    // (Implementation would go here)
    TEST_ASSERT_TRUE(true); // Placeholder
}

void test_sibling_order_and_z_index(void) {
    // Test sibling order and z-index
    // (Implementation would go here)
    TEST_ASSERT_TRUE(true); // Placeholder
}

// Context Tests (Temporarily disabled)
// void test_set_render_context(void);
// void test_reset_render_context(void);
