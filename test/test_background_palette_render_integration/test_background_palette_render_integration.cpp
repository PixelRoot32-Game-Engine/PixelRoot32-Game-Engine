/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Integration tests for the flow: configure background palette slots ->
 * tilemap with paletteIndices per cell -> render -> verify correct
 * palette color per tile (per-cell palette selection).
 */

#include <unity.h>
#include "../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "../mocks/MockDrawSurface.h"
#include <memory>
#include <cstdint>

using namespace pixelroot32::graphics;

int MockDrawSurface::instances = 0;

// NES palette (from PaletteDefs): Black=0x0000, White=0xFF9C
static constexpr uint16_t NES_BLACK = 0x0000u;
static constexpr uint16_t NES_WHITE = 0xFF9Cu;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)

/**
 * @brief Integration test: init slots -> set slot 0 (NES) and slot 1 (custom) ->
 * draw 2x1 tilemap with paletteIndices [0, 1] -> verify left tile uses NES
 * colors and right tile uses custom colors.
 */
void test_background_palette_slots_to_render_per_cell_integration(void) {
    initBackgroundPaletteSlots();
    setBackgroundPaletteSlot(0, PaletteType::NES);

    // Custom palette for slot 1: distinct RGB565 so we can assert on output
    static const uint16_t customPaletteSlot1[16] = {
        0xAAAAu, 0xBBBBu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu,
        0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu, 0xAAAAu
    };
    setBackgroundCustomPaletteSlot(1, customPaletteSlot1);

    // Tile: 8x8 2bpp, palette [Black, White]. Data: rows with 0x0055 -> first 4 pixels = index 1 (White)
    static const uint16_t tileData2bpp[8] = {
        0x0055u, 0x0055u, 0x0055u, 0x0055u,
        0x0055u, 0x0055u, 0x0055u, 0x0055u
    };
    static const Color tilePalette[] = { Color::Black, Color::White };
    static const Sprite2bpp tile = {
        reinterpret_cast<const uint8_t*>(tileData2bpp),
        tilePalette,
        8, 8, 2
    };

    static const Sprite2bpp tiles[] = { { nullptr, nullptr, 0, 0, 0 }, tile };
    static uint8_t indices[] = { 1, 1 };
    static uint8_t paletteIndices[] = { 0, 1 };  // cell (0,0) -> slot 0, cell (1,0) -> slot 1

    TileMap2bpp map = {};
    map.indices = indices;
    map.width = 2;
    map.height = 1;
    map.tiles = tiles;
    map.tileWidth = 8;
    map.tileHeight = 8;
    map.tileCount = 2;
    map.runtimeMask = nullptr;
    map.paletteIndices = paletteIndices;

    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(std::move(config));
    renderer.init();

    renderer.beginFrame();
    renderer.drawTileMap(map, 0, 0);
    renderer.endFrame();

    TEST_ASSERT_TRUE_MESSAGE(mockRaw->calls.size() > 0,
        "Renderer should produce draw calls");

    bool leftTileUsesNes = false;
    bool rightTileUsesCustom = false;
    for (const auto& call : mockRaw->calls) {
        if (call.type != "pixel") continue;
        uint16_t c = call.color;
        int x = call.x;
        if (x < 8) {
            if (c == NES_BLACK || c == NES_WHITE) leftTileUsesNes = true;
        } else {
            if (c == 0xAAAAu || c == 0xBBBBu) rightTileUsesCustom = true;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(leftTileUsesNes,
        "Left tile (palette slot 0) should be drawn with NES palette colors");
    TEST_ASSERT_TRUE_MESSAGE(rightTileUsesCustom,
        "Right tile (palette slot 1) should be drawn with custom slot 1 colors");
}

/**
 * @brief When paletteIndices is nullptr, all cells use slot 0 (default).
 */
void test_background_palette_no_palette_indices_uses_slot_zero(void) {
    initBackgroundPaletteSlots();
    setBackgroundPaletteSlot(0, PaletteType::NES);

    static const uint16_t customSlot1[16] = {
        0x1111u, 0x2222u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u,
        0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u
    };
    setBackgroundCustomPaletteSlot(1, customSlot1);

    static const uint16_t tileData[8] = {
        0x0055u, 0x0055u, 0x0055u, 0x0055u,
        0x0055u, 0x0055u, 0x0055u, 0x0055u
    };
    static const Color tilePalette[] = { Color::Black, Color::White };
    static const Sprite2bpp tile = {
        reinterpret_cast<const uint8_t*>(tileData),
        tilePalette, 8, 8, 2
    };
    static const Sprite2bpp tiles[] = { { nullptr, nullptr, 0, 0, 0 }, tile };
    static uint8_t indices[] = { 1, 1 };

    TileMap2bpp map = {};
    map.indices = indices;
    map.width = 2;
    map.height = 1;
    map.tiles = tiles;
    map.tileWidth = 8;
    map.tileHeight = 8;
    map.tileCount = 2;
    map.runtimeMask = nullptr;
    map.paletteIndices = nullptr;  // no per-cell palette -> all use slot 0

    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(std::move(config));
    renderer.init();

    renderer.beginFrame();
    renderer.drawTileMap(map, 0, 0);
    renderer.endFrame();

    bool anyNes = false;
    bool anyCustom = false;
    for (const auto& call : mockRaw->calls) {
        if (call.type != "pixel") continue;
        if (call.color == NES_BLACK || call.color == NES_WHITE) anyNes = true;
        if (call.color == 0x1111u || call.color == 0x2222u) anyCustom = true;
    }
    TEST_ASSERT_TRUE_MESSAGE(anyNes, "Without paletteIndices all tiles should use slot 0 (NES)");
    TEST_ASSERT_FALSE_MESSAGE(anyCustom, "Custom slot 1 should not be used when paletteIndices is null");
}

/**
 * @brief Five layers: 5x1 tilemap, each cell uses a different palette slot (0..4),
 * each slot has a custom palette. Verifies that each "layer" (tile column) renders
 * with its assigned slot colors.
 */
void test_background_palette_five_layers_each_custom_slot_integration(void) {
    initBackgroundPaletteSlots();

    // Slot 0: NES (layer 0)
    setBackgroundPaletteSlot(0, PaletteType::NES);

    // Slots 1..4: custom palettes with distinct colors per layer
    static const uint16_t pal1[16] = {
        0x1111u, 0x2222u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u,
        0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u, 0x1111u
    };
    static const uint16_t pal2[16] = {
        0x3333u, 0x4444u, 0x3333u, 0x3333u, 0x3333u, 0x3333u, 0x3333u, 0x3333u,
        0x3333u, 0x3333u, 0x3333u, 0x3333u, 0x3333u, 0x3333u, 0x3333u, 0x3333u
    };
    static const uint16_t pal3[16] = {
        0x5555u, 0x6666u, 0x5555u, 0x5555u, 0x5555u, 0x5555u, 0x5555u, 0x5555u,
        0x5555u, 0x5555u, 0x5555u, 0x5555u, 0x5555u, 0x5555u, 0x5555u, 0x5555u
    };
    static const uint16_t pal4[16] = {
        0x7777u, 0x8888u, 0x7777u, 0x7777u, 0x7777u, 0x7777u, 0x7777u, 0x7777u,
        0x7777u, 0x7777u, 0x7777u, 0x7777u, 0x7777u, 0x7777u, 0x7777u, 0x7777u
    };
    setBackgroundCustomPaletteSlot(1, pal1);
    setBackgroundCustomPaletteSlot(2, pal2);
    setBackgroundCustomPaletteSlot(3, pal3);
    setBackgroundCustomPaletteSlot(4, pal4);

    // Shared tile: 8x8 2bpp, palette [Black, White], some pixels use index 1 (White)
    static const uint16_t tileData[8] = {
        0x0055u, 0x0055u, 0x0055u, 0x0055u,
        0x0055u, 0x0055u, 0x0055u, 0x0055u
    };
    static const Color tilePalette[] = { Color::Black, Color::White };
    static const Sprite2bpp tile = {
        reinterpret_cast<const uint8_t*>(tileData),
        tilePalette, 8, 8, 2
    };
    static const Sprite2bpp tiles[] = { { nullptr, nullptr, 0, 0, 0 }, tile };

    // 5 layers = 5 tiles in a row, each cell with its palette slot 0..4
    static uint8_t indices5[] = { 1, 1, 1, 1, 1 };
    static uint8_t paletteIndices5[] = { 0, 1, 2, 3, 4 };

    TileMap2bpp map = {};
    map.indices = indices5;
    map.width = 5;
    map.height = 1;
    map.tiles = tiles;
    map.tileWidth = 8;
    map.tileHeight = 8;
    map.tileCount = 2;
    map.runtimeMask = nullptr;
    map.paletteIndices = paletteIndices5;

    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(std::move(config));
    renderer.init();

    renderer.beginFrame();
    renderer.drawTileMap(map, 0, 0);
    renderer.endFrame();

    TEST_ASSERT_TRUE_MESSAGE(mockRaw->calls.size() > 0,
        "Renderer should produce draw calls for 5 layers");

    // Layer 0: x in [0, 7]   -> NES (0x0000, 0xFF9C)
    // Layer 1: x in [8, 15]  -> pal1 (0x1111, 0x2222)
    // Layer 2: x in [16, 23] -> pal2 (0x3333, 0x4444)
    // Layer 3: x in [24, 31] -> pal3 (0x5555, 0x6666)
    // Layer 4: x in [32, 39] -> pal4 (0x7777, 0x8888)
    bool layer0Ok = false;
    bool layer1Ok = false;
    bool layer2Ok = false;
    bool layer3Ok = false;
    bool layer4Ok = false;

    for (const auto& call : mockRaw->calls) {
        if (call.type != "pixel") continue;
        int x = call.x;
        uint16_t c = call.color;
        if (x >= 0 && x < 8) {
            if (c == NES_BLACK || c == NES_WHITE) layer0Ok = true;
        } else if (x >= 8 && x < 16) {
            if (c == 0x1111u || c == 0x2222u) layer1Ok = true;
        } else if (x >= 16 && x < 24) {
            if (c == 0x3333u || c == 0x4444u) layer2Ok = true;
        } else if (x >= 24 && x < 32) {
            if (c == 0x5555u || c == 0x6666u) layer3Ok = true;
        } else if (x >= 32 && x < 40) {
            if (c == 0x7777u || c == 0x8888u) layer4Ok = true;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(layer0Ok, "Layer 0 (slot 0) should use NES palette");
    TEST_ASSERT_TRUE_MESSAGE(layer1Ok, "Layer 1 (slot 1) should use custom palette 1");
    TEST_ASSERT_TRUE_MESSAGE(layer2Ok, "Layer 2 (slot 2) should use custom palette 2");
    TEST_ASSERT_TRUE_MESSAGE(layer3Ok, "Layer 3 (slot 3) should use custom palette 3");
    TEST_ASSERT_TRUE_MESSAGE(layer4Ok, "Layer 4 (slot 4) should use custom palette 4");
}

#endif // PIXELROOT32_ENABLE_2BPP_SPRITES

#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)

/**
 * @brief Same flow for 4bpp tilemap: paletteIndices [0, 1] -> left NES, right custom.
 */
void test_background_palette_slots_4bpp_per_cell_integration(void) {
    initBackgroundPaletteSlots();
    setBackgroundPaletteSlot(0, PaletteType::NES);

    static const uint16_t customPaletteSlot1[16] = {
        0xCCCCu, 0xDDDDu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu,
        0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu, 0xCCCCu
    };
    setBackgroundCustomPaletteSlot(1, customPaletteSlot1);

    // 4bpp 8x8: 4 bits per pixel, 8 pixels per row = 4 bytes per row, 8 rows = 32 bytes
    // One row: pixel 0 = index 1 (White), rest 0. Byte 0 = 0x10 (high nibble 1, low 0), byte 1 = 0x00, ...
    static const uint8_t tileData4bpp[32] = {
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00
    };
    static const Color tilePalette4[] = { Color::Black, Color::White };
    static const Sprite4bpp tile4 = {
        tileData4bpp, tilePalette4, 8, 8, 2
    };
    static const Sprite4bpp tiles4[] = { { nullptr, nullptr, 0, 0, 0 }, tile4 };
    static uint8_t indices4[] = { 1, 1 };
    static uint8_t paletteIndices4[] = { 0, 1 };

    TileMap4bpp map4 = {};
    map4.indices = indices4;
    map4.width = 2;
    map4.height = 1;
    map4.tiles = tiles4;
    map4.tileWidth = 8;
    map4.tileHeight = 8;
    map4.tileCount = 2;
    map4.runtimeMask = nullptr;
    map4.paletteIndices = paletteIndices4;

    auto mock = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mock.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(std::move(config));
    renderer.init();

    renderer.beginFrame();
    renderer.drawTileMap(map4, 0, 0);
    renderer.endFrame();

    bool leftNes = false;
    bool rightCustom = false;
    for (const auto& call : mockRaw->calls) {
        if (call.type != "pixel") continue;
        if (call.x < 8 && (call.color == NES_BLACK || call.color == NES_WHITE)) leftNes = true;
        if (call.x >= 8 && (call.color == 0xCCCCu || call.color == 0xDDDDu)) rightCustom = true;
    }
    TEST_ASSERT_TRUE_MESSAGE(leftNes, "4bpp left tile should use NES (slot 0)");
    TEST_ASSERT_TRUE_MESSAGE(rightCustom, "4bpp right tile should use custom slot 1");
}

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
    RUN_TEST(test_background_palette_slots_to_render_per_cell_integration);
    RUN_TEST(test_background_palette_no_palette_indices_uses_slot_zero);
    RUN_TEST(test_background_palette_five_layers_each_custom_slot_integration);
#endif
#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)
    RUN_TEST(test_background_palette_slots_4bpp_per_cell_integration);
#endif

    return UNITY_END();
}
