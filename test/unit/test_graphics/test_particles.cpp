/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/particles/ParticleEmitter.h"
#include "graphics/Renderer.h"
#include "graphics/FontManager.h"
#include "core/Engine.h"
#include "../../mocks/MockDrawSurface.h"
#include <memory>

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::particles;
using namespace pixelroot32::math;

int MockDrawSurface::instances = 0;

// Note: Global engine instance is provided by MockEngineInstance.cpp during unit tests.
// Individual tests should not define their own 'engine' symbol.

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_particle_emitter_initialization(void) {
    ParticleConfig cfg = {
        Color::White, Color::Red,
        1.0f, 2.0f,
        0.1f, 0.95f,
        10, 20,
        true,
        0.0f, 360.0f
    };
    
    ParticleEmitter emitter({100, 100}, cfg);
    
    TEST_ASSERT_EQUAL_FLOAT(100, emitter.position.x);
    TEST_ASSERT_EQUAL_FLOAT(100, emitter.position.y);
}

void test_particle_burst(void) {
    ParticleConfig cfg = {
        Color::White, Color::Red,
        1.0f, 2.0f,
        0.1f, 0.95f,
        10, 20,
        true,
        0.0f, 360.0f
    };
    
    ParticleEmitter emitter(Vector2::ZERO(), cfg);
    emitter.burst({50, 50}, 10);
    
    // We can't directly inspect private particles array, 
    // but we can check if draw() produces calls.
    auto mock = std::make_unique<MockDrawSurface>();
    // Keep raw pointer for assertions since DisplayConfig takes ownership
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig dcfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(dcfg);
    
    // For now, let's just test that update() runs without crashing
    emitter.update(16);
    emitter.draw(renderer);
    
    // Check if any draw calls were recorded
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_particle_emitter_lifecycle(void) {
    ParticleConfig cfg = {
        Color::White, Color::Red,
        1.0f, 2.0f,
        0.1f, 0.95f,
        2, 2, // Very short life
        true,
        0.0f, 0.0f // All going right
    };
    
    ParticleEmitter emitter(Vector2::ZERO(), cfg);
    emitter.burst(Vector2::ZERO(), 1);
    
    // Update multiple times
    emitter.update(16);
    emitter.update(16);
    emitter.update(16);
    
    // After 3 updates of 16ms, with life=2 frames, particle should be dead
}

// =============================================================================
// Renderer Drawing Primitives Tests
// =============================================================================

void test_renderer_draw_filled_rectangle(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawFilledRectangle(10, 20, 30, 40, Color::Red);
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_rectangle"));
}

void test_renderer_draw_rectangle(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawRectangle(5, 5, 50, 50, Color::Blue);
    TEST_ASSERT_TRUE(mockRaw->hasCall("rectangle"));
}

void test_renderer_draw_filled_circle(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawFilledCircle(100, 100, 25, Color::Green);
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_circle"));
}

void test_renderer_draw_circle(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawCircle(50, 50, 10, Color::Yellow);
    TEST_ASSERT_TRUE(mockRaw->hasCall("circle"));
}

void test_renderer_draw_line(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawLine(0, 0, 100, 100, Color::White);
    TEST_ASSERT_TRUE(mockRaw->hasCall("line"));
}

void test_renderer_draw_pixel(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawPixel(50, 50, Color::Red);
    TEST_ASSERT_TRUE(mockRaw->hasCall("pixel"));
}

void test_renderer_transparent_not_drawn(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Transparent color should not produce any draw calls
    renderer.drawFilledRectangle(10, 10, 50, 50, Color::Transparent);
    renderer.drawRectangle(10, 10, 50, 50, Color::Transparent);
    renderer.drawCircle(50, 50, 10, Color::Transparent);
    renderer.drawFilledCircle(50, 50, 10, Color::Transparent);
    renderer.drawLine(0, 0, 100, 100, Color::Transparent);
    renderer.drawPixel(50, 50, Color::Transparent);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_offset(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.setDisplayOffset(10, 20);
    renderer.drawPixel(5, 5, Color::White);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    // Pixel should be at 15, 25
    TEST_ASSERT_EQUAL(15, mockRaw->calls[0].x);
    TEST_ASSERT_EQUAL(25, mockRaw->calls[0].y);
}

void test_renderer_offset_bypass(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.setDisplayOffset(10, 20);
    renderer.setOffsetBypass(true);
    renderer.drawPixel(5, 5, Color::White);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    // With bypass, pixel should be at 5, 5 (not offset)
    TEST_ASSERT_EQUAL(5, mockRaw->calls[0].x);
    TEST_ASSERT_EQUAL(5, mockRaw->calls[0].y);
    
    renderer.setOffsetBypass(false);
}

void test_renderer_begin_end_frame(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Should not crash
    renderer.beginFrame();
    renderer.drawPixel(0, 0, Color::White);
    renderer.endFrame();
    // beginFrame clears buffer, but we draw after so pixel should exist
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_draw_sprite_basic(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Create a simple 4x4 sprite with some bits set
    uint16_t data[] = { 0x000F, 0x000F, 0x000F, 0x000F };
    Sprite sprite = { data, 4, 4 };
    
    renderer.drawSprite(sprite, 10, 10, Color::White, false);
    // Should have drawn some pixels
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    TEST_ASSERT_TRUE(mockRaw->hasCall("pixel"));
}

void test_renderer_draw_sprite_flipx(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t data[] = { 0x0001, 0x0000, 0x0000, 0x0000 };
    Sprite sprite = { data, 4, 4 };
    
    renderer.drawSprite(sprite, 10, 10, Color::Red, true);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_draw_sprite_scaled(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t data[] = { 0x000F, 0x000F };
    Sprite sprite = { data, 4, 2 };
    
    renderer.drawSprite(sprite, 10, 10, 2.0f, 2.0f, Color::White, false);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_draw_sprite_null_data(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    Sprite sprite = { nullptr, 4, 4 };
    renderer.drawSprite(sprite, 10, 10, Color::White, false);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_draw_bitmap(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint8_t bitmap[] = { 0xFF, 0xFF };
    renderer.drawBitmap(10, 10, 8, 2, bitmap, Color::White);
    TEST_ASSERT_TRUE(mockRaw->hasCall("bitmap"));
}

void test_renderer_draw_text_basic(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // drawText with empty string should not crash
    renderer.drawText("", 10, 10, Color::White, 1);
    
    // drawText with transparent should not crash
    renderer.drawText("Hello", 10, 10, Color::Transparent, 1);
}

void test_renderer_draw_text_centered(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Should not crash even with empty text or transparent
    renderer.drawTextCentered("", 100, Color::White, 1);
    renderer.drawTextCentered("Test", 100, Color::Transparent, 1);
}

void test_renderer_init(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // init should not crash
    renderer.init();
    TEST_ASSERT_EQUAL(240, renderer.getLogicalWidth());
    TEST_ASSERT_EQUAL(240, renderer.getLogicalHeight());
}

void test_renderer_set_render_context(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    PaletteContext ctx = PaletteContext::Background;
    renderer.setRenderContext(&ctx);
    renderer.drawPixel(50, 50, Color::White);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    
    renderer.setRenderContext(nullptr);
}

void test_renderer_multisprite(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Test MultiSprite with null layers → early return
    MultiSprite ms = { 8, 8, nullptr, 0 };
    renderer.drawMultiSprite(ms, 0, 0);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Test MultiSprite with valid data
    uint16_t data[] = { 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F };
    SpriteLayer layers[1] = { { data, Color::White } };
    MultiSprite ms2 = { 4, 8, layers, 1 };
    renderer.drawMultiSprite(ms2, 0, 0);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_multisprite_scaled(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Test scaled MultiSprite with null layers → early return
    MultiSprite ms = { 8, 8, nullptr, 0 };
    renderer.drawMultiSprite(ms, 0, 0, 2.0f, 2.0f);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_draw_filledrectangle_w(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.drawFilledRectangleW(10, 20, 30, 40, 0xFFFF);
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_rectangle"));
}

// =============================================================================
// Renderer TileMap Tests
// =============================================================================

void test_renderer_tilemap_1bpp_basic(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Create a simple 4x4 tile
    uint16_t tileData[] = { 0x000F, 0x000F, 0x000F, 0x000F };
    Sprite tiles[2] = {
        { nullptr, 4, 4 },         // tile 0 (empty/skip)
        { tileData, 4, 4 }         // tile 1
    };
    
    // Create a 2x2 tilemap with one tile used
    uint8_t indices[] = { 1, 0, 0, 1 };
    TileMap map = { indices, 2, 2, tiles, 4, 4, 2 };
    
    renderer.drawTileMap(map, 0, 0, Color::White);
    // Should have drawn pixels for the two tiles that use index 1
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    TEST_ASSERT_TRUE(mockRaw->hasCall("pixel"));
}

void test_renderer_tilemap_null_data(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Null indices
    TileMap map1 = { nullptr, 2, 2, nullptr, 4, 4, 2 };
    renderer.drawTileMap(map1, 0, 0, Color::White);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Zero dimensions
    uint8_t indices[] = { 1 };
    uint16_t tileData[] = { 0x000F };
    Sprite tiles[2] = { { nullptr, 4, 1 }, { tileData, 4, 1 } };
    TileMap map2 = { indices, 0, 0, tiles, 4, 4, 2 };
    renderer.drawTileMap(map2, 0, 0, Color::White);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Zero tile dimensions
    TileMap map3 = { indices, 1, 1, tiles, 0, 0, 2 };
    renderer.drawTileMap(map3, 0, 0, Color::White);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Zero tileCount
    TileMap map4 = { indices, 1, 1, tiles, 4, 4, 0 };
    renderer.drawTileMap(map4, 0, 0, Color::White);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_tilemap_out_of_bounds_index(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t tileData[] = { 0x000F };
    Sprite tiles[2] = { { nullptr, 4, 1 }, { tileData, 4, 1 } };
    
    // Index 5 is >= tileCount(2), should be skipped
    uint8_t indices[] = { 5, 0, 0, 0 };
    TileMap map = { indices, 2, 2, tiles, 4, 1, 2 };
    
    renderer.drawTileMap(map, 0, 0, Color::White);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_tilemap_with_offset(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t tileData[] = { 0x0001 };
    Sprite tiles[2] = { { nullptr, 1, 1 }, { tileData, 1, 1 } };
    uint8_t indices[] = { 1 };
    TileMap map = { indices, 1, 1, tiles, 1, 1, 2 };
    
    // Draw with a display offset
    renderer.setDisplayOffset(50, 50);
    renderer.drawTileMap(map, 10, 10, Color::White);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    
    renderer.setDisplayOffset(0, 0);
}

void test_renderer_tilemap_viewport_culling(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t tileData[] = { 0x0001 };
    Sprite tiles[2] = { { nullptr, 1, 1 }, { tileData, 1, 1 } };
    uint8_t indices[] = { 1 };
    TileMap map = { indices, 1, 1, tiles, 8, 8, 2 };
    
    // Negative origin — partially culled
    renderer.drawTileMap(map, -4, -4, Color::White);
    // The tile at (0,0) starts at (-4,-4), which is partially off-screen;
    // some pixels might or might not appear depending on tile size vs screen
}

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
void test_renderer_draw_sprite_2bpp(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    static const Color palette2[] = { Color::Black, Color::White };
    static const uint8_t data2bpp[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    Sprite2bpp sprite2 = { data2bpp, palette2, 8, 4, 2 };
    renderer.drawSprite(sprite2, 10, 10, false);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}
#endif

#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)
void test_renderer_draw_sprite_4bpp(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    static const Color palette4[] = { Color::Black, Color::White, Color::Red, Color::Blue };
    static const uint8_t data4bpp[] = { 0x01, 0x00 };
    Sprite4bpp sprite4 = { data4bpp, palette4, 2, 1, 2 };
    renderer.drawSprite(sprite4, 5, 5, false);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}
#endif

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
void test_renderer_draw_tilemap_2bpp(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    static const Color pal[] = { Color::Black, Color::White };
    static const uint8_t tileData[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const Sprite2bpp tiles2[] = { { nullptr, nullptr, 0, 0, 0 }, { tileData, pal, 8, 4, 2 } };
    uint8_t indices[] = { 1 };
    TileMap2bpp map = { indices, 1, 1, tiles2, 8, 4, 2 };
    renderer.drawTileMap(map, 0, 0);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}
#endif

#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)
void test_renderer_draw_tilemap_4bpp(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    static const Color pal[] = { Color::Black, Color::White };
    static const uint8_t tileData[] = { 0x01, 0x00 };
    static const Sprite4bpp tiles4[] = { { nullptr, nullptr, 0, 0, 0 }, { tileData, pal, 2, 1, 2 } };
    uint8_t indices[] = { 1 };
    TileMap4bpp map = { indices, 1, 1, tiles4, 2, 1, 2 };
    renderer.drawTileMap(map, 0, 0);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}
#endif

// =============================================================================
// Renderer Text with Font Tests
// =============================================================================

void test_renderer_draw_text_with_font(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Create a minimal font with one glyph for character 'A' (ASCII 65)
    uint16_t glyphData[] = { 0x0006, 0x0009, 0x000F, 0x0009, 0x0009 };
    Sprite glyphs[1] = { { glyphData, 4, 5 } };
    Font testFont = { glyphs, 'A', 'A', 4, 5, 1, 6 };
    
    renderer.drawText("A", 10, 10, Color::White, 1, &testFont);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    TEST_ASSERT_TRUE(mockRaw->hasCall("pixel"));
}

void test_renderer_draw_text_scaled_with_font(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t glyphData[] = { 0x0006, 0x0009, 0x000F, 0x0009, 0x0009 };
    Sprite glyphs[1] = { { glyphData, 4, 5 } };
    Font testFont = { glyphs, 'A', 'A', 4, 5, 1, 6 };
    
    // Size > 1 should use scaled drawing path
    renderer.drawText("A", 10, 10, Color::White, 2, &testFont);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_draw_text_unsupported_char(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t glyphData[] = { 0x000F };
    Sprite glyphs[1] = { { glyphData, 4, 1 } };
    Font testFont = { glyphs, 'A', 'A', 4, 1, 1, 2 };
    
    // 'Z' is not in range [A, A] → should skip (glyphIndex == 255)
    renderer.drawText("Z", 10, 10, Color::White, 1, &testFont);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_draw_text_centered_with_font(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t glyphData[] = { 0x0006, 0x0009, 0x000F, 0x0009, 0x0009 };
    Sprite glyphs[1] = { { glyphData, 4, 5 } };
    Font testFont = { glyphs, 'A', 'A', 4, 5, 1, 6 };
    
    renderer.drawTextCentered("A", 100, Color::White, 1, &testFont);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_draw_text_null_font_no_default(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Clear default font to test the null font path
    FontManager::setDefaultFont(nullptr);
    
    renderer.drawText("Hello", 10, 10, Color::White, 1, nullptr);
    // Should return early (no font available)
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    renderer.drawTextCentered("Hello", 100, Color::White, 1, nullptr);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

// =============================================================================
// Renderer Move Constructor & Edge Cases
// =============================================================================

void test_renderer_move_constructor(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    
    // Create renderer then move it
    Renderer renderer1(std::move(cfg));
    Renderer renderer2(std::move(renderer1));
    
    renderer2.drawPixel(50, 50, Color::Red);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    TEST_ASSERT_EQUAL(50, mockRaw->calls[0].x);
    TEST_ASSERT_EQUAL(50, mockRaw->calls[0].y);
}

void test_renderer_set_font(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // setFont should not crash (it's currently a no-op)
    renderer.setFont(nullptr);
    uint8_t fakeFont[] = { 0x00 };
    renderer.setFont(fakeFont);
}

void test_renderer_set_display_size(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.setDisplaySize(128, 128);
    TEST_ASSERT_EQUAL(128, renderer.getLogicalWidth());
    TEST_ASSERT_EQUAL(128, renderer.getLogicalHeight());
}

void test_renderer_render_context_background(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Test with Background context
    PaletteContext bgCtx = PaletteContext::Background;
    renderer.setRenderContext(&bgCtx);
    TEST_ASSERT_EQUAL(&bgCtx, renderer.getRenderContext());
    
    renderer.drawFilledRectangle(10, 10, 50, 50, Color::Red);
    renderer.drawCircle(100, 100, 20, Color::Blue);
    renderer.drawLine(0, 0, 50, 50, Color::Green);
    renderer.drawBitmap(0, 0, 8, 1, (const uint8_t*)"\xFF", Color::White);
    
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_rectangle"));
    TEST_ASSERT_TRUE(mockRaw->hasCall("circle"));
    TEST_ASSERT_TRUE(mockRaw->hasCall("line"));
    TEST_ASSERT_TRUE(mockRaw->hasCall("bitmap"));
    
    renderer.setRenderContext(nullptr);
    TEST_ASSERT_EQUAL(nullptr, renderer.getRenderContext());
}

void test_renderer_sprite_clipping(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Sprite fully off-screen (negative Y)
    uint16_t data[] = { 0x000F, 0x000F };
    Sprite sprite = { data, 4, 2 };
    
    renderer.drawSprite(sprite, 0, -10, Color::White, false);
    // All rows at Y < 0 should be clipped
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Sprite fully off-screen (beyond screen)
    renderer.drawSprite(sprite, 250, 250, Color::White, false);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_sprite_zero_dimensions(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Zero width
    uint16_t data[] = { 0x000F };
    Sprite sprite1 = { data, 0, 4 };
    renderer.drawSprite(sprite1, 10, 10, Color::White, false);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Zero height
    Sprite sprite2 = { data, 4, 0 };
    renderer.drawSprite(sprite2, 10, 10, Color::White, false);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_scaled_sprite_invalid(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t data[] = { 0x000F };
    Sprite sprite = { data, 4, 1 };
    
    // Zero scale should return early
    renderer.drawSprite(sprite, 0, 0, 0.0f, 0.0f, Color::White, false);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    // Negative scale should return early
    renderer.drawSprite(sprite, 0, 0, -1.0f, 1.0f, Color::White, false);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_scaled_sprite_flipx(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // Single pixel in leftmost position
    uint16_t data[] = { 0x0008 }; // Bit 3 set (leftmost of 4-wide)
    Sprite sprite = { data, 4, 1 };
    
    renderer.drawSprite(sprite, 10, 10, 2.0f, 2.0f, Color::White, true);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_multisprite_scaled_valid(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    uint16_t layer1Data[] = { 0x000F, 0x000F, 0x000F, 0x000F };
    uint16_t layer2Data[] = { 0x0001, 0x0001, 0x0001, 0x0001 };
    SpriteLayer layers[2] = { { layer1Data, Color::White }, { layer2Data, Color::Red } };
    MultiSprite ms = { 4, 4, layers, 2 };
    
    renderer.drawMultiSprite(ms, 0, 0, 2.0f, 2.0f);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
}

void test_renderer_multisprite_null_layer_data(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    // MultiSprite with a null data layer — should be skipped
    SpriteLayer layers[1] = { { nullptr, Color::White } };
    MultiSprite ms = { 4, 4, layers, 1 };
    
    renderer.drawMultiSprite(ms, 0, 0);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
    
    renderer.drawMultiSprite(ms, 0, 0, 2.0f, 2.0f);
    TEST_ASSERT_EQUAL(0, mockRaw->calls.size());
}

void test_renderer_offset_all_primitives(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.setDisplayOffset(10, 20);
    
    renderer.drawFilledCircle(0, 0, 5, Color::White);
    TEST_ASSERT_TRUE(mockRaw->calls.size() > 0);
    TEST_ASSERT_EQUAL(10, mockRaw->calls[0].x);
    TEST_ASSERT_EQUAL(20, mockRaw->calls[0].y);
    
    mockRaw->calls.clear();
    renderer.drawRectangle(0, 0, 10, 10, Color::White);
    TEST_ASSERT_EQUAL(10, mockRaw->calls[0].x);
    TEST_ASSERT_EQUAL(20, mockRaw->calls[0].y);
    
    mockRaw->calls.clear();
    renderer.drawLine(0, 0, 5, 5, Color::White);
    TEST_ASSERT_EQUAL(10, mockRaw->calls[0].x);
    TEST_ASSERT_EQUAL(20, mockRaw->calls[0].y);
    
    renderer.setDisplayOffset(0, 0);
}

void test_renderer_filledrectangle_w_with_offset(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig cfg = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(cfg);
    
    renderer.setDisplayOffset(5, 15);
    renderer.drawFilledRectangleW(10, 20, 30, 40, 0x1234);
    TEST_ASSERT_TRUE(mockRaw->hasCall("filled_rectangle"));
    TEST_ASSERT_EQUAL(15, mockRaw->calls[0].x);  // 10 + 5
    TEST_ASSERT_EQUAL(35, mockRaw->calls[0].y);  // 20 + 15
    
    renderer.setDisplayOffset(0, 0);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Particle tests
    RUN_TEST(test_particle_emitter_initialization);
    RUN_TEST(test_particle_burst);
    RUN_TEST(test_particle_emitter_lifecycle);
    
    // Renderer drawing primitives
    RUN_TEST(test_renderer_draw_filled_rectangle);
    RUN_TEST(test_renderer_draw_rectangle);
    RUN_TEST(test_renderer_draw_filled_circle);
    RUN_TEST(test_renderer_draw_circle);
    RUN_TEST(test_renderer_draw_line);
    RUN_TEST(test_renderer_draw_pixel);
    RUN_TEST(test_renderer_transparent_not_drawn);
    RUN_TEST(test_renderer_offset);
    RUN_TEST(test_renderer_offset_bypass);
    RUN_TEST(test_renderer_begin_end_frame);
    RUN_TEST(test_renderer_draw_sprite_basic);
    RUN_TEST(test_renderer_draw_sprite_flipx);
    RUN_TEST(test_renderer_draw_sprite_scaled);
    RUN_TEST(test_renderer_draw_sprite_null_data);
    RUN_TEST(test_renderer_draw_bitmap);
    RUN_TEST(test_renderer_draw_text_basic);
    RUN_TEST(test_renderer_draw_text_centered);
    RUN_TEST(test_renderer_init);
    RUN_TEST(test_renderer_set_render_context);
    RUN_TEST(test_renderer_multisprite);
    RUN_TEST(test_renderer_multisprite_scaled);
    RUN_TEST(test_renderer_draw_filledrectangle_w);
    
    // TileMap tests
    RUN_TEST(test_renderer_tilemap_1bpp_basic);
    RUN_TEST(test_renderer_tilemap_null_data);
    RUN_TEST(test_renderer_tilemap_out_of_bounds_index);
    RUN_TEST(test_renderer_tilemap_with_offset);
    RUN_TEST(test_renderer_tilemap_viewport_culling);
#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
    RUN_TEST(test_renderer_draw_sprite_2bpp);
#endif
#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)
    RUN_TEST(test_renderer_draw_sprite_4bpp);
#endif
#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
    RUN_TEST(test_renderer_draw_tilemap_2bpp);
#endif
#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)
    RUN_TEST(test_renderer_draw_tilemap_4bpp);
#endif
    
    // Text with Font tests
    RUN_TEST(test_renderer_draw_text_with_font);
    RUN_TEST(test_renderer_draw_text_scaled_with_font);
    RUN_TEST(test_renderer_draw_text_unsupported_char);
    RUN_TEST(test_renderer_draw_text_centered_with_font);
    RUN_TEST(test_renderer_draw_text_null_font_no_default);
    
    // Edge cases & move semantics
    RUN_TEST(test_renderer_move_constructor);
    RUN_TEST(test_renderer_set_font);
    RUN_TEST(test_renderer_set_display_size);
    RUN_TEST(test_renderer_render_context_background);
    RUN_TEST(test_renderer_sprite_clipping);
    RUN_TEST(test_renderer_sprite_zero_dimensions);
    RUN_TEST(test_renderer_scaled_sprite_invalid);
    RUN_TEST(test_renderer_scaled_sprite_flipx);
    RUN_TEST(test_renderer_multisprite_scaled_valid);
    RUN_TEST(test_renderer_multisprite_null_layer_data);
    RUN_TEST(test_renderer_offset_all_primitives);
    RUN_TEST(test_renderer_filledrectangle_w_with_offset);
    
    return UNITY_END();
}
