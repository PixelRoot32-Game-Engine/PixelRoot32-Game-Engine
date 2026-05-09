/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/StaticTilemapLayerCache.h"

#include <cstdlib>
#include <cstring>

#include "platforms/EngineConfig.h"

namespace pixelroot32::graphics {

namespace {

void drawSpecs(Renderer& renderer, const TileMap4bppDrawSpec* layers, std::size_t count, LayerType layerType) {
    if (!layers || count == 0) {
        return;
    }
    for (std::size_t i = 0; i < count; ++i) {
        const TileMap4bpp* m = layers[i].map;
        if (!m) {
            continue;
        }
        renderer.drawTileMap(*m, layers[i].originX, layers[i].originY, layerType);
    }
}

void drawAllLayers(Renderer& renderer,
                   const TileMap4bppDrawSpec* staticLayers,
                   std::size_t staticLayerCount,
                   const TileMap4bppDrawSpec* dynamicLayers,
                   std::size_t dynamicLayerCount) {
    drawSpecs(renderer, staticLayers, staticLayerCount, LayerType::Static);
    drawSpecs(renderer, dynamicLayers, dynamicLayerCount, LayerType::Dynamic);
}

#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
bool hasAnyStaticMap(const TileMap4bppDrawSpec* staticLayers, std::size_t staticLayerCount) {
    if (!staticLayers || staticLayerCount == 0) {
        return false;
    }
    for (std::size_t i = 0; i < staticLayerCount; ++i) {
        if (staticLayers[i].map) {
            return true;
        }
    }
    return false;
}
#endif

} // namespace

void StaticTilemapLayerCache::clear() {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    cacheBytes.reset();
    cacheByteCount = 0;
    cacheValid = false;
    userInvalidated = true;
#endif
}

bool StaticTilemapLayerCache::allocateForLogicalSize(int width, int height) {
#if !PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    (void)width;
    (void)height;
    return true;
#else
    if (width <= 0 || height <= 0) {
        clear();
        return false;
    }
    const std::size_t n = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (cacheBytes && cacheByteCount == n) {
        invalidate();
        return true;
    }
    cacheBytes.reset();
    cacheByteCount = 0;
    // malloc (not operator new per STYLE_GUIDE); nullptr on OOM without abort.
    uint8_t* raw = static_cast<uint8_t*>(std::malloc(n));
    if (!raw) {
        return false;
    }
    cacheBytes.reset(raw);
    cacheByteCount = n;
    cacheValid = false;
    userInvalidated = true;
    return true;
#endif
}

bool StaticTilemapLayerCache::allocateForRenderer(const Renderer& renderer) {
    return allocateForLogicalSize(renderer.getLogicalWidth(), renderer.getLogicalHeight());
}

void StaticTilemapLayerCache::invalidate() {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    cacheValid = false;
    userInvalidated = true;
#endif
}

void StaticTilemapLayerCache::setFramebufferCacheEnabled(bool enabled) {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    if (framebufferCacheEnabled == enabled) {
        return;
    }
    framebufferCacheEnabled = enabled;
    invalidate();
#else
    (void)enabled;
#endif
}

bool StaticTilemapLayerCache::isFramebufferCacheEnabled() const {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    return framebufferCacheEnabled;
#else
    return false;
#endif
}

#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
bool StaticTilemapLayerCache::wouldRestoreFramebufferViaCacheMemcpy(Renderer& renderer,
                                                                    int cameraSampleX,
                                                                    int cameraSampleY,
                                                                    const TileMap4bppDrawSpec* staticLayers,
                                                                    std::size_t staticLayerCount) const {
    if (!framebufferCacheEnabled) {
        return false;
    }
    if (!hasAnyStaticMap(staticLayers, staticLayerCount)) {
        return false;
    }
    uint8_t* fb = renderer.getDrawSurface().getSpriteBuffer();
    const int w = renderer.getLogicalWidth();
    const int h = renderer.getLogicalHeight();
    const std::size_t bufBytes = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
    if (!fb || !cacheBytes || cacheByteCount != bufBytes) {
        return false;
    }
    const bool camMoved = (cameraSampleX != lastCameraX || cameraSampleY != lastCameraY);
    const bool needRebuild = !cacheValid || camMoved || userInvalidated;
    return !needRebuild;
}
#endif

void StaticTilemapLayerCache::adviseFramebufferBeforeBeginFrame(
    Renderer& renderer,
    int cameraSampleX,
    int cameraSampleY,
    const TileMap4bppDrawSpec* staticLayers,
    std::size_t staticLayerCount,
    const TileMap4bppDrawSpec* dynamicLayers,
    std::size_t dynamicLayerCount) const {
#if !PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    (void)renderer;
    (void)cameraSampleX;
    (void)cameraSampleY;
    (void)staticLayers;
    (void)staticLayerCount;
    (void)dynamicLayers;
    (void)dynamicLayerCount;
#else
    if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
        (void)renderer;
        (void)cameraSampleX;
        (void)cameraSampleY;
        (void)staticLayers;
        (void)staticLayerCount;
        (void)dynamicLayers;
        (void)dynamicLayerCount;
    } else {
        (void)dynamicLayers;
        (void)dynamicLayerCount;
        const bool suppress = wouldRestoreFramebufferViaCacheMemcpy(
            renderer, cameraSampleX, cameraSampleY, staticLayers, staticLayerCount);
        renderer.accumulateFramebufferClearSuppressionAdvice(suppress);
    }
#endif
}

void StaticTilemapLayerCache::draw(Renderer& renderer,
                                   int cameraSampleX,
                                   int cameraSampleY,
                                   const TileMap4bppDrawSpec* staticLayers,
                                   std::size_t staticLayerCount,
                                   const TileMap4bppDrawSpec* dynamicLayers,
                                   std::size_t dynamicLayerCount) {
#if !PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    drawAllLayers(renderer, staticLayers, staticLayerCount, dynamicLayers, dynamicLayerCount);
    return;
#else
    if (!framebufferCacheEnabled) {
        drawAllLayers(renderer, staticLayers, staticLayerCount, dynamicLayers, dynamicLayerCount);
        return;
    }

    if (!hasAnyStaticMap(staticLayers, staticLayerCount)) {
        drawSpecs(renderer, dynamicLayers, dynamicLayerCount, LayerType::Dynamic);
        return;
    }

    uint8_t* fb = renderer.getDrawSurface().getSpriteBuffer();
    const int w = renderer.getLogicalWidth();
    const int h = renderer.getLogicalHeight();
    const std::size_t bufBytes = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);

    if (!fb || !cacheBytes || cacheByteCount != bufBytes) {
        drawAllLayers(renderer, staticLayers, staticLayerCount, dynamicLayers, dynamicLayerCount);
        return;
    }

    const bool camMoved = (cameraSampleX != lastCameraX || cameraSampleY != lastCameraY);
    const bool needRebuild = !cacheValid || camMoved || userInvalidated;

    if (needRebuild) {
        drawSpecs(renderer, staticLayers, staticLayerCount, LayerType::Static);
        std::memcpy(cacheBytes.get(), fb, bufBytes);
        drawSpecs(renderer, dynamicLayers, dynamicLayerCount, LayerType::Dynamic);
        lastCameraX = cameraSampleX;
        lastCameraY = cameraSampleY;
        cacheValid = true;
        userInvalidated = false;
    } else {
        std::memcpy(fb, cacheBytes.get(), bufBytes);
        drawSpecs(renderer, dynamicLayers, dynamicLayerCount, LayerType::Dynamic);
    }
#endif
}

} // namespace pixelroot32::graphics
