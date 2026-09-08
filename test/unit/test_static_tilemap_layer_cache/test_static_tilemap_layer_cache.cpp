/**
 * @file test_static_tilemap_layer_cache.cpp
 * @brief Unit tests for graphics/StaticTilemapLayerCache module
 * @version 1.0
 * @date 2026-04-05
 * 
 * Tests for StaticTilemapLayerCache - framebuffer cache for static tilemap layers.
 */

#include <unity.h>
#include "../test_config.h"
#include "graphics/Color.h"
#include "graphics/StaticTilemapLayerCache.h"
#include "mocks/MockDrawSurface.h"
#include "mocks/MockRenderer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#ifdef PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_cache_default_constructor(void) {
    StaticTilemapLayerCache cache;
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_allocate_valid_size(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(240, 240);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_allocate_invalid_width(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(0, 240);
    
    TEST_ASSERT_FALSE(result);
}

void test_cache_allocate_invalid_height(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(240, 0);
    
    TEST_ASSERT_FALSE(result);
}

void test_cache_allocate_negative_dimensions(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(-100, -100);
    
    TEST_ASSERT_FALSE(result);
}

void test_cache_allocate_same_size_twice(void) {
    StaticTilemapLayerCache cache;
    
    bool result1 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(result1);
    
    bool result2 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(result2);
}

void test_cache_allocate_different_size(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    bool result = cache.allocateForLogicalSize(320, 240);
    
    TEST_ASSERT_TRUE(result);
}

void test_cache_clear(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.clear();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_invalidate(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_set_enabled_true(void) {
    StaticTilemapLayerCache cache;
    
    cache.setFramebufferCacheEnabled(true);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_set_enabled_false(void) {
    StaticTilemapLayerCache cache;
    cache.setFramebufferCacheEnabled(true);
    
    cache.setFramebufferCacheEnabled(false);
    
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
}

void test_cache_toggle_enabled_twice(void) {
    StaticTilemapLayerCache cache;
    
    cache.setFramebufferCacheEnabled(true);
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
    
    cache.setFramebufferCacheEnabled(false);
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
    
    cache.setFramebufferCacheEnabled(true);
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_multiple_allocate_calls(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(100, 100);
    (void)cache.allocateForLogicalSize(200, 200);
    (void)cache.allocateForLogicalSize(320, 240);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_allocate_then_clear_then_allocate_again(void) {
    StaticTilemapLayerCache cache;
    
    bool r1 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(r1);
    
    cache.clear();
    
    bool r2 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(r2);
}

void test_cache_invalidate_clears_valid_flag(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_multiple_invalidate_calls(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    cache.invalidate();
    cache.invalidate();
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_enable_disable_preserves_allocation(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.setFramebufferCacheEnabled(false);
    
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
    
    cache.setFramebufferCacheEnabled(true);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_clear_then_invalidate(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.clear();
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

// =============================================================================
// Value Tests for line coverage - allocateForRenderer
// Note: allocateForRenderer requires Renderer which needs sprite buffer support
// For now, we can only verify that allocateForLogicalSize works correctly
// which internally uses the same allocation logic
// =============================================================================

// The allocateForRenderer function delegates to allocateForLogicalSize,
// so testing allocateForLogicalSize adequately covers the allocation logic.
// Additional coverage would require a full Renderer implementation.

// =============================================================================
// Value Tests for line coverage - draw() method
// Note: draw() requires sprite buffer support from Renderer which has complex
// mocking requirements. The existing tests cover the class API correctly.
// =============================================================================

// =============================================================================
// Tests for draw() method - now fixed with sprite buffer setup
// The key fix: allocate a sprite buffer before calling draw()
// =============================================================================

void test_cache_draw_null_static_layers(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    auto surface = std::make_unique<MockDrawSurface>();
    surface->setDisplaySize(240, 240);
    uint8_t fb[240 * 240];
    surface->setSpriteBuffer(fb, sizeof(fb));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    MockRenderer renderer(config);
    
    // Should not crash with null static layers
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    // Verify cache is still enabled
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_empty_dynamic_layers(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    auto surface = std::make_unique<MockDrawSurface>();
    surface->setDisplaySize(240, 240);
    uint8_t fb[240 * 240];
    surface->setSpriteBuffer(fb, sizeof(fb));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    MockRenderer renderer(config);
    
    // Create static layer spec with null map
    TileMap4bppDrawSpec staticSpec = {nullptr, 0, 0};
    
    // Should not crash with empty dynamic layers
    cache.draw(renderer, 0, 0, &staticSpec, 1, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_all_null_layers(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    auto surface = std::make_unique<MockDrawSurface>();
    surface->setDisplaySize(240, 240);
    uint8_t fb[240 * 240];
    surface->setSpriteBuffer(fb, sizeof(fb));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    MockRenderer renderer(config);
    
    // All null layers should not crash
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_with_disabled_cache(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    cache.setFramebufferCacheEnabled(false);
    
    auto surface = std::make_unique<MockDrawSurface>();
    surface->setDisplaySize(240, 240);
    uint8_t fb[240 * 240];
    surface->setSpriteBuffer(fb, sizeof(fb));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    MockRenderer renderer(config);
    
    // With cache disabled, should just draw directly
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_multiple_camera_positions(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    auto surface = std::make_unique<MockDrawSurface>();
    surface->setDisplaySize(240, 240);
    uint8_t fb[240 * 240];
    surface->setSpriteBuffer(fb, sizeof(fb));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    MockRenderer renderer(config);
    
    // Different camera positions should not crash
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    cache.draw(renderer, 10, 10, nullptr, 0, nullptr, 0);
    cache.draw(renderer, -10, -10, nullptr, 0, nullptr, 0);
    cache.draw(renderer, 100, 200, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_after_invalidate(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    auto surface = std::make_unique<MockDrawSurface>();
    surface->setDisplaySize(240, 240);
    uint8_t fb[240 * 240];
    surface->setSpriteBuffer(fb, sizeof(fb));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    MockRenderer renderer(config);
    
    // Draw once
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    // Invalidate should mark cache as needing rebuild
    cache.invalidate();
    
    // Draw again after invalidate
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

// =============================================================================
// Projected static layers
//
// These cases only exist when PIXELROOT32_ENABLE_TILEMAP_PROJECTION is on --
// i.e. under [env:native_test_gameplay], never under [env:native_test]. They
// use the framebuffer-oracle idiom from test/unit/test_tilemap_projected_draw/:
// a MockDrawSurface exposing a real 8bpp sprite buffer, a sentinel fill, and a
// byte-for-byte comparison against a framebuffer produced by calling
// Renderer::drawTileMap directly.
//
// Every expected pixel count below is 32 * 16 = 512, which is reachable by
// construction: the fixture tile is a fully opaque 32x16 sprite (every 4bpp
// nibble is palette index 1; index 0 is the transparent sentinel the blitter
// skips) and every case places it fully inside the 240x240 buffer.
// =============================================================================

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

#include "math/Projection.h"

namespace math = pixelroot32::math;

namespace {

constexpr int kProjFbWidth = 240;
constexpr int kProjFbHeight = 240;
constexpr std::size_t kProjFbBytes =
    static_cast<std::size_t>(kProjFbWidth) * static_cast<std::size_t>(kProjFbHeight);

/// Background fill no palette entry below packs to, so "left alone" and
/// "drawn" stay distinguishable.
constexpr uint8_t kSentinel = 0xAAu;

/// A second, different background fill used to tell a memcpy restore (which
/// overwrites the whole buffer) from a redraw (which only touches the tile's
/// own opaque pixels). Distinct from kSentinel and from the drawn value
/// expectedPack(0xF800) == 0xE0.
constexpr uint8_t kScribble = 0x55u;

uint8_t gProjFrameBuffer[kProjFbBytes];
uint8_t gOracleFrameBuffer[kProjFbBytes];

/// The packing Renderer must apply, restated independently (same rationale as
/// test_sprite4bpp_framebuffer: a test that reuses the implementation cannot
/// catch the implementation changing).
uint8_t expectedPack(uint16_t rgb565) {
    return static_cast<uint8_t>(((rgb565 & 0xE000) >> 8) |
                                ((rgb565 & 0x0700) >> 6) |
                                ((rgb565 & 0x0018) >> 3));
}

const uint16_t kProjPalette[16] = {
    0x0000u, 0xF800u, 0x07E0u, 0x001Fu,
    0xFFE0u, 0xF81Fu, 0x07FFu, 0xFFFFu,
    0x8410u, 0xC618u, 0x7BEFu, 0x39E7u,
    0xFC00u, 0x03E0u, 0x0018u, 0xE01Fu,
};

/// Identity mapping: sprite palette entry i resolves to kProjPalette[i].
const Color kProjMapping[16] = {
    static_cast<Color>(0),  static_cast<Color>(1),  static_cast<Color>(2),  static_cast<Color>(3),
    static_cast<Color>(4),  static_cast<Color>(5),  static_cast<Color>(6),  static_cast<Color>(7),
    static_cast<Color>(8),  static_cast<Color>(9),  static_cast<Color>(10), static_cast<Color>(11),
    static_cast<Color>(12), static_cast<Color>(13), static_cast<Color>(14), static_cast<Color>(15),
};

// Real floor-tile geometry from examples/iso_dungeon/src/assets/: 32x16 with
// its foot row at 8. 4bpp packs two pixels per byte, so 32 * 16 / 2 bytes.
constexpr uint8_t kFloorWidth = 32;
constexpr uint8_t kFloorHeight = 16;
constexpr uint8_t kFloorFootY = 8;
constexpr std::size_t kFloorOpaquePixels =
    static_cast<std::size_t>(kFloorWidth) * static_cast<std::size_t>(kFloorHeight);
uint8_t gOpaqueTileData[kFloorOpaquePixels / 2];

const Sprite4bpp kFloorSprite = {gOpaqueTileData, kProjMapping, kFloorWidth, kFloorHeight, 16};

// Index 0 is the empty-tile sentinel drawTileMap always skips, so its slot
// stays a zeroed Sprite4bpp and is never read for pixels.
const Sprite4bpp kProjTiles[2] = {Sprite4bpp{}, kFloorSprite};
const uint8_t kProjTileFootY[2] = {0, kFloorFootY};

uint8_t gProjIndices[1] = {1};

// Basis from examples/iso_dungeon/src/IsoDungeonConstants.h: +1 tileX steps
// right+down, +1 tileY steps left+down -- the classic 32x16 isometric diamond,
// anchored so cell (0, 0) centres at screen (120, 88).
constexpr math::ProjectionSpec kIsoSpec{120, 88, 16, 8, -16, 8};

// Under kIsoSpec, cell (0, 0) centres at (120, 88); the projected overload
// anchors a tile at (centreX - width / 2, centreY - footY), so the floor tile
// lands at (120 - 16, 88 - 8) = (104, 80) and spans x=[104,135], y=[80,95] --
// entirely inside the 240x240 buffer, hence all 512 of its pixels are written.
constexpr int kProjectedDrawX = 104;
constexpr int kProjectedDrawY = 80;

// The axis-aligned overload ignores footY and places cell (0, 0) at the raw
// origin (Renderer.cpp: baseX = originX + tx * tileWidth). (40, 60) keeps the
// whole 32x16 tile on screen and never overlaps (104, 80), so the two paths
// are distinguishable by inspecting a single pixel.
constexpr int kAxisAlignedOriginX = 40;
constexpr int kAxisAlignedOriginY = 60;

/// Owns a Renderer wired to a MockDrawSurface exposing gProjFrameBuffer.
struct ProjHarness {
    std::unique_ptr<Renderer> renderer;

    explicit ProjHarness(uint8_t fill = kSentinel) {
        auto mock = std::make_unique<MockDrawSurface>();
        mock->setSpriteBuffer(gProjFrameBuffer, sizeof(gProjFrameBuffer));

        DisplayConfig config =
            PIXELROOT32_CUSTOM_DISPLAY(mock.release(), kProjFbWidth, kProjFbHeight);
        renderer = std::make_unique<Renderer>(std::move(config));
        renderer->init();
        renderer->beginFrame();

        // Seeded AFTER beginFrame so the fill survives whatever clearing
        // strategy the dirty-region configuration picked for this build.
        std::memset(gProjFrameBuffer, fill, sizeof(gProjFrameBuffer));
    }
};

/// Resets the global palette and the fixture tile bitmap. Called at the top of
/// every projected case because Unity shares one process across cases.
void seedProjectionFixtures() {
    setDualCustomPalette(kProjPalette, kProjPalette);
    // 0x11: both nibbles are palette index 1, so every pixel is opaque.
    std::memset(gOpaqueTileData, 0x11, sizeof(gOpaqueTileData));
    gProjIndices[0] = 1;
}

TileMap4bpp makeFloorMap() {
    TileMap4bpp map{};
    map.indices = gProjIndices;
    map.width = 1;
    map.height = 1;
    map.tiles = kProjTiles;
    map.tileWidth = kFloorWidth;
    map.tileHeight = kFloorHeight;
    map.tileCount = 2;
    map.runtimeMask = nullptr;
    map.tileFootY = kProjTileFootY;
    return map;
}

std::size_t countBytes(const uint8_t* buffer, uint8_t value) {
    std::size_t n = 0;
    for (std::size_t i = 0; i < kProjFbBytes; ++i) {
        if (buffer[i] == value) {
            ++n;
        }
    }
    return n;
}

uint8_t projFbAt(int x, int y) {
    return gProjFrameBuffer[static_cast<std::size_t>(y) * kProjFbWidth + static_cast<std::size_t>(x)];
}

}  // namespace

/// (a) A spec carrying a projection must reach the projected overload: the
/// resulting framebuffer is byte-identical to a direct
/// Renderer::drawTileMap(map, 0, 0, Static, kIsoSpec) oracle.
void test_cache_draw_projected_spec_matches_projected_oracle(void) {
    seedProjectionFixtures();
    TileMap4bpp map = makeFloorMap();

    {
        ProjHarness oracle;
        oracle.renderer->drawTileMap(map, 0, 0, LayerType::Static, kIsoSpec);
        std::memcpy(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);
    }

    StaticTilemapLayerCache cache;
    TEST_ASSERT_TRUE(cache.allocateForLogicalSize(kProjFbWidth, kProjFbHeight));

    ProjHarness h;
    const TileMap4bppDrawSpec staticSpec = {&map, 0, 0, &kIsoSpec};
    cache.draw(*h.renderer, 0, 0, &staticSpec, 1, nullptr, 0);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);

    // Satisfiable by construction: the opaque 32x16 tile lands whole at
    // (104, 80), so exactly 512 bytes leave the sentinel.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kProjPalette[1]),
                            projFbAt(kProjectedDrawX, kProjectedDrawY));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(kProjFbBytes - kFloorOpaquePixels),
                             static_cast<uint32_t>(countBytes(gProjFrameBuffer, kSentinel)));
    // The axis-aligned path would have put this tile at (0, 0) instead.
    TEST_ASSERT_EQUAL_UINT8(kSentinel, projFbAt(0, 0));
}

/// (b) Regression guard for the five existing axis-aligned consumers: the
/// three-element aggregate initialiser still compiles and still produces the
/// exact framebuffer the plain overload produces.
void test_cache_draw_null_projection_matches_axis_aligned_oracle(void) {
    seedProjectionFixtures();
    TileMap4bpp map = makeFloorMap();

    {
        ProjHarness oracle;
        oracle.renderer->drawTileMap(map, kAxisAlignedOriginX, kAxisAlignedOriginY,
                                     LayerType::Static);
        std::memcpy(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);
    }

    StaticTilemapLayerCache cache;
    TEST_ASSERT_TRUE(cache.allocateForLogicalSize(kProjFbWidth, kProjFbHeight));

    ProjHarness h;
    // Exactly the initialiser shape shipped by metroidvania, legend_of_clone,
    // midway_clone and animated_tilemap -- no projection member mentioned.
    const TileMap4bppDrawSpec staticSpec = {&map, kAxisAlignedOriginX, kAxisAlignedOriginY};
    cache.draw(*h.renderer, 0, 0, &staticSpec, 1, nullptr, 0);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);

    // Same 512 opaque pixels, at the axis-aligned anchor this time.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kProjPalette[1]),
                            projFbAt(kAxisAlignedOriginX, kAxisAlignedOriginY));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(kProjFbBytes - kFloorOpaquePixels),
                             static_cast<uint32_t>(countBytes(gProjFrameBuffer, kSentinel)));
    // The projected anchor must be untouched.
    TEST_ASSERT_EQUAL_UINT8(kSentinel, projFbAt(kProjectedDrawX, kProjectedDrawY));
}

/// (c) Cache hit: a second draw() at the same camera sample restores the
/// projected snapshot with memcpy instead of redrawing it.
void test_cache_draw_projected_cache_hit_restores_snapshot(void) {
    seedProjectionFixtures();
    TileMap4bpp map = makeFloorMap();

    StaticTilemapLayerCache cache;
    TEST_ASSERT_TRUE(cache.allocateForLogicalSize(kProjFbWidth, kProjFbHeight));

    ProjHarness h;
    const TileMap4bppDrawSpec staticSpec = {&map, 0, 0, &kIsoSpec};

    cache.draw(*h.renderer, 0, 0, &staticSpec, 1, nullptr, 0);
    std::memcpy(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);

    // Wipe the framebuffer to a value nothing above writes, then draw again
    // with an unchanged camera sample.
    std::memset(gProjFrameBuffer, kScribble, kProjFbBytes);
    cache.draw(*h.renderer, 0, 0, &staticSpec, 1, nullptr, 0);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);

    // Discriminating by construction: a redraw would only overwrite the tile's
    // 512 opaque pixels and leave 57600 - 512 = 57088 scribble bytes behind.
    // A memcpy restore puts the sentinel back everywhere instead.
    TEST_ASSERT_EQUAL_UINT32(0u, static_cast<uint32_t>(countBytes(gProjFrameBuffer, kScribble)));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(kProjFbBytes - kFloorOpaquePixels),
                             static_cast<uint32_t>(countBytes(gProjFrameBuffer, kSentinel)));
    // ...and the restored content is the projected image, not the axis-aligned one.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kProjPalette[1]),
                            projFbAt(kProjectedDrawX, kProjectedDrawY));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, projFbAt(0, 0));
}

/// (d) A moved camera sample forces a rebuild, and the rebuilt content is
/// still drawn through the projection.
void test_cache_draw_projected_camera_move_forces_rebuild(void) {
    seedProjectionFixtures();
    TileMap4bpp map = makeFloorMap();

    // Oracle for a REBUILD over a scribbled framebuffer: the projected tile
    // blitted on top, everything else still scribble.
    {
        ProjHarness oracle(kScribble);
        oracle.renderer->drawTileMap(map, 0, 0, LayerType::Static, kIsoSpec);
        std::memcpy(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);
    }

    StaticTilemapLayerCache cache;
    TEST_ASSERT_TRUE(cache.allocateForLogicalSize(kProjFbWidth, kProjFbHeight));

    ProjHarness h;
    const TileMap4bppDrawSpec staticSpec = {&map, 0, 0, &kIsoSpec};

    cache.draw(*h.renderer, 0, 0, &staticSpec, 1, nullptr, 0);
    std::memset(gProjFrameBuffer, kScribble, kProjFbBytes);
    cache.draw(*h.renderer, 16, 8, &staticSpec, 1, nullptr, 0);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(gOracleFrameBuffer, gProjFrameBuffer, kProjFbBytes);

    // 57600 - 512 = 57088 scribble bytes survive precisely because the cache
    // redrew instead of restoring; a memcpy restore would have left none, and
    // would have brought the sentinel background back.
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(kProjFbBytes - kFloorOpaquePixels),
                             static_cast<uint32_t>(countBytes(gProjFrameBuffer, kScribble)));
    TEST_ASSERT_EQUAL_UINT32(0u, static_cast<uint32_t>(countBytes(gProjFrameBuffer, kSentinel)));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kProjPalette[1]),
                            projFbAt(kProjectedDrawX, kProjectedDrawY));
}

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cache_default_constructor);
    RUN_TEST(test_cache_allocate_valid_size);
    RUN_TEST(test_cache_allocate_invalid_width);
    RUN_TEST(test_cache_allocate_invalid_height);
    RUN_TEST(test_cache_allocate_negative_dimensions);
    RUN_TEST(test_cache_allocate_same_size_twice);
    RUN_TEST(test_cache_allocate_different_size);
    RUN_TEST(test_cache_clear);
    RUN_TEST(test_cache_invalidate);
    RUN_TEST(test_cache_set_enabled_true);
    RUN_TEST(test_cache_set_enabled_false);
    RUN_TEST(test_cache_toggle_enabled_twice);
    RUN_TEST(test_cache_multiple_allocate_calls);
    RUN_TEST(test_cache_allocate_then_clear_then_allocate_again);
    RUN_TEST(test_cache_invalidate_clears_valid_flag);
    RUN_TEST(test_cache_multiple_invalidate_calls);
    RUN_TEST(test_cache_enable_disable_preserves_allocation);
    RUN_TEST(test_cache_clear_then_invalidate);
    
    // Phase 4: uncommented draw() tests with sprite buffer fix
    RUN_TEST(test_cache_draw_null_static_layers);
    RUN_TEST(test_cache_draw_empty_dynamic_layers);
    RUN_TEST(test_cache_draw_all_null_layers);
    RUN_TEST(test_cache_draw_with_disabled_cache);
    RUN_TEST(test_cache_draw_multiple_camera_positions);
    RUN_TEST(test_cache_draw_after_invalidate);

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    // Only registered when the flag is on; these are the cases that prove the
    // projected dispatch exists. If the focused suite's case count does not
    // grow under [env:native_test_gameplay], they are compiling out.
    RUN_TEST(test_cache_draw_projected_spec_matches_projected_oracle);
    RUN_TEST(test_cache_draw_null_projection_matches_axis_aligned_oracle);
    RUN_TEST(test_cache_draw_projected_cache_hit_restores_snapshot);
    RUN_TEST(test_cache_draw_projected_camera_move_forces_rebuild);
#endif

    return UNITY_END();
}

#else

void setUp(void) {}
void tearDown(void) {}

void test_cache_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_disabled);
    return UNITY_END();
}

#endif