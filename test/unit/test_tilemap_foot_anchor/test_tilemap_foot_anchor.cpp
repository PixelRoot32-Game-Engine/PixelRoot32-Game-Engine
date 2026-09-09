/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// Real measured anchors from examples/iso_dungeon/src/assets/: FLOOR_A,
// FLOOR_B, FLOOR_ACCENT, WALL, DOOR_NE, DOOR_NW (DungeonTiles.h) and
// ALTAR, PILLAR (DungeonProps.h).
// Five distinct heights (8, 8, 8, 32, 32, 32, 22, 30) over the SAME 32x16
// diamond footprint are the concrete evidence that the rejected alternative
// `footY = tileHeight / 2` cannot work: WALL/DOOR_NE/DOOR_NW share the same
// extruded footprint as FLOOR_A/B/ACCENT but need a completely different
// anchor row, and ALTAR/PILLAR need two more values again. A per-tile table
// is the only shape that reproduces IsoDraw::drawAtCell's per-sprite FOOT_Y.
static const uint8_t kDungeonFootY[8] = {8, 8, 8, 32, 32, 32, 22, 30};

void test_tilemap_foot_anchor_null_table_returns_zero() {
    Sprite tiles[8] = {};
    TileMap tileMap;
    tileMap.indices = nullptr;
    tileMap.width = 0;
    tileMap.height = 0;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 32;
    tileMap.tileHeight = 16;
    tileMap.tileCount = 8;
    tileMap.runtimeMask = nullptr;
    tileMap.tileFootY = nullptr;

    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(0));
    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(3));
    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(tileMap.tileCount - 1));
}

void test_tilemap_foot_anchor_null_table_returns_zero_4bpp() {
    Sprite4bpp tiles[8] = {};
    TileMap4bpp tileMap;
    tileMap.indices = nullptr;
    tileMap.width = 0;
    tileMap.height = 0;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 32;
    tileMap.tileHeight = 16;
    tileMap.tileCount = 8;
    tileMap.runtimeMask = nullptr;
    tileMap.tileFootY = nullptr;

    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(0));
    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(3));
    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(tileMap.tileCount - 1));
}

void test_tilemap_foot_anchor_returns_stored_value_per_index() {
    Sprite tiles[8] = {};
    TileMap tileMap;
    tileMap.indices = nullptr;
    tileMap.width = 0;
    tileMap.height = 0;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 32;
    tileMap.tileHeight = 16;
    tileMap.tileCount = 8;
    tileMap.runtimeMask = nullptr;
    tileMap.tileFootY = kDungeonFootY;

    TEST_ASSERT_EQUAL_UINT8(8, tileMap.footYFor(0));   // FLOOR_A
    TEST_ASSERT_EQUAL_UINT8(8, tileMap.footYFor(1));   // FLOOR_B
    TEST_ASSERT_EQUAL_UINT8(8, tileMap.footYFor(2));   // FLOOR_ACCENT
    TEST_ASSERT_EQUAL_UINT8(32, tileMap.footYFor(3));  // WALL
    TEST_ASSERT_EQUAL_UINT8(32, tileMap.footYFor(4));  // DOOR_NE
    TEST_ASSERT_EQUAL_UINT8(32, tileMap.footYFor(5));  // DOOR_NW
    TEST_ASSERT_EQUAL_UINT8(22, tileMap.footYFor(6));  // ALTAR
    TEST_ASSERT_EQUAL_UINT8(30, tileMap.footYFor(7));  // PILLAR
}

void test_tilemap_foot_anchor_returns_stored_value_per_index_4bpp() {
    Sprite4bpp tiles[8] = {};
    TileMap4bpp tileMap;
    tileMap.indices = nullptr;
    tileMap.width = 0;
    tileMap.height = 0;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 32;
    tileMap.tileHeight = 16;
    tileMap.tileCount = 8;
    tileMap.runtimeMask = nullptr;
    tileMap.tileFootY = kDungeonFootY;

    TEST_ASSERT_EQUAL_UINT8(8, tileMap.footYFor(0));   // FLOOR_A
    TEST_ASSERT_EQUAL_UINT8(8, tileMap.footYFor(1));   // FLOOR_B
    TEST_ASSERT_EQUAL_UINT8(8, tileMap.footYFor(2));   // FLOOR_ACCENT
    TEST_ASSERT_EQUAL_UINT8(32, tileMap.footYFor(3));  // WALL
    TEST_ASSERT_EQUAL_UINT8(32, tileMap.footYFor(4));  // DOOR_NE
    TEST_ASSERT_EQUAL_UINT8(32, tileMap.footYFor(5));  // DOOR_NW
    TEST_ASSERT_EQUAL_UINT8(22, tileMap.footYFor(6));  // ALTAR
    TEST_ASSERT_EQUAL_UINT8(30, tileMap.footYFor(7));  // PILLAR
}

void test_tilemap_foot_anchor_out_of_range_returns_zero() {
    Sprite tiles[8] = {};
    TileMap tileMap;
    tileMap.indices = nullptr;
    tileMap.width = 0;
    tileMap.height = 0;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 32;
    tileMap.tileHeight = 16;
    tileMap.tileCount = 8;
    tileMap.runtimeMask = nullptr;
    tileMap.tileFootY = kDungeonFootY;

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tileMap.footYFor(tileMap.tileCount),
        "index == tileCount must return 0 even with a non-null table");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tileMap.footYFor(tileMap.tileCount + 5),
        "index > tileCount must return 0 even with a non-null table");
}

void test_tilemap_foot_anchor_zero_index_reads_table() {
    // kDungeonFootY[0] == 8, a non-zero slot, so this assertion can actually
    // distinguish "reads the table" from "hardcoded 0 for the sentinel
    // index". footYFor(0) has no special case for the empty-tile sentinel
    // that drawTileMap skips (src/graphics/Renderer.cpp:892): the table is
    // parallel to tiles[], so slot 0 exists and is the tileset author's data.
    // Special-casing it here would bind the asset format's meaning to a
    // renderer branch that work unit 5 is about to rewrite.
    Sprite tiles[8] = {};
    TileMap tileMap;
    tileMap.indices = nullptr;
    tileMap.width = 0;
    tileMap.height = 0;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 32;
    tileMap.tileHeight = 16;
    tileMap.tileCount = 8;
    tileMap.runtimeMask = nullptr;
    tileMap.tileFootY = kDungeonFootY;

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(8, tileMap.footYFor(0),
        "footYFor(0) must read slot 0 of the table, not hardcode 0");
}

void test_tilemap_foot_anchor_default_constructed_tilemap_has_null_table() {
    TileMap tileMap;
    TEST_ASSERT_NULL(tileMap.tileFootY);
}

void test_tilemap_foot_anchor_default_constructed_tilemap4bpp_has_null_table() {
    TileMap4bpp tileMap;
    TEST_ASSERT_NULL(tileMap.tileFootY);
}

void test_tilemap_foot_anchor_existing_fixture_unchanged() {
    // Existing-style field-assigned TileMap fixture that does NOT name
    // tileFootY, matching the idiom in test/unit/test_tile_mask/. This is
    // the layout-safety evidence: the new NSDMI'd member must not break
    // callers that never set it.
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};

    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;

    TEST_ASSERT_NULL(tileMap.tileFootY);
    TEST_ASSERT_EQUAL_UINT8(0, tileMap.footYFor(0));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_tilemap_foot_anchor_null_table_returns_zero);
    RUN_TEST(test_tilemap_foot_anchor_null_table_returns_zero_4bpp);
    RUN_TEST(test_tilemap_foot_anchor_returns_stored_value_per_index);
    RUN_TEST(test_tilemap_foot_anchor_returns_stored_value_per_index_4bpp);
    RUN_TEST(test_tilemap_foot_anchor_out_of_range_returns_zero);
    RUN_TEST(test_tilemap_foot_anchor_zero_index_reads_table);
    RUN_TEST(test_tilemap_foot_anchor_default_constructed_tilemap_has_null_table);
    RUN_TEST(test_tilemap_foot_anchor_default_constructed_tilemap4bpp_has_null_table);
    RUN_TEST(test_tilemap_foot_anchor_existing_fixture_unchanged);

    return UNITY_END();
}
