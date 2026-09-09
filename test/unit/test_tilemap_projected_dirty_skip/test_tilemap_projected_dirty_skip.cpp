/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Integration tests for the per-tile dirty-skip optimization in
 * Renderer::drawTileMapProjectedImpl (change iso-perf-cached-ground).
 *
 * ## What the predicate under test actually does
 *
 * `drawTileMapProjectedImpl` skips blitting a tile when ALL of these hold:
 *   - PIXELROOT32_ENABLE_DIRTY_REGIONS is on (`if constexpr` gate), and
 *   - the layer is LayerType::Dynamic, and
 *   - `selectiveRestoreValidThisFrame_` is true, i.e. beginFrame() blanked
 *     only the prev-dirty cells rather than the whole framebuffer, and
 *   - the camera has not scrolled: `xOffset == prevXOffset_ &&
 *     yOffset == prevYOffset_` (prev* snapshotted by the last endFrame()), and
 *   - the tile's screen rect intersects NO prev-dirty 8x8 cell.
 *
 * Its whole point is that such a tile's pixels are already correct from last
 * frame, so re-blitting them is wasted work.
 *
 * ## Why the fixture is shaped the way it is
 *
 * A test can only observe the skip if the framebuffer distinguishes "written
 * this frame" from "left alone". So:
 *
 *   1. A MockDrawSurface exposes gFrameBuffer as the 8bpp sprite buffer, which
 *      is what makes dirty regions run at all in beginFrame() (the non-8bpp
 *      path bails to a full clearBuffer()).
 *   2. Every frame re-seeds gFrameBuffer to kSentinel AFTER beginFrame() and
 *      BEFORE drawing, so a pixel that still reads kSentinel afterwards was
 *      provably not blitted.
 *   3. The tile bitmap is filled with 0x11 -- BOTH 4bpp nibbles are palette
 *      index 1, and index 0 is the transparent index the blitter skips. A
 *      zero-filled bitmap (the previous version of this file) is 100%
 *      transparent, so nothing is ever written and every "sentinel survived"
 *      assertion passes vacuously. Every "was blitted" assertion below is
 *      therefore reachable only because the fixture data is opaque.
 *   4. The map is SPARSE: exactly two non-zero cells, placed far enough apart
 *      on screen that their 32x16 rects land in disjoint sets of 8x8 dirty
 *      cells. That is the only way one tile can intersect prev-dirty while
 *      the other does not; a dense 2x2 iso map (the previous fixture) packs
 *      its tile centres within 16 px, so every tile always intersects and
 *      NOTHING can ever be skipped.
 *
 * Each case then probes ONE pixel per tile and asserts the exact expected
 * byte: `expectedPack(kPalette[1])` for "blitted", `kSentinel` for "skipped".
 * A single pixel probe is the decisive discriminator -- whole-framebuffer byte
 * counts are not, because these fixtures cover only ~2 kB of a 57.6 kB buffer
 * whether or not the predicate engages.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "platforms/EngineConfig.h"
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

/// Same identity palette as test_tilemap_projected_draw: index i -> kPalette[i].
const uint16_t kPalette[16] = {
    0x0000u, 0xF800u, 0x07E0u, 0x001Fu,
    0xFFE0u, 0xF81Fu, 0x07FFu, 0xFFFFu,
    0x8410u, 0xC618u, 0x7BEFu, 0x39E7u,
    0xFC00u, 0x03E0u, 0x0018u, 0xE01Fu,
};
const Color kIdentityMapping[16] = {
    static_cast<Color>(0),  static_cast<Color>(1),  static_cast<Color>(2),  static_cast<Color>(3),
    static_cast<Color>(4),  static_cast<Color>(5),  static_cast<Color>(6),  static_cast<Color>(7),
    static_cast<Color>(8),  static_cast<Color>(9),  static_cast<Color>(10), static_cast<Color>(11),
    static_cast<Color>(12), static_cast<Color>(13), static_cast<Color>(14), static_cast<Color>(15),
};

/// Real measured floor dimensions from examples/iso_dungeon/src/assets.
constexpr uint8_t kFloorWidth = 32, kFloorHeight = 16, kFloorFootY = 8;
constexpr int kMaxTileBytes = 40 * 16;

/// Opaque fixture data. 0x11 = both 4bpp nibbles are palette index 1; index 0
/// is the transparent index drawSpriteInternal skips, so a zero fill would
/// make every blit a no-op and every assertion below unfalsifiable.
uint8_t gSolidTileData[kMaxTileBytes];

uint8_t fbAt(int x, int y) {
    return gFrameBuffer[y * kFbWidth + x];
}

}  // namespace

void setUp(void) {
    setDualCustomPalette(kPalette, kPalette);
    std::memset(gSolidTileData, 0x11, sizeof(gSolidTileData));
    std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
}

void tearDown(void) {
    test_teardown();
}

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION && PIXELROOT32_ENABLE_DIRTY_REGIONS

#include "math/Projection.h"
namespace math = pixelroot32::math;

namespace {

const Sprite4bpp kFloorSprite = {gSolidTileData, kIdentityMapping, kFloorWidth, kFloorHeight, 16};

/// Index 0 is the empty-tile sentinel drawTileMap always skips, which is what
/// lets the map below stay sparse.
const Sprite4bpp kTiles[2] = {Sprite4bpp{}, kFloorSprite};
const uint8_t kTileFootY[2] = {0, kFloorFootY};

/// Basis from examples/iso_dungeon/src/IsoDungeonConstants.h.
/// cellToScreen(tx, ty) = (120 + 16*tx - 16*ty, 88 + 8*tx + 8*ty); the blit
/// anchor is that centre minus (width/2, footY) = minus (16, 8).
constexpr math::ProjectionSpec kIsoSpec{120, 88, 16, 8, -16, 8};

/// A plain axis-aligned basis with a non-zero origin, so the same two-cell
/// fixture lands wholly on screen under an orthogonal projection too.
/// cellToScreen(tx, ty) = (48 + 32*tx, 32 + 16*ty).
constexpr math::ProjectionSpec kOrthoSpec{48, 32, 32, 0, 0, 16};

constexpr int kMapW = 8, kMapH = 8;

// --- Iso geometry, derived from kIsoSpec by hand (see the spec comment) ---
// NEAR cell (0,0): centre (120, 88) -> rect [104,136) x [80,96)
//                  -> 8x8 cells cx 13..16, cy 10..11
// FAR  cell (7,7): centre (120,200) -> rect [104,136) x [192,208)
//                  -> 8x8 cells cx 13..16, cy 24..25
// The two cell sets are disjoint in cy, so a prev-dirty region covering one
// tile provably does not reach the other. That disjointness is the entire
// reason the skip predicate can be observed at all.
constexpr int kIsoNearCellX = 0, kIsoNearCellY = 0;
constexpr int kIsoFarCellX = 7, kIsoFarCellY = 7;
constexpr int kIsoNearProbeX = 120, kIsoNearProbeY = 88;   // inside the near rect
constexpr int kIsoFarProbeX = 120, kIsoFarProbeY = 200;    // inside the far rect
/// One 8x8 cell strictly inside the FAR tile's rect and outside the NEAR
/// tile's: (120..127, 192..199).
constexpr uint8_t kIsoFarDirtyCellX = 15, kIsoFarDirtyCellY = 24;

// --- Ortho geometry, derived from kOrthoSpec the same way ---
// NEAR cell (0,0): centre (48, 32)  -> rect [32,64)   x [24,40)
//                  -> cells cx 4..7,   cy 3..4
// FAR  cell (5,5): centre (208,112) -> rect [192,224) x [104,120)
//                  -> cells cx 24..27, cy 13..14   (disjoint from the near set)
constexpr int kOrthoFarCellX = 5, kOrthoFarCellY = 5;
constexpr int kOrthoNearProbeX = 48, kOrthoNearProbeY = 32;
constexpr int kOrthoFarProbeX = 208, kOrthoFarProbeY = 112;

/// A map whose cells are all empty (index 0) until a test opts specific cells
/// in. Sparsity is load-bearing: it is what puts two tiles in disjoint dirty
/// cells (a dense iso map's tiles overlap within 16 px and always intersect).
struct SparseMap {
    uint8_t indices[kMapW * kMapH]{};
    TileMap4bpp map{};

    SparseMap() {
        map.indices = indices;
        map.width = kMapW;
        map.height = kMapH;
        map.tiles = kTiles;
        map.tileWidth = 32;
        map.tileHeight = 16;
        map.tileCount = 2;
        map.runtimeMask = nullptr;
        map.tileFootY = kTileFootY;
    }

    /// Puts the opaque floor tile (index 1) in cell (cx, cy).
    SparseMap& with(int cx, int cy) {
        indices[cy * kMapW + cx] = 1;
        return *this;
    }
};

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
    }

    /// Opens a frame and re-seeds the sentinel AFTER beginFrame(), so whatever
    /// clearing strategy beginFrame() picked cannot be mistaken for a blit.
    void beginFrameWithSentinel(int offsetX = 0, int offsetY = 0) {
        renderer->setDisplayOffset(offsetX, offsetY);
        renderer->beginFrame();
        std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
    }

    /// Settling frame: draws `m` as Dynamic so its tiles mark curr-dirty
    /// cells, then closes the frame. endFrame() snapshots (offsetX, offsetY)
    /// into prevXOffset_/prevYOffset_, and the next beginFrame() swaps those
    /// marks into prev -- which makes countPrevMarkedCells() > 0 and therefore
    /// selectiveRestoreValidThisFrame_ true (Renderer.cpp beginFrame()).
    void settleByDrawing(const SparseMap& m, int offsetX = 0, int offsetY = 0) {
        beginFrameWithSentinel(offsetX, offsetY);
        renderer->drawTileMap(m.map, 0, 0, LayerType::Dynamic, kIsoSpec);
        renderer->endFrame();
    }

    /// Settling frame that marks ONE cell directly instead of drawing, so the
    /// prev-dirty set is exactly what the test names -- no dependence on which
    /// cells a blit happens to touch.
    void settleByMarkingCell(uint8_t cx, uint8_t cy) {
        beginFrameWithSentinel(0, 0);
        renderer->markCellDirtyForTest(cx, cy);
        renderer->endFrame();
    }
};

}  // namespace

/// AC: with the camera stationary and selective restore valid, a Dynamic tile
/// whose rect intersects a prev-dirty cell is blitted, and one whose rect
/// intersects none is skipped.
///
/// Both directions are asserted from the SAME draw call, which is what makes
/// the case discriminating: "some sentinel survived" alone would pass even if
/// nothing were ever skipped, because these two tiles cover ~2 kB of a 57.6 kB
/// framebuffer.
void test_idle_skip_blits_prev_dirty_tiles_and_skips_the_rest(void) {
    // Settling frame draws ONLY the near tile, so only its cells (cy 10..11)
    // end up in prev.
    SparseMap settleMap;
    settleMap.with(kIsoNearCellX, kIsoNearCellY);
    // Test frame draws both tiles.
    SparseMap testMap;
    testMap.with(kIsoNearCellX, kIsoNearCellY).with(kIsoFarCellX, kIsoFarCellY);

    Harness h;
    h.settleByDrawing(settleMap, 0, 0);

    h.beginFrameWithSentinel(0, 0);  // same offset as settle -> gate holds
    h.renderer->drawTileMap(testMap.map, 0, 0, LayerType::Dynamic, kIsoSpec);

    // Near tile overlaps prev-dirty cells -> must be blitted.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(kIsoNearProbeX, kIsoNearProbeY));
    // Far tile (cy 24..25) overlaps no prev-dirty cell -> must be skipped.
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(kIsoFarProbeX, kIsoFarProbeY));
}

/// AC: a camera scroll between frames disables the skip predicate, because
/// last frame's pixels are no longer where this frame needs them.
///
/// Identical fixture to the idle case; the ONLY change is that the test frame
/// runs at yOffset 8 while endFrame() snapshotted prevYOffset_ = 0, so
/// `yOffset == prevYOffset_` fails. The far tile's rect still intersects no
/// prev-dirty cell -- it would be skipped if the gate held -- so asserting it
/// was blitted isolates the offset gate and nothing else.
///
/// Deliberately NOT asserted: that the whole 57.6 kB framebuffer was
/// overwritten. That was the original (unsatisfiable) assertion -- two 32x16
/// tiles can touch at most 1024 bytes.
void test_camera_scroll_disables_skip_for_a_non_intersecting_tile(void) {
    SparseMap settleMap;
    settleMap.with(kIsoNearCellX, kIsoNearCellY);
    SparseMap testMap;
    testMap.with(kIsoNearCellX, kIsoNearCellY).with(kIsoFarCellX, kIsoFarCellY);

    Harness h;
    h.settleByDrawing(settleMap, 0, 0);  // prevXOffset_/prevYOffset_ = (0, 0)

    h.beginFrameWithSentinel(0, 8);  // yOffset 8 != prevYOffset_ 0 -> gate fails
    h.renderer->drawTileMap(testMap.map, 0, 0, LayerType::Dynamic, kIsoSpec);

    // The blit applies yOffset, so both probes shift down by 8 px.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]),
                            fbAt(kIsoNearProbeX, kIsoNearProbeY + 8));
    // The decisive one: the tile the idle case proved is skippable is blitted
    // here purely because the camera moved.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]),
                            fbAt(kIsoFarProbeX, kIsoFarProbeY + 8));
}

/// AC: LayerType::Static never engages the skip predicate.
///
/// Same fixture and same frame sequence as the idle case, so the far tile is
/// in exactly the state that gets it skipped there; only the LayerType
/// differs. Asserting it was blitted is therefore a direct contrast with
/// test_idle_skip_..., not a smoke test. (The previous version of this case
/// ended in TEST_PASS() and asserted nothing at all.)
void test_static_layer_is_never_skipped(void) {
    SparseMap settleMap;
    settleMap.with(kIsoNearCellX, kIsoNearCellY);
    SparseMap testMap;
    testMap.with(kIsoNearCellX, kIsoNearCellY).with(kIsoFarCellX, kIsoFarCellY);

    Harness h;
    h.settleByDrawing(settleMap, 0, 0);

    h.beginFrameWithSentinel(0, 0);
    h.renderer->drawTileMap(testMap.map, 0, 0, LayerType::Static, kIsoSpec);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(kIsoNearProbeX, kIsoNearProbeY));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(kIsoFarProbeX, kIsoFarProbeY));
}

/// AC: projection-agnosticism -- the predicate keys off the tile's screen
/// rect, not off any isometric assumption, so an orthogonal ProjectionSpec
/// shows the same blit/skip split.
void test_skip_works_with_orthogonal_projection(void) {
    SparseMap settleMap;
    settleMap.with(0, 0);
    SparseMap testMap;
    testMap.with(0, 0).with(kOrthoFarCellX, kOrthoFarCellY);

    Harness h;
    // Settling frame must use the same spec, otherwise the prev-dirty cells
    // would describe a different screen layout than the one under test.
    h.beginFrameWithSentinel(0, 0);
    h.renderer->drawTileMap(settleMap.map, 0, 0, LayerType::Dynamic, kOrthoSpec);
    h.renderer->endFrame();

    h.beginFrameWithSentinel(0, 0);
    h.renderer->drawTileMap(testMap.map, 0, 0, LayerType::Dynamic, kOrthoSpec);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(kOrthoNearProbeX, kOrthoNearProbeY));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(kOrthoFarProbeX, kOrthoFarProbeY));
}

/// AC: the predicate reads the actual prev-dirty cell set, not "whichever
/// tiles were drawn last frame".
///
/// This is the exact inverse of test_idle_skip_...: the settling frame draws
/// nothing and instead marks one cell inside the FAR tile's rect, so the FAR
/// tile must blit and the NEAR tile must be skipped. Flipping which tile
/// survives while the map and the draw call stay identical is what proves the
/// dirty-cell lookup drives the decision. (The previous version of this case
/// named markCellDirtyForTest but never called it, and merely re-ran the idle
/// scenario with a `sentinel > 0` assertion.)
void test_marked_cell_selects_which_tile_blits(void) {
    SparseMap testMap;
    testMap.with(kIsoNearCellX, kIsoNearCellY).with(kIsoFarCellX, kIsoFarCellY);

    Harness h;
    h.settleByMarkingCell(kIsoFarDirtyCellX, kIsoFarDirtyCellY);

    h.beginFrameWithSentinel(0, 0);
    h.renderer->drawTileMap(testMap.map, 0, 0, LayerType::Dynamic, kIsoSpec);

    // Near tile no longer intersects any prev-dirty cell -> skipped.
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(kIsoNearProbeX, kIsoNearProbeY));
    // Far tile contains the one marked cell -> blitted.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(kIsoFarProbeX, kIsoFarProbeY));
}

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION && PIXELROOT32_ENABLE_DIRTY_REGIONS

int runUnityTests() {
    UNITY_BEGIN();
#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION && PIXELROOT32_ENABLE_DIRTY_REGIONS
    RUN_TEST(test_idle_skip_blits_prev_dirty_tiles_and_skips_the_rest);
    RUN_TEST(test_camera_scroll_disables_skip_for_a_non_intersecting_tile);
    RUN_TEST(test_static_layer_is_never_skipped);
    RUN_TEST(test_skip_works_with_orthogonal_projection);
    RUN_TEST(test_marked_cell_selects_which_tile_blits);
#endif
    return UNITY_END();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return runUnityTests();
}
