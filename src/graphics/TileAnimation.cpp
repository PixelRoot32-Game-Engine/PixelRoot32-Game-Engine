/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under MIT License
 */

#include "graphics/TileAnimation.h"
#include "platforms/PlatformMemory.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

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
    globalFrameCounter(0)
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
    
    // Reset lookup table to identity mapping
    for (uint16_t i = 0; i < tileCount && i < MAX_TILESET_SIZE; i++) {
        lookupTable[i] = i;
    }
}

void TileAnimationManager::step() {
    globalFrameCounter++;
    
    for (uint8_t animIdx = 0; animIdx < animCount; animIdx++) {
        TileAnimation anim;
        PIXELROOT32_MEMCPY_P(&anim, &animations[animIdx], sizeof(TileAnimation));
        
        // Calculate current frame (handle frameDuration = 0)
        uint8_t currentFrame = (anim.frameDuration > 0) 
            ? (globalFrameCounter / anim.frameDuration) % anim.frameCount 
            : 0;
        uint8_t currentTileIndex = anim.baseTileIndex + currentFrame;
        
        // Update all tiles in this animation
        for (uint8_t frame = 0; frame < anim.frameCount; frame++) {
            uint8_t tileIdx = anim.baseTileIndex + frame;
            if (tileIdx < tileCount && tileIdx < MAX_TILESET_SIZE) {
                lookupTable[tileIdx] = currentTileIndex;
            }
        }
    }
}

uint8_t IRAM_ATTR TileAnimationManager::resolveFrame(uint8_t tileIndex) {
    if (tileIndex >= tileCount) return tileIndex;
    return lookupTable[tileIndex];
}
    
} // namespace pixelroot32::graphics
