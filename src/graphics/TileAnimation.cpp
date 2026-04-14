/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under MIT License
 */

#include "graphics/TileAnimation.h"
#include "platforms/PlatformMemory.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

#if defined(PLATFORM_NATIVE)
#include "platforms/mock/MockArduino.h"
#else
#include <Arduino.h>
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

using pixelroot32::core::logging::log;

namespace pixelroot32::graphics {

   
TileAnimationManager::TileAnimationManager(
    const TileAnimation* animations,
    uint8_t animCount,
    uint16_t tileCount
) : animations(animations),
    animCount(animCount),
    tileCount(tileCount),
    globalFrameCounter(0),
    tickAccumUs(0),
    lastStepMicros(0)
{
    // Compile-time validation
    static_assert(MAX_TILESET_SIZE >= 64, 
        "MAX_TILESET_SIZE must be at least 64 tiles");
    
    // Runtime validation (debug mode)
    if constexpr (pixelroot32::platforms::config::EnableLogging) {
        if (tileCount > MAX_TILESET_SIZE) {
            log("ERROR: tileCount (%d) exceeds MAX_TILESET_SIZE (%d)", 
                tileCount, MAX_TILESET_SIZE);
        }
    }
    
    // Initialize lookup table to identity (non-animated)
    for (uint16_t i = 0; i < tileCount && i < MAX_TILESET_SIZE; i++) {
        lookupTable[i] = i;
    }
    
    if (animCount > 0) {
        rebuildLookupTable();
    }

    lastStepMicros = micros();

    if constexpr (pixelroot32::platforms::config::EnableLogging) {
        // Validate animations
        for (uint8_t i = 0; i < animCount; i++) {
            TileAnimation anim;
            PIXELROOT32_MEMCPY_P(&anim, &animations[i], sizeof(TileAnimation));
            
            if (anim.frameCount == 0) {
                log("ERROR: Animation %d has frameCount=0", i);
            }
            if (anim.frameDuration == 0) {
                log("ERROR: Animation %d has frameDuration=0", i);
            }
            
            uint16_t lastTile = anim.baseTileIndex + anim.frameCount - 1;
            if (lastTile >= tileCount || lastTile >= MAX_TILESET_SIZE) {
                log("ERROR: Animation %d exceeds tileset bounds (last tile: %d, max: %d)", 
                    i, lastTile, tileCount);
            }
        }
    }
}

void TileAnimationManager::reset() {
    globalFrameCounter = 0;
    tickAccumUs = 0;
    lastStepMicros = micros();

    // Reset lookup table to identity mapping
    for (uint16_t i = 0; i < tileCount && i < MAX_TILESET_SIZE; i++) {
        lookupTable[i] = i;
    }

    if (animCount > 0) {
        rebuildLookupTable();
    }
}

void TileAnimationManager::rebuildLookupTable() {
    for (uint8_t animIdx = 0; animIdx < animCount; animIdx++) {
        TileAnimation anim;
        PIXELROOT32_MEMCPY_P(&anim, &animations[animIdx], sizeof(TileAnimation));

        uint8_t currentFrame = (anim.frameDuration > 0)
            ? static_cast<uint8_t>((globalFrameCounter / anim.frameDuration) % anim.frameCount)
            : 0;
        uint8_t currentTileIndex = anim.baseTileIndex + currentFrame;

        for (uint8_t frame = 0; frame < anim.frameCount; frame++) {
            uint16_t tileIdx = static_cast<uint16_t>(anim.baseTileIndex) + frame;
            if (tileIdx < tileCount && tileIdx < MAX_TILESET_SIZE) {
                lookupTable[tileIdx] = currentTileIndex;
            }
        }
    }
}

void TileAnimationManager::step(unsigned long deltaTimeMs) {
    if (animCount == 0) {
        return;
    }

    constexpr uint32_t kMicrosPerAnimTick = 1000000u / 60u;
    constexpr uint32_t kMaxWallUs = 50000u;
    constexpr uint32_t kMaxTicksPerCall = 10u;

    const uint32_t now = micros();

    uint32_t wallUs = (lastStepMicros != 0u) ? (now - lastStepMicros) : 0u;
    lastStepMicros = now;

    if (wallUs > kMaxWallUs) {
        wallUs = kMaxWallUs;
    }

    if (wallUs == 0u) {
        const uint32_t ms = static_cast<uint32_t>(deltaTimeMs > 50ul ? 50ul : deltaTimeMs);
        wallUs = ms * 1000u;
    }
    if (wallUs == 0u) {
        return;
    }
    if (wallUs > kMaxWallUs) {
        wallUs = kMaxWallUs;
    }

    tickAccumUs += wallUs;

    uint32_t ticksThisUpdate = 0;
    while (tickAccumUs >= kMicrosPerAnimTick && ticksThisUpdate < kMaxTicksPerCall) {
        tickAccumUs -= kMicrosPerAnimTick;
        globalFrameCounter++;
        ticksThisUpdate++;
    }

    if (ticksThisUpdate == 0) {
        return;
    }

    rebuildLookupTable();
}

uint8_t IRAM_ATTR TileAnimationManager::resolveFrame(uint8_t tileIndex) {
    if (tileIndex >= tileCount) return tileIndex;
    return lookupTable[tileIndex];
}

uint32_t TileAnimationManager::getVisualSignature() const {
    // FNV-1a mix: one sample per animation (all slots for that anim share the same resolved tile).
    uint32_t h = 2166136261u;
    for (uint8_t i = 0; i < animCount; ++i) {
        TileAnimation anim;
        PIXELROOT32_MEMCPY_P(&anim, &animations[i], sizeof(TileAnimation));
        const uint16_t base = anim.baseTileIndex;
        if (base < tileCount) {
            h ^= static_cast<uint32_t>(lookupTable[base]);
            h *= 16777619u;
        }
    }
    return h;
}
    
} // namespace pixelroot32::graphics
