/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstddef>

#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
#include <cstdlib>
#include <memory>
#endif

#include "graphics/Renderer.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
#include "math/Projection.h"
#endif

namespace pixelroot32::graphics {

/**
 * @struct TileMap4bppDrawSpec
 * @brief One drawable 4bpp tilemap layer with an origin in logical coordinates.
 *
 * Entries with map == nullptr are skipped. Use any number of static layers
 * (snapshotted together) and dynamic layers (redrawn every frame after restore).
 *
 * @var TileMap4bppDrawSpec::projection
 * Optional isometric/oblique basis, only present when
 * @c PIXELROOT32_ENABLE_TILEMAP_PROJECTION is on. @c nullptr — the default, and
 * therefore what every existing three-element aggregate initialiser
 * (@c {&map, 0, 0}) keeps meaning — selects the axis-aligned
 * Renderer::drawTileMap overload, byte for byte as before. A non-null spec
 * selects the projected overload instead, which places, culls and marks cells
 * through that basis (see math/Projection.h).
 *
 * The member is deliberately last and default-initialised so adding it did not
 * touch a single call site, and so the struct's layout is unchanged when the
 * flag is off.
 */
struct TileMap4bppDrawSpec {
    const TileMap4bpp* map;
    int originX;
    int originY;
#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    const pixelroot32::math::ProjectionSpec* projection = nullptr;
#endif
};

/**
 * @class StaticTilemapLayerCache
 * @brief Centralized framebuffer snapshot for static 4bpp tilemap layers.
 *
 * On drivers that expose a direct logical 8bpp sprite buffer (e.g. TFT_eSPI),
 * this avoids redrawing “static” layers every frame when the sampled camera
 * position is unchanged and the cache has not been invalidated.
 *
 * Override points:
 * - Compile-time: set @c PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE to 0 in build flags.
 * - Run-time: setFramebufferCacheEnabled(false) per scene or platform init.
 * - No sprite buffer or failed allocation: automatically falls back to full draw.
 *
 * Allocate during scene @c init() via allocateForLogicalSize() or allocateForRenderer()
 * so the game loop does not hit the heap (see ARCH_MEMORY_SYSTEM.md).
 *
 * ### Projected layers
 *
 * A layer whose TileMap4bppDrawSpec::projection is non-null is cached exactly
 * like an axis-aligned one, because the cache stores *framebuffer bytes*: it
 * never re-derives cell placement, so the basis a layer was drawn through is
 * irrelevant to the snapshot and to its restore.
 *
 * What that does NOT buy is a cheap scrolling isometric background. draw()
 * rebuilds whenever the sampled camera moved (@c camMoved), so a layer drawn
 * through a projection under a camera that scrolls every frame is redrawn
 * every frame and the snapshot is pure overhead. The win is on frames where
 * the camera is stationary or clamped — a paused or menu frame, a room whose
 * map fits the screen, a camera pinned at a level edge. Isometric maps often
 * scroll on both axes at once, which makes the stationary case rarer here than
 * it is for a side-scroller; measure before enabling the cache on a projected
 * layer.
 */
class StaticTilemapLayerCache {
public:
    StaticTilemapLayerCache() = default;

    /** Releases the snapshot buffer and marks the cache invalid. */
    void clear();

    /**
     * @brief Pre-allocate the snapshot for a logical framebuffer of width * height bytes.
     * @return false if dimensions are invalid or allocation failed.
     */
    [[nodiscard]] bool allocateForLogicalSize(int width, int height);

    /** @brief Same as allocateForLogicalSize(renderer.getLogicalWidth/Height()). */
    [[nodiscard]] bool allocateForRenderer(const Renderer& renderer);

    /** Force a full rebuild on the next draw (tile data, palette, stepped static animations, etc.). */
    void invalidate();

    /**
     * @param cameraSampleX  Typically @c -renderer.getXOffset() (must match what should trigger rebuilds).
     * @param cameraSampleY  Typically @c -renderer.getYOffset().
     */
    void draw(Renderer& renderer,
              int cameraSampleX,
              int cameraSampleY,
              const TileMap4bppDrawSpec* staticLayers,
              std::size_t staticLayerCount,
              const TileMap4bppDrawSpec* dynamicLayers,
              std::size_t dynamicLayerCount);

    /**
     * Call from Scene::adviseFramebufferBeforeBeginFrame (Engine runs it before Renderer::beginFrame).
     * When dirty regions are enabled and the next draw() will memcpy the static snapshot over the full framebuffer,
     * this lets beginFrame skip selective / full clears.
     */
    void adviseFramebufferBeforeBeginFrame(Renderer& renderer,
                                           int cameraSampleX,
                                           int cameraSampleY,
                                           const TileMap4bppDrawSpec* staticLayers,
                                           std::size_t staticLayerCount,
                                           const TileMap4bppDrawSpec* dynamicLayers,
                                           std::size_t dynamicLayerCount) const;

    void setFramebufferCacheEnabled(bool enabled);
    [[nodiscard]] bool isFramebufferCacheEnabled() const;

private:
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    [[nodiscard]] bool wouldRestoreFramebufferViaCacheMemcpy(Renderer& renderer,
                                                             int cameraSampleX,
                                                             int cameraSampleY,
                                                             const TileMap4bppDrawSpec* staticLayers,
                                                             std::size_t staticLayerCount) const;

    /** std::malloc / std::free — STYLE_GUIDE forbids operator new; avoid game-loop alloc. */
    struct CacheBufferDeleter {
        void operator()(uint8_t* p) const noexcept {
            std::free(p);
        }
    };
    std::unique_ptr<uint8_t, CacheBufferDeleter> cacheBytes;
    std::size_t cacheByteCount = 0;
    int lastCameraX = 0;
    int lastCameraY = 0;
    bool cacheValid = false;
    bool userInvalidated = true;
    bool framebufferCacheEnabled = true;
#endif
};

} // namespace pixelroot32::graphics
