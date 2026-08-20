/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for graphics::expandProjectedMapBounds: corner-based extent,
 * per-tile overhang, index-0 sentinel exclusion, union across repeated
 * calls, degenerate/defensive no-op inputs, and cross-format parity.
 *
 * Fixtures never read `.data` / `.palette` -- expandProjectedMapBounds only
 * reads `width`, `height` and the foot-anchor table, so those pointers are
 * left null throughout. This suite intentionally does not draw anything or
 * touch a Renderer/DrawSurface: pure geometry over caller-owned POD, per the
 * design's "N/A -- no ... process-integration boundary" threat matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

#include "graphics/ProjectedMapBounds.h"
#include "math/Projection.h"

namespace math = pixelroot32::math;

namespace {

// Isometric 2:1, 32x16 diamond -- the header's own documented layout
// (math/Projection.h) and the spec's canonical fixture basis.
constexpr math::ProjectionSpec kIso{0, 0, 16, 8, -16, 8};

// Plain axis-aligned basis, used for the orthogonal-basis scenario.
constexpr math::ProjectionSpec kOrthogonal{0, 0, 16, 0, 0, 16};

// --- Tileset A: index 0 = 32x32 empty-tile sentinel (footY 0, the
// exporter's convention -- see TileMapGeneric::footYFor's own note),
// index 1 = the real 32x16 foot-anchored tile (footY 8). The sentinel is
// deliberately NOT zero-sized here (unlike the sibling draw-test suite's
// Sprite4bpp{}): its whole purpose in this suite is to prove the bounds
// scan excludes it even though it WOULD inflate the box if counted (see
// the dedicated index-0 exclusion test below, the 240-vs-256 discriminator
// the spec calls out by name).
constexpr uint8_t kSentinelWidth = 32, kSentinelHeight = 32;
constexpr uint8_t kTileAWidth = 32, kTileAHeight = 16, kTileAFootY = 8;

const Sprite4bpp kTilesetA4bpp[2] = {
    Sprite4bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite4bpp{nullptr, nullptr, kTileAWidth, kTileAHeight, 0},
};
const Sprite2bpp kTilesetA2bpp[2] = {
    Sprite2bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite2bpp{nullptr, nullptr, kTileAWidth, kTileAHeight, 0},
};
const Sprite kTilesetA1bpp[2] = {
    Sprite{nullptr, kSentinelWidth, kSentinelHeight},
    Sprite{nullptr, kTileAWidth, kTileAHeight},
};
const uint8_t kTilesetAFootY[2] = {0, kTileAFootY};

// --- Tileset B: same sentinel convention, different real tile (16x40,
// footY 32) -- used by the layer-union scenario to prove left/right come
// from whichever layer is wider and top/bottom from whichever is taller,
// not from "whichever ran last".
constexpr uint8_t kTileBWidth = 16, kTileBHeight = 40, kTileBFootY = 32;

const Sprite4bpp kTilesetB4bpp[2] = {
    Sprite4bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite4bpp{nullptr, nullptr, kTileBWidth, kTileBHeight, 0},
};
const uint8_t kTilesetBFootY[2] = {0, kTileBFootY};

// --- Orthogonal-basis tileset: 16x16, footY 0 (fully top-anchored).
constexpr uint8_t kOrthoTileWidth = 16, kOrthoTileHeight = 16, kOrthoTileFootY = 0;

const Sprite4bpp kOrthoTiles4bpp[2] = {
    Sprite4bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite4bpp{nullptr, nullptr, kOrthoTileWidth, kOrthoTileHeight, 0},
};
const uint8_t kOrthoFootY[2] = {0, kOrthoTileFootY};

// --- Tileset D: isolates the spec's "Sprite taller than its cell" scenario.
// Index 1 has the SAME width as the cell (16 -- no horizontal overhang, so
// left/right cannot distinguish the bug on this fixture, deliberately) but
// height 32, DOUBLE the cell's 16, anchored by footY=16. `up` reads
// `footYFor(i)` only -- never `tile.height` -- so it is identical whether
// the scan is correct or regressed; `down` (`tile.height - footY`) is the
// ONLY field this fixture pins: 32-16=16 (correct) vs 16-16=0 -- not >
// footY, so the regressed scan (reading `map.tileHeight` instead of
// `tile.height`) would compute down=0, not 16.
constexpr uint8_t kTileDCellWidth = 16, kTileDCellHeight = 16;
constexpr uint8_t kTileDWidth = 16, kTileDHeight = 32, kTileDFootY = 16;

const Sprite4bpp kTilesetD4bpp[2] = {
    Sprite4bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite4bpp{nullptr, nullptr, kTileDWidth, kTileDHeight, 0},
};
const uint8_t kTilesetDFootY[2] = {0, kTileDFootY};

// --- Tileset E: isolates the spec's "Sprite wider than its cell" scenario.
// Index 1 has the SAME height as the cell (16 -- no vertical overhang,
// isolates width) but width 48, wider than the cell's 32. left/right
// (`tile.width/2`, `tile.width - tile.width/2`) are the ONLY fields this
// fixture pins: 48/2=24 (correct) vs the regressed `map.tileWidth`(32)/2=16.
constexpr uint8_t kTileECellWidth = 32, kTileECellHeight = 16;
constexpr uint8_t kTileEWidth = 48, kTileEHeight = 16, kTileEFootY = 8;

const Sprite4bpp kTilesetE4bpp[2] = {
    Sprite4bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite4bpp{nullptr, nullptr, kTileEWidth, kTileEHeight, 0},
};
const uint8_t kTilesetEFootY[2] = {0, kTileEFootY};

// --- Tileset F: isolates the spec's "Foot table with varying anchors"
// scenario. Two real tiles, both with the cell's own width (16 -- isolates
// height/footY) but DIFFERENT height and footY: index 1 (16x40, footY=32)
// contributes the maximum UP reach (32); index 2 (16x24, footY=4)
// contributes the maximum DOWN reach (24-4=20) -- the two maxima genuinely
// come from different tile indices, per the spec's own scenario text. Also
// doubles as a mutation detector on `down`: a regressed scan reading
// `map.tileHeight` (16) for both tiles instead of their own `tile.height`
// would compute down = max(16>32 ? 0, 16>4 ? 16-4=12) = 12, not 20.
constexpr uint8_t kTileFCellWidth = 16, kTileFCellHeight = 16;
constexpr uint8_t kTileF1Width = 16, kTileF1Height = 40, kTileF1FootY = 32;
constexpr uint8_t kTileF2Width = 16, kTileF2Height = 24, kTileF2FootY = 4;

const Sprite4bpp kTilesetF4bpp[3] = {
    Sprite4bpp{nullptr, nullptr, kSentinelWidth, kSentinelHeight, 0},
    Sprite4bpp{nullptr, nullptr, kTileF1Width, kTileF1Height, 0},
    Sprite4bpp{nullptr, nullptr, kTileF2Width, kTileF2Height, 0},
};
const uint8_t kTilesetFFootY[3] = {0, kTileF1FootY, kTileF2FootY};

/// Builds a 10x20 TileMap4bpp using tileset A, all cells index 1.
TileMap4bpp make10x20MapA4bpp(uint8_t* indices) {
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 10;
    map.height = 20;
    map.tiles = kTilesetA4bpp;
    map.tileWidth = kTileAWidth;
    map.tileHeight = kTileAHeight;
    map.tileCount = 2;
    map.tileFootY = kTilesetAFootY;
    return map;
}

/// Same 10x20/tileset-A geometry as make10x20MapA4bpp, through the 2bpp
/// format -- used by the 3-format parity test.
TileMap2bpp make10x20MapA2bpp(uint8_t* indices) {
    TileMap2bpp map{};
    map.indices = indices;
    map.width = 10;
    map.height = 20;
    map.tiles = kTilesetA2bpp;
    map.tileWidth = kTileAWidth;
    map.tileHeight = kTileAHeight;
    map.tileCount = 2;
    map.tileFootY = kTilesetAFootY;
    return map;
}

/// Same 10x20/tileset-A geometry, through the 1bpp format -- used by the
/// 3-format parity test.
TileMap make10x20MapA1bpp(uint8_t* indices) {
    TileMap map{};
    map.indices = indices;
    map.width = 10;
    map.height = 20;
    map.tiles = kTilesetA1bpp;
    map.tileWidth = kTileAWidth;
    map.tileHeight = kTileAHeight;
    map.tileCount = 2;
    map.tileFootY = kTilesetAFootY;
    return map;
}

/// Builds a 10x20 TileMap4bpp using tileset B (different real tile), for
/// the layer-union test.
TileMap4bpp make10x20MapB4bpp(uint8_t* indices) {
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 10;
    map.height = 20;
    map.tiles = kTilesetB4bpp;
    map.tileWidth = kTileBWidth;
    map.tileHeight = kTileBHeight;
    map.tileCount = 2;
    map.tileFootY = kTilesetBFootY;
    return map;
}

/// Builds a 2x2 TileMap4bpp over an arbitrary tileset -- shared by the three
/// per-tile-overhang isolation scenarios below (tilesets D/E/F). 2x2/kIso
/// reproduces the exact corner extent {minX=-16, maxX=16, minY=0, maxY=16}
/// independently re-derived in
/// test_projected_map_bounds_camera_range_centre_collapse, so only each
/// tileset's reach differs between these tests -- the corner math itself is
/// not what is under test here.
TileMap4bpp make2x2Map4bpp(uint8_t* indices, const Sprite4bpp* tiles, const uint8_t* footY,
                            uint16_t tileCount, uint8_t cellWidth, uint8_t cellHeight) {
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 2;
    map.height = 2;
    map.tiles = tiles;
    map.tileWidth = cellWidth;
    map.tileHeight = cellHeight;
    map.tileCount = tileCount;
    map.tileFootY = footY;
    return map;
}

/// Builds a 3x2 TileMap4bpp using the orthogonal-basis tileset.
TileMap4bpp make3x2MapOrtho(uint8_t* indices) {
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 3;
    map.height = 2;
    map.tiles = kOrthoTiles4bpp;
    map.tileWidth = kOrthoTileWidth;
    map.tileHeight = kOrthoTileHeight;
    map.tileCount = 2;
    map.tileFootY = kOrthoFootY;
    return map;
}

void assertBounds(const ScreenBounds& b, int left, int top, int right, int bottom, bool valid) {
    TEST_ASSERT_EQUAL_INT(left, b.left);
    TEST_ASSERT_EQUAL_INT(top, b.top);
    TEST_ASSERT_EQUAL_INT(right, b.right);
    TEST_ASSERT_EQUAL_INT(bottom, b.bottom);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(valid), static_cast<int>(b.valid));
}

void assertCameraBounds(const CameraBounds& c, int minX, int maxX, int minY, int maxY, bool valid) {
    TEST_ASSERT_EQUAL_INT(minX, c.minX);
    TEST_ASSERT_EQUAL_INT(maxX, c.maxX);
    TEST_ASSERT_EQUAL_INT(minY, c.minY);
    TEST_ASSERT_EQUAL_INT(maxY, c.maxY);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(valid), static_cast<int>(c.valid));
}

}  // namespace

/// Req: Corner-based extent + Per-tile overhang. A 10x20 map on kIso with
/// tileset A must extend 480x240 -- NOT 320x320 (10*32 x 20*16, the wrong
/// map-level-tileWidth/tileHeight calculation this requirement forbids).
void test_projected_map_bounds_corner_extent_with_per_tile_overhang(void) {
    uint8_t indices[10 * 20];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make10x20MapA4bpp(indices);

    ScreenBounds bounds{};
    expandProjectedMapBounds(bounds, map, kIso);

    assertBounds(bounds, -320, -8, 160, 232, true);
    TEST_ASSERT_EQUAL_INT(480, bounds.right - bounds.left);
    TEST_ASSERT_EQUAL_INT(240, bounds.bottom - bounds.top);
}

/// Req: Sentinel index 0 excluded (the 240-vs-256 discriminator the spec
/// calls out by name). Tileset A's index 0 is a full-size 32x32 sentinel
/// with footY 0; if the scan wrongly included it, downReach would become
/// tiles[0].height - 0 = 32, forcing bottom = 224 + 32 = 256 instead of the
/// correct 224 + 8 = 232. Asserting both directions makes this test fail
/// loudly under either regression: dropping the exclusion, or a scan that
/// simply never contributes anything (a vacuous 0-reach stub).
void test_projected_map_bounds_sentinel_index_0_excluded(void) {
    uint8_t indices[10 * 20];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make10x20MapA4bpp(indices);

    ScreenBounds bounds{};
    expandProjectedMapBounds(bounds, map, kIso);

    TEST_ASSERT_TRUE(bounds.valid);
    TEST_ASSERT_EQUAL_INT(232, bounds.bottom);
    TEST_ASSERT_NOT_EQUAL(256, bounds.bottom);
}

/// Req: Per-tile overhang -- "Sprite taller than its cell". See tileset D's
/// own comment for the full hand-derivation. Corners: 2x2/kIso gives
/// {minX=-16, maxX=16, minY=0, maxY=16}; reach is left=right=16/2=8 (width
/// equals the cell, uninteresting here by design), up=footYFor(1)=16,
/// down=tile.height-footY=32-16=16. Bounds = {-24, -16, 24, 32}.
void test_projected_map_bounds_per_tile_sprite_taller_than_cell(void) {
    uint8_t indices[2 * 2];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make2x2Map4bpp(indices, kTilesetD4bpp, kTilesetDFootY, 2,
                                      kTileDCellWidth, kTileDCellHeight);

    ScreenBounds bounds{};
    expandProjectedMapBounds(bounds, map, kIso);

    assertBounds(bounds, -24, -16, 24, 32, true);
    // The regression this pins: reading map.tileHeight (16) instead of the
    // per-tile tile.height (32) for the down reach would make bottom 16,
    // not 32 -- see tileset D's comment for the full derivation.
    TEST_ASSERT_NOT_EQUAL(16, bounds.bottom);
}

/// Req: Per-tile overhang -- "Sprite wider than its cell". See tileset E's
/// own comment for the full hand-derivation. Corners: same 2x2/kIso
/// {minX=-16, maxX=16, minY=0, maxY=16}; reach is left=48/2=24,
/// right=48-24=24, up=footYFor(1)=8, down=tile.height-footY=16-8=8.
/// Bounds = {-40, -8, 40, 24}.
void test_projected_map_bounds_per_tile_sprite_wider_than_cell(void) {
    uint8_t indices[2 * 2];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make2x2Map4bpp(indices, kTilesetE4bpp, kTilesetEFootY, 2,
                                      kTileECellWidth, kTileECellHeight);

    ScreenBounds bounds{};
    expandProjectedMapBounds(bounds, map, kIso);

    assertBounds(bounds, -40, -8, 40, 24, true);
    // The regression this pins: reading map.tileWidth (32) instead of the
    // per-tile tile.width (48) for left/right would make right 32, not 40
    // -- see tileset E's comment for the full derivation.
    TEST_ASSERT_NOT_EQUAL(32, bounds.right);
}

/// Req: Per-tile overhang -- "Foot table with varying anchors". See tileset
/// F's own comment for the full hand-derivation: index 1's footY=32 wins
/// the max UP reach, index 2's height/footY (24-4=20) wins the max DOWN
/// reach -- from a DIFFERENT index than the one that won UP. Corners: same
/// 2x2/kIso {minX=-16, maxX=16, minY=0, maxY=16}; reach is left=right=8
/// (both tiles share the cell's own width, uninteresting here by design),
/// up=32, down=20. Bounds = {-24, -32, 24, 36}.
void test_projected_map_bounds_per_tile_foot_table_varying_anchors(void) {
    uint8_t indices[2 * 2];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make2x2Map4bpp(indices, kTilesetF4bpp, kTilesetFFootY, 3,
                                      kTileFCellWidth, kTileFCellHeight);

    ScreenBounds bounds{};
    expandProjectedMapBounds(bounds, map, kIso);

    assertBounds(bounds, -24, -32, 24, 36, true);
    // The regression this pins: reading map.tileHeight (16) for both tiles
    // instead of each one's own tile.height would make bottom 28, not 36
    // -- see tileset F's comment for the full derivation.
    TEST_ASSERT_NOT_EQUAL(28, bounds.bottom);
}

/// Req: Union across repeated calls. Layer A (tileset A) then layer B
/// (tileset B, different real-tile dimensions) on the same 10x20/kIso
/// geometry: the union's left/right come from A (the wider tileset) and
/// top/bottom from B (the taller one) -- proving the union takes the
/// extremum per edge, not "whichever call ran last".
void test_projected_map_bounds_unions_across_layers(void) {
    uint8_t indicesA[10 * 20];
    uint8_t indicesB[10 * 20];
    for (auto& idx : indicesA) idx = 1;
    for (auto& idx : indicesB) idx = 1;
    TileMap4bpp mapA = make10x20MapA4bpp(indicesA);
    TileMap4bpp mapB = make10x20MapB4bpp(indicesB);

    ScreenBounds bounds{};
    TEST_ASSERT_FALSE(bounds.valid);

    expandProjectedMapBounds(bounds, mapA, kIso);
    assertBounds(bounds, -320, -8, 160, 232, true);

    // B alone (verified independently, in a fresh accumulator) before
    // asserting the union -- otherwise a bug that makes B a no-op could
    // pass the union assertion vacuously (A's box unchanged).
    ScreenBounds bBoundsAlone{};
    expandProjectedMapBounds(bBoundsAlone, mapB, kIso);
    assertBounds(bBoundsAlone, -312, -32, 152, 232, true);
    // This is also where HORIZONTAL sentinel-index-0 exclusion is proven
    // (the dedicated sentinel test above only proves the vertical axis,
    // since tileset A's sentinel and real tile share width 32). Tileset B's
    // sentinel is 32x32 (left=right=16) while its real tile is 16x40
    // (left=right=8); wrongly including the sentinel would widen left from
    // -312 to -320 and right from 152 to 160. Asserting both directions
    // ties this coverage explicitly to the "all four reaches" requirement
    // instead of leaving it incidental.
    TEST_ASSERT_NOT_EQUAL(-320, bBoundsAlone.left);
    TEST_ASSERT_NOT_EQUAL(160, bBoundsAlone.right);

    expandProjectedMapBounds(bounds, mapB, kIso);
    assertBounds(bounds, -320, -32, 160, 232, true);
}

/// Req: Degenerate/defensive inputs are no-ops. Byte-identical to
/// drawTileMapProjectedImpl's own guard (Renderer.cpp:1131-1136): each of
/// the seven degenerate conditions below must leave an ALREADY-VALID
/// ScreenBounds completely unmodified, proving the guard fires before any
/// scan/corner work rather than merely "computing zero".
void test_projected_map_bounds_degenerate_inputs_are_no_ops(void) {
    uint8_t indices[10 * 20];
    for (auto& idx : indices) idx = 1;

    const ScreenBounds seeded{7, 11, 13, 17, true};

    auto assertNoOp = [&](TileMap4bpp map) {
        ScreenBounds bounds = seeded;
        expandProjectedMapBounds(bounds, map, kIso);
        assertBounds(bounds, seeded.left, seeded.top, seeded.right, seeded.bottom, seeded.valid);
    };

    TileMap4bpp base = make10x20MapA4bpp(indices);

    TileMap4bpp nullIndices = base;
    nullIndices.indices = nullptr;
    assertNoOp(nullIndices);

    TileMap4bpp nullTiles = base;
    nullTiles.tiles = nullptr;
    assertNoOp(nullTiles);

    TileMap4bpp zeroWidth = base;
    zeroWidth.width = 0;
    assertNoOp(zeroWidth);

    TileMap4bpp zeroHeight = base;
    zeroHeight.height = 0;
    assertNoOp(zeroHeight);

    TileMap4bpp zeroTileWidth = base;
    zeroTileWidth.tileWidth = 0;
    assertNoOp(zeroTileWidth);

    TileMap4bpp zeroTileHeight = base;
    zeroTileHeight.tileHeight = 0;
    assertNoOp(zeroTileHeight);

    TileMap4bpp zeroTileCount = base;
    zeroTileCount.tileCount = 0;
    assertNoOp(zeroTileCount);
}

/// Req: Corner-based extent + Per-tile overhang, proven format-agnostic.
/// The exact same 10x20/kIso/tileset-A geometry, expressed as TileMap,
/// TileMap2bpp and TileMap4bpp, must produce an IDENTICAL ScreenBounds --
/// proving the shared expandFromCellRect/scanTilesetReach machinery, not
/// each overload's own body, determines the result.
void test_projected_map_bounds_identical_across_1bpp_2bpp_4bpp(void) {
    uint8_t indices4bpp[10 * 20];
    uint8_t indices2bpp[10 * 20];
    uint8_t indices1bpp[10 * 20];
    for (auto& idx : indices4bpp) idx = 1;
    for (auto& idx : indices2bpp) idx = 1;
    for (auto& idx : indices1bpp) idx = 1;

    TileMap4bpp map4bpp = make10x20MapA4bpp(indices4bpp);
    TileMap2bpp map2bpp = make10x20MapA2bpp(indices2bpp);
    TileMap map1bpp = make10x20MapA1bpp(indices1bpp);

    ScreenBounds bounds4bpp{};
    ScreenBounds bounds2bpp{};
    ScreenBounds bounds1bpp{};
    expandProjectedMapBounds(bounds4bpp, map4bpp, kIso);
    expandProjectedMapBounds(bounds2bpp, map2bpp, kIso);
    expandProjectedMapBounds(bounds1bpp, map1bpp, kIso);

    assertBounds(bounds4bpp, -320, -8, 160, 232, true);
    assertBounds(bounds2bpp, -320, -8, 160, 232, true);
    assertBounds(bounds1bpp, -320, -8, 160, 232, true);
}

/// Req: Corner-based extent, orthogonal basis. Proves the corner-transform
/// formula holds for a non-isometric basis too -- the requirement forbids
/// an isometric-only implementation.
void test_projected_map_bounds_orthogonal_basis(void) {
    uint8_t indices[3 * 2];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make3x2MapOrtho(indices);

    ScreenBounds bounds{};
    expandProjectedMapBounds(bounds, map, kOrthogonal);

    assertBounds(bounds, -8, 0, 40, 32, true);
}

/// Req: Centre-collapse on undersized world -- the no-collapse boundary
/// case. World #1 (test 1's own bounds, {-320,-8,160,232}) against a
/// 240x240 viewport: X is a normal (non-collapsed) range because the world
/// (480 wide) exceeds the viewport. Y is the exact-equality edge --
/// world height is exactly 240, so `hi == lo` (not `hi < lo`) and the axis
/// must NOT collapse; it must simply resolve to the single value both ends
/// already share. This pins the half-open subtraction having no off-by-one:
/// a `right - viewWidth` that were one pixel wrong would flip this case
/// into either a spurious collapse or a range that excludes a legal
/// position.
void test_projected_map_bounds_camera_range_no_collapse(void) {
    const ScreenBounds world{-320, -8, 160, 232, true};

    const CameraBounds range = cameraRangeFor(world, 240, 240);

    assertCameraBounds(range, -320, -80, -8, -8, true);
}

/// Req: Centre-collapse on undersized world (CRITICAL). A 2x2 map on kIso
/// with tileset A produces world {-32,-8,32,24} (hand-derived: corner
/// anchors {minX=-16,maxX=16,minY=0,maxY=16} widened by tileset A's
/// reach 16/16/8/8). Against a 240x240 viewport BOTH axes are narrower
/// than the viewport, so both must collapse independently to their own
/// midpoint: X to (-32 + (32-240))/2 = -240/2 = -120, Y to
/// (-8 + (24-240))/2 = -224/2 = -112. Without the collapse,
/// Camera2D::setPosition's min-then-max clamp order would jam the camera
/// at maxX/maxY with all the slack on one side -- this test's expected
/// values are the CENTRED point, not the raw (possibly inverted) range.
void test_projected_map_bounds_camera_range_centre_collapse(void) {
    uint8_t indices[2 * 2];
    for (auto& idx : indices) idx = 1;
    TileMap4bpp map = make10x20MapA4bpp(indices);
    map.width = 2;
    map.height = 2;

    ScreenBounds world{};
    expandProjectedMapBounds(world, map, kIso);
    assertBounds(world, -32, -8, 32, 24, true);

    const CameraBounds range = cameraRangeFor(world, 240, 240);

    assertCameraBounds(range, -120, -120, -112, -112, true);
}

/// Req: Centre-collapse on undersized world -- axes collapse
/// INDEPENDENTLY. A world narrower than the viewport on Y only (X stays
/// comfortably wider) must collapse minY/maxY while leaving minX/maxX at
/// the normal, non-collapsed range. Reuses world #1's X extent
/// ({-320,160}, 480 wide) against a taller synthetic Y extent narrower
/// than a 500px-tall viewport.
void test_projected_map_bounds_camera_range_axes_collapse_independently(void) {
    const ScreenBounds world{-320, -8, 160, 92, true};  // Y extent: 100px tall.

    const CameraBounds range = cameraRangeFor(world, 240, 500);

    // X: 480 wide vs 240 viewport -- normal range, not collapsed.
    TEST_ASSERT_EQUAL_INT(-320, range.minX);
    TEST_ASSERT_EQUAL_INT(-80, range.maxX);
    // Y: 100 tall vs 500 viewport -- collapses to (-8 + (92-500))/2 = -208.
    TEST_ASSERT_EQUAL_INT(-208, range.minY);
    TEST_ASSERT_EQUAL_INT(-208, range.maxY);
    TEST_ASSERT_TRUE(range.valid);
}

/// Req: Centre-collapse rounding contract (deliberate). `(lo + hi) / 2`
/// truncates toward zero in C++, not floor -- pinned here because negative
/// `left`/`top` are the NORMAL case under an isometric basis (`axisYx` is
/// negative), not an edge case. `{left=-5, right=6}`, `viewW=12`:
/// `lo=-5, hi=6-12=-6`, inverted (`lo > hi`), collapse midpoint
/// `(-5 + -6)/2 = -11/2 = -5` under truncation (floor would give -6 -- if
/// this test ever asserts -6, someone "fixed" this to floor division and
/// silently shifted the camera by a pixel). The mirrored positive case,
/// `{left=5, right=16}`, `viewW=12`: `lo=5, hi=4`, midpoint `9/2=4`, pinning
/// that the bias direction is sign-dependent (toward `hi` for a negative
/// sum, toward `lo` for a positive one), not a fixed rounding direction.
void test_projected_map_bounds_camera_range_rounding_negative_odd(void) {
    const ScreenBounds negative{-5, -5, 6, 6, true};
    const CameraBounds negativeRange = cameraRangeFor(negative, 12, 12);
    assertCameraBounds(negativeRange, -5, -5, -5, -5, true);

    const ScreenBounds positive{5, 5, 16, 16, true};
    const CameraBounds positiveRange = cameraRangeFor(positive, 12, 12);
    assertCameraBounds(positiveRange, 4, 4, 4, 4, true);
}

/// Req: Centre-collapse on undersized world -- `valid` propagation. A
/// never-seeded `ScreenBounds{}` (`valid == false`) must produce a
/// `CameraBounds` with `valid == false` and no computed range: nothing
/// about `lo`/`hi`/collapse is meaningful when the source world was never
/// accumulated from any layer.
void test_projected_map_bounds_camera_range_invalid_passthrough(void) {
    const ScreenBounds world{};
    TEST_ASSERT_FALSE(world.valid);

    const CameraBounds range = cameraRangeFor(world, 240, 240);

    TEST_ASSERT_FALSE(range.valid);
}

void test_projected_map_bounds_zero_cost_when_disabled(void) {
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_TILEMAP_PROJECTION=1: ScreenBounds and "
        "expandProjectedMapBounds are compiled; functional coverage is above.");
}

#else

void test_projected_map_bounds_zero_cost_when_disabled(void) {
    // With the flag off, ScreenBounds and expandProjectedMapBounds are not
    // compiled at all -- graphics/ProjectedMapBounds.h's body is entirely
    // guarded out. This translation unit compiling and passing without
    // referencing either symbol IS the "zero bytes reserved" property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_TILEMAP_PROJECTION=0: ScreenBounds and "
        "expandProjectedMapBounds are not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    RUN_TEST(test_projected_map_bounds_corner_extent_with_per_tile_overhang);
    RUN_TEST(test_projected_map_bounds_sentinel_index_0_excluded);
    RUN_TEST(test_projected_map_bounds_per_tile_sprite_taller_than_cell);
    RUN_TEST(test_projected_map_bounds_per_tile_sprite_wider_than_cell);
    RUN_TEST(test_projected_map_bounds_per_tile_foot_table_varying_anchors);
    RUN_TEST(test_projected_map_bounds_unions_across_layers);
    RUN_TEST(test_projected_map_bounds_degenerate_inputs_are_no_ops);
    RUN_TEST(test_projected_map_bounds_identical_across_1bpp_2bpp_4bpp);
    RUN_TEST(test_projected_map_bounds_orthogonal_basis);
    RUN_TEST(test_projected_map_bounds_camera_range_no_collapse);
    RUN_TEST(test_projected_map_bounds_camera_range_centre_collapse);
    RUN_TEST(test_projected_map_bounds_camera_range_axes_collapse_independently);
    RUN_TEST(test_projected_map_bounds_camera_range_rounding_negative_odd);
    RUN_TEST(test_projected_map_bounds_camera_range_invalid_passthrough);
#endif
    RUN_TEST(test_projected_map_bounds_zero_cost_when_disabled);

    return UNITY_END();
}
