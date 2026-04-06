#pragma once

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "../../mocks/MockDrawSurface.h"
#include <memory>

using namespace pixelroot32::graphics;

// ============================================================================
// Renderer Drawing Tests
// ============================================================================

void test_renderer_initialization() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    TEST_ASSERT_EQUAL(240, renderer.getLogicalWidth());
    TEST_ASSERT_EQUAL(240, renderer.getLogicalHeight());
}

void test_renderer_draw_rectangle() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawRectangle(10, 20, 50, 30, Color::Red);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("rectangle"));
}

void test_renderer_draw_filled_rectangle() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawFilledRectangle(5, 10, 40, 20, Color::Blue);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_rectangle"));
}

void test_renderer_draw_pixel() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawPixel(100, 50, Color::Green);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("pixel"));
}

void test_renderer_draw_line() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawLine(0, 0, 100, 100, Color::White);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("line"));
}

void test_renderer_draw_circle() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawCircle(50, 50, 20, Color::Red);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("circle"));
}

void test_renderer_draw_filled_circle() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawFilledCircle(50, 50, 15, Color::Blue);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_circle"));
}

void test_renderer_set_display_size() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.setDisplaySize(320, 240);
    TEST_ASSERT_EQUAL(320, renderer.getLogicalWidth());
    TEST_ASSERT_EQUAL(240, renderer.getLogicalHeight());
}

void test_renderer_set_display_offset() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.setDisplayOffset(10, 20);
    TEST_ASSERT_EQUAL(10, renderer.getXOffset());
    TEST_ASSERT_EQUAL(20, renderer.getYOffset());
}

void test_renderer_offset_bypass() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    TEST_ASSERT_FALSE(renderer.isOffsetBypassEnabled());
    renderer.setOffsetBypass(true);
    TEST_ASSERT_TRUE(renderer.isOffsetBypassEnabled());
}

void test_renderer_draw_text() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawText("Test", 10, 10, Color::White, 1);
    TEST_ASSERT_TRUE(true);
}

void test_renderer_draw_text_centered() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawTextCentered("Center", 50, Color::White, 1);
    TEST_ASSERT_TRUE(true);
}

void test_renderer_begin_end_frame() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.beginFrame();
    renderer.endFrame();
    TEST_ASSERT_TRUE(true);
}

void test_renderer_set_contrast() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.setContrast(128);
    TEST_ASSERT_TRUE(true);
}

void test_renderer_draw_bitmap() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    uint8_t bitmap[4] = {0xFF, 0x81, 0x81, 0xFF};
    renderer.drawBitmap(10, 10, 2, 2, bitmap, Color::White);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("bitmap"));
}

void test_renderer_draw_filled_rectangle_w() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    renderer.drawFilledRectangleW(10, 10, 50, 30, 0xF800);
    TEST_ASSERT_TRUE(true);
}

void test_renderer_draw_filled_rectangle_w();
