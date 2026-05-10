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

namespace pixelroot32::graphics {

/**
 * @brief One drawable 4bpp tilemap layer with an origin in logical coordinates.
 *
 * Entries with map == nullptr are skipped. Use any number of static layers
 * (snapshotted together) and dynamic layers (redrawn every frame after restore).
 */
struct TileMap4bppDrawSpec {
    const TileMap4bpp* map;
    int originX;
    int originY;
};

/**
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
