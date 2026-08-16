/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unified test runner for the per-pixel tile collision helper.
 *
 * Covers physics::isTilePixelSolid and physics::isWorldPixelSolid with inline
 * Sprite4bpp / TileMap4bpp fixtures. Mirrors the build-glue pattern of
 * test/unit/test_tile_collision_builder/.
 */

#include <unity.h>
#include <cstring>
#include "../../test_config.h"
#include "physics/TilePixelCollision.h"
#include "graphics/Renderer.h"
#include "test_tile_pixel_collision.h"

using namespace pixelroot32::graphics;
using namespace pixelroot32::physics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// ============================================================================
// Fixture helpers
// ============================================================================

/**
 * @brief Set the 4bpp pixel at (px, py) in a bitmap of the given width.
 *
 * Mirrors the Renderer's LSB-first, 2-pixels-per-byte packing: the low nibble
 * holds the even column and the high nibble holds the odd column. Used to build
 * fixtures independently of the helper under test.
 */
static void setPixel4bpp(uint8_t* data, int width, int px, int py, uint8_t value) {
    const int rowStride = (width * 4 + 7) / 8;
    const int byteIdx = py * rowStride + (px >> 1);
    if (px & 1) {
        data[byteIdx] = static_cast<uint8_t>((data[byteIdx] & 0x0F) | ((value & 0x0F) << 4));
    } else {
        data[byteIdx] = static_cast<uint8_t>((data[byteIdx] & 0xF0) | (value & 0x0F));
    }
}

/**
 * @brief Fill an inclusive rectangle [x0,x1]x[y0,y1] with an opaque value.
 */
static void fillRect4bpp(uint8_t* data, int width, int x0, int y0, int x1, int y1, uint8_t value) {
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            setPixel4bpp(data, width, x, y, value);
        }
    }
}

/**
 * @brief Inline 16x16 Sprite4bpp fixture (rowStride = 8 bytes, 128 bytes total).
 */
struct TileFixture {
    uint8_t data[128];
    Color palette[2];
    Sprite4bpp sprite;

    TileFixture() {
        memset(data, 0, sizeof(data));
        palette[0] = Color::Black;   // index 0 = transparent (unused by helper)
        palette[1] = Color::White;   // index 1 = opaque
        sprite.data = data;
        sprite.palette = palette;
        sprite.width = 16;
        sprite.height = 16;
        sprite.paletteSize = 2;
    }
};

/**
 * @brief Inline 2x2 TileMap4bpp + TileBehaviorLayer world fixture.
 *
 * The map references a single real tile at tileset index 1 (index 0 is the
 * renderer's empty-tile sentinel, so it is never drawn/read).
 */
struct WorldFixture {
    uint8_t tileData[128];
    uint8_t indices[4];
    uint8_t flagData[4];
    Color palette[2];
    Sprite4bpp tiles[2];
    TileMap4bpp map;
    TileBehaviorLayer layer;

    WorldFixture() {
        memset(tileData, 0, sizeof(tileData));
        palette[0] = Color::Black;
        palette[1] = Color::White;

        tiles[0].data = nullptr;   // empty sentinel slot; never read
        tiles[0].palette = palette;
        tiles[0].width = 16;
        tiles[0].height = 16;
        tiles[0].paletteSize = 2;

        tiles[1].data = tileData;
        tiles[1].palette = palette;
        tiles[1].width = 16;
        tiles[1].height = 16;
        tiles[1].paletteSize = 2;

        for (int i = 0; i < 4; ++i) indices[i] = 1;     // all cells -> tile 1
        for (int i = 0; i < 4; ++i) flagData[i] = TILE_NONE;

        map.indices = indices;
        map.width = 2;
        map.height = 2;
        map.tiles = tiles;
        map.tileWidth = 16;
        map.tileHeight = 16;
        map.tileCount = 2;
        map.runtimeMask = nullptr;
        map.animManager = nullptr;
        map.paletteIndices = nullptr;

        layer.data = flagData;
        layer.width = 2;
        layer.height = 2;
    }
};

// ============================================================================
// isTilePixelSolid tests
// ============================================================================

void test_opaque_pixel_returns_true() {
    TileFixture f;
    setPixel4bpp(f.data, 16, 8, 8, 1);   // opaque center pixel

    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 8, 8));
}

void test_transparent_pixel_returns_false() {
    TileFixture f;   // all-zero bitmap = fully transparent

    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 0, 0));
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 15, 15));
}

void test_mixed_tile_corner_vs_center() {
    TileFixture f;
    // Corner (0,0) stays transparent; center (8,8) opaque (even col -> low nibble).
    setPixel4bpp(f.data, 16, 8, 8, 1);

    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 0, 0));
    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 8, 8));
    // (9,8) shares the same byte as (8,8) but its high nibble is untouched.
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 9, 8));

    // Now make the odd column opaque (high nibble) and re-check.
    setPixel4bpp(f.data, 16, 9, 8, 2);
    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 9, 8));
    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 8, 8));   // low nibble intact
}

void test_out_of_bounds_pixel_returns_false() {
    TileFixture f;
    setPixel4bpp(f.data, 16, 8, 8, 1);

    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, -1, 0));
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 0, -1));
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 16, 0));    // >= width
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 0, 16));    // >= height
}

void test_null_tile_returns_false() {
    TEST_ASSERT_FALSE(isTilePixelSolid(nullptr, 0, 0));

    Sprite4bpp s;
    s.data = nullptr;
    s.palette = nullptr;
    s.width = 16;
    s.height = 16;
    s.paletteSize = 0;
    TEST_ASSERT_FALSE(isTilePixelSolid(&s, 0, 0));
}

void test_non_16x16_tile_formula() {
    // 8x8 tile -> rowStride = (8*4+7)/8 = 4 bytes, 32 bytes total.
    uint8_t data[32];
    memset(data, 0, sizeof(data));
    Color palette[2] = {Color::Black, Color::White};
    Sprite4bpp sprite;
    sprite.data = data;
    sprite.palette = palette;
    sprite.width = 8;
    sprite.height = 8;
    sprite.paletteSize = 2;

    setPixel4bpp(data, 8, 7, 7, 1);   // odd col (high nibble), last row
    setPixel4bpp(data, 8, 0, 0, 3);   // even col (low nibble), first row

    TEST_ASSERT_TRUE(isTilePixelSolid(&sprite, 7, 7));
    TEST_ASSERT_TRUE(isTilePixelSolid(&sprite, 0, 0));
    TEST_ASSERT_FALSE(isTilePixelSolid(&sprite, 1, 0));   // high nibble of byte 0 is 0
    TEST_ASSERT_FALSE(isTilePixelSolid(&sprite, 4, 4));
}

// ============================================================================
// Erosion tests
// ============================================================================

void test_erode_isolated_pixel_returns_false() {
    TileFixture f;
    setPixel4bpp(f.data, 16, 8, 8, 1);   // single opaque pixel, all neighbours empty

    // Default (no erosion) keeps the pixel solid (backward compatible).
    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 8, 8));

    // Erosion radius 1 removes an isolated pixel: it has no solid support.
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 8, 8, 1));
}

void test_erode_solid_block_core_stays_solid() {
    TileFixture f;
    fillRect4bpp(f.data, 16, 4, 4, 7, 7, 1);   // 4x4 opaque block

    // The block's interior (5,5) has a fully opaque 3x3 neighbourhood.
    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 5, 5, 1));

    // The block's corner (4,4) neighbours a transparent pixel at (3,4).
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 4, 4, 1));

    // Erosion radius 2 needs a fully opaque 5x5 square: the 4x4 block has none.
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 5, 5, 2));
}

void test_erode_tile_edge_pixel_false() {
    TileFixture f;
    fillRect4bpp(f.data, 16, 0, 0, 15, 15, 1);   // full 16x16 opaque tile

    // Interior pixel survives erosion: all neighbours are in-tile and opaque.
    TEST_ASSERT_TRUE(isTilePixelSolid(&f.sprite, 8, 8, 1));

    // An edge pixel's neighbourhood extends beyond the tile, which the helper
    // treats as transparent -> voided.
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 0, 0, 1));
    TEST_ASSERT_FALSE(isTilePixelSolid(&f.sprite, 15, 15, 1));
}

// ============================================================================
// isWorldPixelSolid tests
// ============================================================================

void test_world_pixel_flag_not_set_short_circuits() {
    WorldFixture f;
    // Bitmap is opaque, but the behavior flag is TILE_NONE -> false, and it
    // must short-circuit before any bitmap read.
    setPixel4bpp(f.tileData, 16, 8, 8, 1);

    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8));

    // Null tilemap also short-circuits immediately.
    TEST_ASSERT_FALSE(isWorldPixelSolid(nullptr, f.layer, 0, 0, 8, 8));
}

void test_world_pixel_solid_flag_transparent_pixel() {
    WorldFixture f;
    f.flagData[0] = TILE_SOLID;   // cell (0,0) solid flag, bitmap transparent at (8,8)

    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8));
}

void test_world_pixel_solid_flag_opaque_pixel() {
    WorldFixture f;
    f.flagData[0] = TILE_SOLID;
    setPixel4bpp(f.tileData, 16, 8, 8, 1);

    TEST_ASSERT_TRUE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8));

    // The requiredFlags parameter is respected: a sensor-only query on a
    // solid-flagged tile returns false.
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8, TILE_SENSOR));
}

void test_world_pixel_erode_isolated_pixel() {
    WorldFixture f;
    f.flagData[0] = TILE_SOLID;
    setPixel4bpp(f.tileData, 16, 8, 8, 1);   // isolated opaque pixel

    // Erosion is forwarded through the wrapper: radius 0 keeps it solid,
    // radius 1 voids the isolated pixel.
    TEST_ASSERT_TRUE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8, TILE_SOLID, 0));
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8, TILE_SOLID, 1));
}

void test_world_pixel_out_of_bounds_tile_coords() {
    WorldFixture f;
    f.flagData[0] = TILE_SOLID;
    setPixel4bpp(f.tileData, 16, 8, 8, 1);

    // Tile coords outside the 2x2 layer/map.
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, -1, 0, 8, 8));
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, -1, 8, 8));
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 2, 0, 8, 8));
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 2, 8, 8));

    // Empty-tile sentinel (index 0) with the solid flag set.
    f.indices[0] = 0;
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8));

    // Tile index beyond tileCount.
    f.indices[0] = 5;
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 0, 0, 8, 8));

    // Tile coord in-bounds in the layer but out-of-bounds in the tilemap
    // (layer 2x2, map shrunk to 1x1): the tilemap bounds check catches it.
    f.indices[0] = 1;
    f.flagData[1] = TILE_SOLID;   // cell (1,0) solid in the 2x2 layer
    f.map.width = 1;
    f.map.height = 1;
    TEST_ASSERT_FALSE(isWorldPixelSolid(&f.map, f.layer, 1, 0, 8, 8));
}

// ============================================================================
// Runner
// ============================================================================

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_opaque_pixel_returns_true);
    RUN_TEST(test_transparent_pixel_returns_false);
    RUN_TEST(test_mixed_tile_corner_vs_center);
    RUN_TEST(test_out_of_bounds_pixel_returns_false);
    RUN_TEST(test_null_tile_returns_false);
    RUN_TEST(test_non_16x16_tile_formula);

    RUN_TEST(test_erode_isolated_pixel_returns_false);
    RUN_TEST(test_erode_solid_block_core_stays_solid);
    RUN_TEST(test_erode_tile_edge_pixel_false);

    RUN_TEST(test_world_pixel_flag_not_set_short_circuits);
    RUN_TEST(test_world_pixel_solid_flag_transparent_pixel);
    RUN_TEST(test_world_pixel_solid_flag_opaque_pixel);
    RUN_TEST(test_world_pixel_erode_isolated_pixel);
    RUN_TEST(test_world_pixel_out_of_bounds_tile_coords);

    return UNITY_END();
}
