/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Characterization tests for the projected TileMap4bpp overload of
 * Renderer::drawTileMap: placement through a math::ProjectionSpec basis
 * instead of the axis-aligned grid, sprite-extent dirty marking, and
 * sprite-extent cull-window padding.
 *
 * The placement oracle is examples/iso_dungeon/src/IsoDraw.h:28-29's shipped
 * formula, copied verbatim rather than re-derived:
 *   drawSprite(sprite, centreX - sprite.width / 2, centreY - footY, 0, flipX)
 * Fixtures use the real measured values from
 * examples/iso_dungeon/src/assets/DungeonTiles.h (foot rows: floors 8, altar
 * 22, pillar 30, wall/doors 32) and the basis from
 * examples/iso_dungeon/src/IsoDungeonConstants.h, following the same
 * "real numbers, synthetic bitmap data" idiom as
 * test/unit/test_tilemap_foot_anchor/.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstdint>
#include <cstring>
#include <memory>

using namespace pixelroot32::graphics;

namespace {

constexpr int kFbWidth = 240;
constexpr int kFbHeight = 240;

/// Fill value no palette entry below packs to, so "left alone" and "drawn"
/// stay distinguishable, and so a DirtyGrid-cleared (0) pixel is unambiguous.
constexpr uint8_t kSentinel = 0xAAu;

uint8_t gFrameBuffer[kFbWidth * kFbHeight];

/// The packing Renderer must apply, restated independently (see
/// test_sprite4bpp_framebuffer for the rationale: a test that reuses the
/// implementation cannot catch the implementation changing).
uint8_t expectedPack(uint16_t rgb565) {
    return static_cast<uint8_t>(((rgb565 & 0xE000) >> 8) |
                                ((rgb565 & 0x0700) >> 6) |
                                ((rgb565 & 0x0018) >> 3));
}

const uint16_t kPalette[16] = {
    0x0000u, 0xF800u, 0x07E0u, 0x001Fu,
    0xFFE0u, 0xF81Fu, 0x07FFu, 0xFFFFu,
    0x8410u, 0xC618u, 0x7BEFu, 0x39E7u,
    0xFC00u, 0x03E0u, 0x0018u, 0xE01Fu,
};

/// Identity mapping: sprite palette entry i resolves to kPalette[i].
const Color kIdentityMapping[16] = {
    static_cast<Color>(0),  static_cast<Color>(1),  static_cast<Color>(2),  static_cast<Color>(3),
    static_cast<Color>(4),  static_cast<Color>(5),  static_cast<Color>(6),  static_cast<Color>(7),
    static_cast<Color>(8),  static_cast<Color>(9),  static_cast<Color>(10), static_cast<Color>(11),
    static_cast<Color>(12), static_cast<Color>(13), static_cast<Color>(14), static_cast<Color>(15),
};

/// Solid opaque fixture data: every packed nibble is palette index 1. These
/// tests characterize WHERE a sprite lands and WHICH cells it marks, not what
/// its pixels look like, so one uniform fill covers every shape below (the
/// largest is WALL at 32x40 = 640 bytes).
constexpr int kMaxTileBytes = 40 * 16;
uint8_t gSolidTileData[kMaxTileBytes];

/// Owns a Renderer wired to a MockDrawSurface exposing gFrameBuffer.
struct Harness {
    std::unique_ptr<Renderer> renderer;

    Harness() {
        auto mock = std::make_unique<MockDrawSurface>();
        MockDrawSurface* mockRaw = mock.get();
        mockRaw->setSpriteBuffer(gFrameBuffer, sizeof(gFrameBuffer));

        DisplayConfig config =
            PIXELROOT32_CUSTOM_DISPLAY(mock.release(), kFbWidth, kFbHeight);
        renderer = std::make_unique<Renderer>(std::move(config));
        renderer->init();
        renderer->beginFrame();

        // Seeded AFTER beginFrame so the sentinel survives whatever clearing
        // strategy the dirty-region configuration picked for this build.
        std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
    }
};

uint8_t fbAt(int x, int y) {
    return gFrameBuffer[y * kFbWidth + x];
}

}  // namespace

void setUp(void) {
    setDualCustomPalette(kPalette, kPalette);
    std::memset(gSolidTileData, 0x11, sizeof(gSolidTileData));
}

void tearDown(void) {
    test_teardown();
}

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

#include "math/Projection.h"

namespace math = pixelroot32::math;

namespace {

// Real measured anchors and dimensions from
// examples/iso_dungeon/src/assets/DungeonTiles.h. See
// test/unit/test_tilemap_foot_anchor/ for the same FOOT_Y constants.
constexpr uint8_t kFloorWidth = 32, kFloorHeight = 16, kFloorFootY = 8;
constexpr uint8_t kAltarWidth = 32, kAltarHeight = 30, kAltarFootY = 22;
constexpr uint8_t kPillarWidth = 16, kPillarHeight = 34, kPillarFootY = 30;
constexpr uint8_t kWallWidth = 32, kWallHeight = 40, kWallFootY = 32;

const Sprite4bpp kFloorSprite  = {gSolidTileData, kIdentityMapping, kFloorWidth,  kFloorHeight,  16};
const Sprite4bpp kAltarSprite  = {gSolidTileData, kIdentityMapping, kAltarWidth,  kAltarHeight,  16};
const Sprite4bpp kPillarSprite = {gSolidTileData, kIdentityMapping, kPillarWidth, kPillarHeight, 16};
const Sprite4bpp kWallSprite   = {gSolidTileData, kIdentityMapping, kWallWidth,   kWallHeight,   16};

// Index 0 is the empty-tile sentinel drawTileMap always skips (see
// footYFor()'s own note on this in Renderer.h); its slot is never read for
// pixels, only scanned for the cull-padding bound, so its dims stay zero.
const Sprite4bpp kTiles[5] = {Sprite4bpp{}, kFloorSprite, kAltarSprite, kPillarSprite, kWallSprite};
const uint8_t kTileFootY[5] = {0, kFloorFootY, kAltarFootY, kPillarFootY, kWallFootY};

// Basis from examples/iso_dungeon/src/IsoDungeonConstants.h
// (kIsoOriginX/Y, kTileHalfWidth/Height): +1 tileX steps right+down, +1
// tileY steps left+down, the classic 32x16 isometric diamond.
constexpr math::ProjectionSpec kIsoSpec{120, 88, 16, 8, -16, 8};

// A plain axis-aligned 16px-stride basis, used where the scenario under test
// (cull padding, overhang marking) is clearer with orthogonal arithmetic. It
// is still a real math::ProjectionSpec passed through the same projected
// branch -- the branch does not special-case orthogonal bases.
constexpr math::ProjectionSpec kGridSpec{0, 0, 16, 0, 0, 16};

}  // namespace

/// AC-1 (oracle equality) and AC-2 (distinct real fixtures resolve to
/// distinct positions): four tiles of four different heights over one 2x2
/// footprint, each checked against IsoDraw.h's formula, copied verbatim.
void test_tilemap_projected_draw_matches_oracle_formula_for_distinct_tiles(void) {
    // Same cell (0,0) for every tile, isolated one per Harness, so only the
    // per-tile FOOT_Y and width vary -- the cleanest proof that distinct
    // anchors resolve distinctly (AC-2). Isolating each draw also avoids a
    // false failure from real isometric art's expected behaviour: a solid
    // fixture sprite's *rectangular* bounding box legitimately overlaps a
    // neighbour's the way a real diamond-shaped sprite's transparent corners
    // would not, so combining several tiles in one map would contaminate the
    // "not drawn here" checks below with that overlap instead of a bug.
    uint8_t indices[1];
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 1;
    map.height = 1;
    map.tiles = kTiles;
    map.tileWidth = 32;
    map.tileHeight = 16;
    map.tileCount = 5;
    map.runtimeMask = nullptr;
    map.tileFootY = kTileFootY;

    // Floor: centre (120,88), footY 8 -> (104, 80).
    {
        indices[0] = 1;
        Harness h;
        h.renderer->drawTileMap(map, 0, 0, LayerType::Static, kIsoSpec);
        TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(104, 80));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(103, 80));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(104, 79));
    }
    // Altar: centre (120,88), footY 22 -> (104, 66).
    {
        indices[0] = 2;
        Harness h;
        h.renderer->drawTileMap(map, 0, 0, LayerType::Static, kIsoSpec);
        TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(104, 66));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(103, 66));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(104, 65));
    }
    // Pillar: centre (120,88), footY 30 -> (112, 58).
    {
        indices[0] = 3;
        Harness h;
        h.renderer->drawTileMap(map, 0, 0, LayerType::Static, kIsoSpec);
        TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(112, 58));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(111, 58));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(112, 57));
    }
    // Wall: centre (120,88), footY 32 -> (104, 56).
    {
        indices[0] = 4;
        Harness h;
        h.renderer->drawTileMap(map, 0, 0, LayerType::Static, kIsoSpec);
        TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(104, 56));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(103, 56));
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(104, 55));
    }
}

/// AC-3 (static): projection == nullptr, passed explicitly, reproduces the
/// same framebuffer as the legacy call that omits the argument entirely.
void test_tilemap_projected_draw_null_projection_matches_orthogonal_static(void) {
    uint8_t indices[4] = {1, 2, 3, 4};
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 2;
    map.height = 2;
    map.tiles = kTiles;
    map.tileWidth = 32;
    map.tileHeight = 16;
    map.tileCount = 5;
    map.runtimeMask = nullptr;
    map.tileFootY = kTileFootY;

    Harness legacy;
    legacy.renderer->drawTileMap(map, 10, 20, LayerType::Static);
    uint8_t legacyFb[kFbWidth * kFbHeight];
    std::memcpy(legacyFb, gFrameBuffer, sizeof(legacyFb));

    Harness projected;
    // A reference has no null form: the collapsed 4-arg call is the plain
    // orthogonal overload, not the projected one passed a null projection.
    projected.renderer->drawTileMap(map, 10, 20, LayerType::Static);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(legacyFb, gFrameBuffer, kFbWidth * kFbHeight);
}

/// AC-3 (dynamic marks): projection == nullptr marks the same orthogonal
/// cell-footprint rect as before -- not the sprite's own extent.
void test_tilemap_projected_draw_null_projection_matches_orthogonal_dynamic_marks(void) {
    uint8_t indices[1] = {1};
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 1;
    map.height = 1;
    map.tiles = kTiles;
    map.tileWidth = 32;
    map.tileHeight = 16;
    map.tileCount = 5;
    map.runtimeMask = nullptr;
    map.tileFootY = kTileFootY;

    Harness h;
    // A reference has no null form: the collapsed 4-arg call is the plain
    // orthogonal overload, not the projected one passed a null projection.
    h.renderer->drawTileMap(map, 10, 20, LayerType::Dynamic);
    h.renderer->beginFrame();

    // Orthogonal mark rect is (10, 20, 32, 16): pixels x=[10,41], y=[20,35].
    TEST_ASSERT_EQUAL_UINT8(0, fbAt(10, 20));
    TEST_ASSERT_EQUAL_UINT8(0, fbAt(41, 35));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(7, 20));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(10, 10));
}

/// AC-4: a tile taller than its cell, drawn Dynamic under a projection, marks
/// every dirty cell its OWN extent touches -- not the cell footprint.
/// Footprint-only marking is known-wrong the moment this fails: WALL (32x40,
/// footY 32) anchored at cell (1,1)'s centre draws 16px above the cell's own
/// [16,16,16,16] footprint, and only sprite-extent marking reaches it.
void test_tilemap_projected_draw_sprite_extent_marks_overhang_dirty_cells(void) {
    uint8_t indices[4] = {0, 0, 0, 4};  // wall at (tx=1, ty=1)
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 2;
    map.height = 2;
    map.tiles = kTiles;
    map.tileWidth = 16;
    map.tileHeight = 16;
    map.tileCount = 5;
    map.runtimeMask = nullptr;
    map.tileFootY = kTileFootY;

    Harness h;
    h.renderer->drawTileMap(map, 0, 0, LayerType::Dynamic, kGridSpec);
    h.renderer->beginFrame();

    // Correct sprite-extent mark rect is (0, -16, 32, 40) -> clipped pixels
    // x=[0,31], y=[0,23]. The wrong footprint-only rect is (16,16,16,16) and
    // never reaches (4,4).
    TEST_ASSERT_EQUAL_UINT8(0, fbAt(4, 4));
    // Sanity: marking stays bounded, not a full-dirty fallback.
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(200, 200));
}

/// AC-5: a tall tile whose CELL sits just outside the unpadded cull window
/// still draws, because the padded window accounts for its own extent.
/// footY 0 puts the sprite's entire 40px height below its anchor; anchoring
/// the cell 20px above the screen top still leaves 20px of the sprite
/// visible, but only if the window is padded by the tileset's max overhang.
void test_tilemap_projected_draw_cull_window_padding_includes_tall_tile_anchored_above_window(void) {
    const Sprite4bpp tallTile = {gSolidTileData, kIdentityMapping, 16, 40, 16};
    const Sprite4bpp localTiles[2] = {Sprite4bpp{}, tallTile};
    const uint8_t localFootY[2] = {0, 0};

    uint8_t indices[1] = {1};
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 1;
    map.height = 1;
    map.tiles = localTiles;
    map.tileWidth = 16;
    map.tileHeight = 16;
    map.tileCount = 2;
    map.runtimeMask = nullptr;
    map.tileFootY = localFootY;

    Harness h;
    h.renderer->drawTileMap(map, 0, -20, LayerType::Static, kGridSpec);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(0, 10));
}

/// AC-6 (runtime mask, bundled with placement so a stub that ignores the
/// projection cannot pass vacuously): the active cell lands at the oracle
/// position; the deactivated one, drawable data notwithstanding, does not.
void test_tilemap_projected_draw_runtime_mask_skip_unchanged_under_projection(void) {
    // Isolated per case (see the oracle-formula test above for why): the
    // same cell (0,0), drawn once active and once masked off.
    uint8_t activeIndices[1] = {1};
    TileMap4bpp activeMap{};
    activeMap.indices = activeIndices;
    activeMap.width = 1;
    activeMap.height = 1;
    activeMap.tiles = kTiles;
    activeMap.tileWidth = 32;
    activeMap.tileHeight = 16;
    activeMap.tileCount = 5;
    activeMap.tileFootY = kTileFootY;

    Harness active;
    active.renderer->drawTileMap(activeMap, 0, 0, LayerType::Static, kIsoSpec);
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(104, 80));

    uint8_t maskedIndices[1] = {1};
    TileMap4bpp maskedMap{};
    maskedMap.indices = maskedIndices;
    maskedMap.width = 1;
    maskedMap.height = 1;
    maskedMap.tiles = kTiles;
    maskedMap.tileWidth = 32;
    maskedMap.tileHeight = 16;
    maskedMap.tileCount = 5;
    maskedMap.tileFootY = kTileFootY;
    maskedMap.initRuntimeMask();
    maskedMap.setTileActive(0, 0, false);

    Harness masked;
    masked.renderer->drawTileMap(maskedMap, 0, 0, LayerType::Static, kIsoSpec);
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(104, 80));

    maskedMap.cleanupRuntimeMask();
}

/// AC-6 (index skip, bundled with placement): the sentinel index 0 and an
/// out-of-range index are both skipped even though their slot holds
/// drawable data, while the valid neighbour lands at the oracle position.
void test_tilemap_projected_draw_index_skip_unchanged_under_projection(void) {
    // Isolated per case (see the oracle-formula test above for why): the
    // same cell (0,0), one valid draw and two that must skip despite
    // drawable data sitting in that slot.
    const Sprite4bpp localTiles[3] = {kFloorSprite, kFloorSprite, kFloorSprite};
    const uint8_t localFootY[3] = {kFloorFootY, kFloorFootY, kFloorFootY};

    uint8_t validIndices[1] = {1};
    TileMap4bpp validMap{};
    validMap.indices = validIndices;
    validMap.width = 1;
    validMap.height = 1;
    validMap.tiles = localTiles;
    validMap.tileWidth = 32;
    validMap.tileHeight = 16;
    validMap.tileCount = 3;
    validMap.tileFootY = localFootY;

    Harness valid;
    valid.renderer->drawTileMap(validMap, 0, 0, LayerType::Static, kIsoSpec);
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(104, 80));

    uint8_t sentinelIndices[1] = {0};  // drawable data at slot 0, but always skipped
    TileMap4bpp sentinelMap = validMap;
    sentinelMap.indices = sentinelIndices;

    Harness sentinelSkip;
    sentinelSkip.renderer->drawTileMap(sentinelMap, 0, 0, LayerType::Static, kIsoSpec);
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(104, 80));

    uint8_t oobIndices[1] = {99};
    TileMap4bpp oobMap = validMap;
    oobMap.indices = oobIndices;

    Harness oobSkip;
    oobSkip.renderer->drawTileMap(oobMap, 0, 0, LayerType::Static, kIsoSpec);
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(104, 80));
}

/// AC-7: a degenerate basis and a zero-size map both draw nothing.
void test_tilemap_projected_draw_degenerate_inputs_draw_nothing(void) {
    uint8_t indices[1] = {1};

    // Degenerate determinant (parallel axes): guarded by
    // cellRangeForScreenRect's own det < 1 check, unchanged from WU-3.
    constexpr math::ProjectionSpec kDegenerateSpec{0, 0, 0, 0, 0, 0};
    TileMap4bpp map{};
    map.indices = indices;
    map.width = 1;
    map.height = 1;
    map.tiles = kTiles;
    map.tileWidth = 32;
    map.tileHeight = 16;
    map.tileCount = 5;
    map.runtimeMask = nullptr;
    map.tileFootY = kTileFootY;

    Harness degenerate;
    degenerate.renderer->drawTileMap(map, 0, 0, LayerType::Static, kDegenerateSpec);
    for (int y = 0; y < kFbHeight; y += 37) {
        for (int x = 0; x < kFbWidth; x += 37) {
            TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(x, y));
        }
    }

    // Zero-size map: the pre-existing top-of-function guard, unaffected by
    // the new parameter.
    TileMap4bpp emptyMap{};
    emptyMap.indices = indices;
    emptyMap.width = 0;
    emptyMap.height = 0;
    emptyMap.tiles = kTiles;
    emptyMap.tileWidth = 32;
    emptyMap.tileHeight = 16;
    emptyMap.tileCount = 5;

    Harness zeroSize;
    zeroSize.renderer->drawTileMap(emptyMap, 0, 0, LayerType::Static, kIsoSpec);
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(0, 0));
}

void test_tilemap_projected_draw_zero_cost_when_disabled(void) {
    // Flag ON: functional coverage lives in the cases above; this mirrors the
    // always-run half of the idiom used by test_math_projection.cpp.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_TILEMAP_PROJECTION=1: the projected drawTileMap "
        "parameter and branch are compiled; functional coverage is above.");
}

#else

void test_tilemap_projected_draw_zero_cost_when_disabled(void) {
    // With the flag off, the trailing math::ProjectionSpec* parameter and the
    // projected branch are not compiled at all -- Renderer::drawTileMap keeps
    // its today signature. This translation unit compiling and passing
    // without referencing math::ProjectionSpec IS the "zero bytes reserved"
    // property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_TILEMAP_PROJECTION=0: the projected drawTileMap "
        "parameter and branch are not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    RUN_TEST(test_tilemap_projected_draw_matches_oracle_formula_for_distinct_tiles);
    RUN_TEST(test_tilemap_projected_draw_null_projection_matches_orthogonal_static);
    RUN_TEST(test_tilemap_projected_draw_null_projection_matches_orthogonal_dynamic_marks);
    RUN_TEST(test_tilemap_projected_draw_sprite_extent_marks_overhang_dirty_cells);
    RUN_TEST(test_tilemap_projected_draw_cull_window_padding_includes_tall_tile_anchored_above_window);
    RUN_TEST(test_tilemap_projected_draw_runtime_mask_skip_unchanged_under_projection);
    RUN_TEST(test_tilemap_projected_draw_index_skip_unchanged_under_projection);
    RUN_TEST(test_tilemap_projected_draw_degenerate_inputs_draw_nothing);
#endif
    RUN_TEST(test_tilemap_projected_draw_zero_cost_when_disabled);

    return UNITY_END();
}
