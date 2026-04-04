/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/StaticTilemapLayerCache.h"

#include <cstdlib>
#include <cstring>

namespace pixelroot32::graphics {

namespace {

void drawSpecs(Renderer& renderer, const TileMap4bppDrawSpec* layers, std::size_t count) {
    if (!layers || count == 0) {
        return;
    }
    for (std::size_t i = 0; i < count; ++i) {
        const TileMap4bpp* m = layers[i].map;
        if (!m) {
            continue;
        }
        renderer.drawTileMap(*m, layers[i].originX, layers[i].originY);
    }
}

void drawAllLayers(Renderer& renderer,
                   const TileMap4bppDrawSpec* staticLayers,
                   std::size_t staticLayerCount,
                   const TileMap4bppDrawSpec* dynamicLayers,
                   std::size_t dynamicLayerCount) {
    drawSpecs(renderer, staticLayers, staticLayerCount);
    drawSpecs(renderer, dynamicLayers, dynamicLayerCount);
}

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

    bool hasStatic = false;
    if (staticLayers) {
        for (std::size_t i = 0; i < staticLayerCount; ++i) {
            if (staticLayers[i].map) {
                hasStatic = true;
                break;
            }
        }
    }

    if (!hasStatic) {
        drawSpecs(renderer, dynamicLayers, dynamicLayerCount);
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
        drawSpecs(renderer, staticLayers, staticLayerCount);
        std::memcpy(cacheBytes.get(), fb, bufBytes);
        drawSpecs(renderer, dynamicLayers, dynamicLayerCount);
        lastCameraX = cameraSampleX;
        lastCameraY = cameraSampleY;
        cacheValid = true;
        userInvalidated = false;
    } else {
        std::memcpy(fb, cacheBytes.get(), bufBytes);
        drawSpecs(renderer, dynamicLayers, dynamicLayerCount);
    }
#endif
}

} // namespace pixelroot32::graphics
