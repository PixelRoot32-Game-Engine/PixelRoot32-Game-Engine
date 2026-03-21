/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Tile consumption helper implementation for Phase 7.
 * Integrates with Scene, Entity system, and TileMapGeneric runtimeMask.
 */
#include "physics/TileConsumptionHelper.h"
#include "core/Entity.h"
#include "core/Actor.h"
#include <cstddef>

namespace pixelroot32::physics {
    using namespace pixelroot32::graphics;
    using namespace pixelroot32::core::logging;

TileConsumptionHelper::TileConsumptionHelper(pixelroot32::core::Scene& scene, void* tilemap, 
                                           const TileConsumptionConfig& config)
    : scene(scene), tilemap(tilemap), config(config) {
    
    // Extract tilemap dimensions for coordinate validation
    extractTilemapDimensions();
}

bool TileConsumptionHelper::consumeTile(pixelroot32::core::Actor* tileActor, uint16_t tileX, uint16_t tileY) {
    // Validate coordinates if enabled
    if (config.validateCoordinates && !validateCoordinates(tileX, tileY)) {
        log(LogLevel::Warning, "TileConsumptionHelper: Invalid tile coordinates (%d, %d)", tileX, tileY);
        return false;
    }
    
    // Check if tile is already consumed
    if (isTileConsumed(tileX, tileY)) {
        if (config.logConsumption) {
            log("TileConsumptionHelper: Tile at (%d, %d) already consumed", tileX, tileY);
        }
        return false;
    }
    
    // Step 1: Remove physics body from scene (CollisionSystem stops considering it)
    if (tileActor != nullptr) {
        scene.removeEntity(tileActor);
        
        if (config.logConsumption) {
            log("TileConsumptionHelper: Removed tile body at (%d, %d)", tileX, tileY);
        }
    }
    
    // Step 2: Update tilemap runtimeMask to hide consumed tile
    if (config.updateTilemap && tilemap != nullptr) {
        // Use template dispatch to handle different TileMapGeneric types
        // This works with Sprite, Sprite2bpp, and Sprite4bpp tilemaps
        updateTilemapRuntimeMask<Sprite>(tileX, tileY, false);
        
        if (config.logConsumption) {
            log("TileConsumptionHelper: Updated tilemap runtimeMask for (%d, %d)", tileX, tileY);
        }
    }
    
    if (config.logConsumption) {
        log("TileConsumptionHelper: Successfully consumed tile at (%d, %d)", tileX, tileY);
    }
    
    return true;
}

bool TileConsumptionHelper::consumeTileFromUserData(pixelroot32::core::Actor* tileActor, uintptr_t packedUserData) {
    if (packedUserData == 0) {
        log(LogLevel::Warning, "TileConsumptionHelper: Invalid packed userData (0)");
        return false;
    }
    
    // Unpack tile coordinates and flags from userData
    uint16_t tileX, tileY;
    TileFlags flags;
    unpackTileData(packedUserData, tileX, tileY, flags);
    
    // Only consume collectible tiles (not solid, damage, or trigger tiles)
    if (!(flags & TILE_COLLECTIBLE)) {
        if (config.logConsumption) {
            log("TileConsumptionHelper: Tile at (%d, %d) not collectible (flags: 0x%02X)", 
                                         tileX, tileY, static_cast<uint8_t>(flags));
        }
        return false;
    }
    
    return consumeTile(tileActor, tileX, tileY);
}

bool TileConsumptionHelper::isTileConsumed(uint16_t tileX, uint16_t tileY) const {
    if (config.validateCoordinates && !validateCoordinates(tileX, tileY)) {
        return true; // Out of bounds considered consumed
    }
    
    if (tilemap == nullptr) {
        return false; // No tilemap = nothing consumed
    }
    
    // Check runtimeMask state using template dispatch
    return !checkTilemapRuntimeMask<Sprite>(tileX, tileY);
}

bool TileConsumptionHelper::restoreTile(uint16_t tileX, uint16_t tileY) {
    if (config.validateCoordinates && !validateCoordinates(tileX, tileY)) {
        return false;
    }
    
    if (tilemap == nullptr) {
        return false;
    }
    
    // Check if tile is actually consumed
    if (!isTileConsumed(tileX, tileY)) {
        return false;
    }
    
    // Restore tile by setting it active in runtimeMask
    updateTilemapRuntimeMask<Sprite>(tileX, tileY, true);
    
    if (config.logConsumption) {
        log("TileConsumptionHelper: Restored tile at (%d, %d)", tileX, tileY);
    }
    
    return true;
}

void TileConsumptionHelper::extractTilemapDimensions() {
    if (tilemap == nullptr) {
        return;
    }
    // Use offsetof to avoid depending on raw struct layout (indices is a pointer;
    // width/height are not at byte offsets 1 and 2 on 32-bit platforms).
    auto* tilemapPtr = static_cast<const uint8_t*>(tilemap);
    constexpr size_t offWidth = offsetof(TileMapGeneric<Sprite>, width);
    constexpr size_t offHeight = offsetof(TileMapGeneric<Sprite>, height);
    tilemapWidth = tilemapPtr[offWidth];
    tilemapHeight = tilemapPtr[offHeight];
    if (config.logConsumption) {
        log("TileConsumptionHelper: Extracted tilemap dimensions %dx%d",
            tilemapWidth, tilemapHeight);
    }
}

template<typename T>
void TileConsumptionHelper::updateTilemapRuntimeMask(uint16_t tileX, uint16_t tileY, bool active) {
    auto* tilemapGeneric = static_cast<TileMapGeneric<T>*>(tilemap);
    if (tilemapGeneric != nullptr) {
        tilemapGeneric->setTileActive(tileX, tileY, active);
    }
}

template<typename T>
bool TileConsumptionHelper::checkTilemapRuntimeMask(uint16_t tileX, uint16_t tileY) const {
    auto* tilemapGeneric = static_cast<const TileMapGeneric<T>*>(tilemap);
    if (tilemapGeneric != nullptr) {
        return tilemapGeneric->isTileActive(tileX, tileY);
    }
    return true; // No tilemap = considered active
}

bool TileConsumptionHelper::validateCoordinates(uint16_t tileX, uint16_t tileY) const {
    if (tilemapWidth == 0 || tilemapHeight == 0) {
        return true; // No dimensions available, assume valid
    }
    
    return tileX < tilemapWidth && tileY < tilemapHeight;
}

// Explicit template instantiations for common tilemap types
template void TileConsumptionHelper::updateTilemapRuntimeMask<Sprite>(uint16_t, uint16_t, bool);
template void TileConsumptionHelper::updateTilemapRuntimeMask<Sprite2bpp>(uint16_t, uint16_t, bool);
template void TileConsumptionHelper::updateTilemapRuntimeMask<Sprite4bpp>(uint16_t, uint16_t, bool);

template bool TileConsumptionHelper::checkTilemapRuntimeMask<Sprite>(uint16_t, uint16_t) const;
template bool TileConsumptionHelper::checkTilemapRuntimeMask<Sprite2bpp>(uint16_t, uint16_t) const;
template bool TileConsumptionHelper::checkTilemapRuntimeMask<Sprite4bpp>(uint16_t, uint16_t) const;

} // namespace pixelroot32::physics
