/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Tile consumption helper for Phase 7: removing entities and updating tilemap.
 * Integrates with Scene, Entity system, and TileMapGeneric runtimeMask.
 */
#pragma once
#include "physics/TileAttributes.h"
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "core/Log.h"

namespace pixelroot32::physics {

/**
 * @struct TileConsumptionConfig
 * @brief Configuration for tile consumption operations.
 */
struct TileConsumptionConfig {
    bool updateTilemap = true;     ///< Update tilemap runtimeMask to hide consumed tiles
    bool logConsumption = true;    ///< Log consumption events for debugging
    bool validateCoordinates = true; ///< Validate tile coordinates before consumption
    
    TileConsumptionConfig() = default;
};

/**
 * @class TileConsumptionHelper
 * @brief Helper class for consuming tiles (removing bodies and updating visuals).
 * 
 * This class implements Phase 7 of the tile attribute system:
 * 1. Remove tile body from Scene (CollisionSystem no longer considers it)
 * 2. Update TileMapGeneric::runtimeMask to hide consumed tiles
 * 3. Reuse existing runtimeMask instead of creating separate consumedMask
 * 
 * Usage:
 * ```cpp
 * TileConsumptionHelper helper(scene, tilemap, config);
 * bool consumed = helper.consumeTile(tileActor, tileX, tileY);
 * ```
 */
class TileConsumptionHelper {
private:
    pixelroot32::core::Scene& scene;
    void* tilemap;  // TileMapGeneric* (void* to avoid template instantiation)
    TileConsumptionConfig config;
    uint16_t tilemapWidth = 0;
    uint16_t tilemapHeight = 0;

public:
    /**
     * @brief Constructs a new TileConsumptionHelper.
     * @param scene Reference to the scene containing the tile entities
     * @param tilemap Pointer to the tilemap for visual updates (TileMapGeneric*)
     * @param config Configuration for consumption behavior
     */
    TileConsumptionHelper(pixelroot32::core::Scene& scene, void* tilemap, 
                         const TileConsumptionConfig& config = TileConsumptionConfig());

    /**
     * @brief Consumes a tile by removing its physics body and updating visuals.
     * 
     * This implements the exact Phase 7 workflow:
     * 1. removeEntity(tileActor) from scene (CollisionSystem stops considering it)
     * 2. setTileActive(tileX, tileY, false) in tilemap runtimeMask
     * 3. Log consumption if enabled
     * 
     * @param tileActor Pointer to the tile physics actor to consume
     * @param tileX Tile X coordinate (from unpacked userData)
     * @param tileY Tile Y coordinate (from unpacked userData)
     * @return true if tile was successfully consumed, false if already consumed or invalid
     */
    bool consumeTile(pixelroot32::core::Actor* tileActor, uint16_t tileX, uint16_t tileY);

    /**
     * @brief Consumes a tile using packed userData from collision callback.
     * 
     * Convenience method that unpacks userData and calls consumeTile().
     * This is the typical usage pattern from Phase 6 collision callbacks.
     * 
     * @param tileActor Pointer to the tile physics actor
     * @param packedUserData Packed userData from tileActor->getUserData()
     * @return true if tile was successfully consumed, false otherwise
     */
    bool consumeTileFromUserData(pixelroot32::core::Actor* tileActor, uintptr_t packedUserData);

    /**
     * @brief Check if a tile has been consumed (hidden in tilemap).
     * 
     * @param tileX Tile X coordinate
     * @param tileY Tile Y coordinate
     * @return true if tile is consumed (inactive), false if still visible
     */
    bool isTileConsumed(uint16_t tileX, uint16_t tileY) const;

    /**
     * @brief Restore a consumed tile (for debugging or special game mechanics).
     * 
     * @param tileX Tile X coordinate
     * @param tileY Tile Y coordinate
     * @return true if tile was restored, false if tile was not consumed
     */
    bool restoreTile(uint16_t tileX, uint16_t tileY);

private:
    /**
     * @brief Extract tilemap dimensions from the tilemap pointer.
     * 
     * This works with any TileMapGeneric specialization (Sprite, Sprite2bpp, Sprite4bpp).
     */
    void extractTilemapDimensions();

    /**
     * @brief Template method to update tilemap runtimeMask.
     * 
     * Uses template specialization to work with different TileMapGeneric types.
     */
    template<typename T>
    void updateTilemapRuntimeMask(uint16_t tileX, uint16_t tileY, bool active);

    /**
     * @brief Template method to check tilemap runtimeMask state.
     */
    template<typename T>
    bool checkTilemapRuntimeMask(uint16_t tileX, uint16_t tileY) const;

    /**
     * @brief Validate tile coordinates against tilemap dimensions.
     */
    bool validateCoordinates(uint16_t tileX, uint16_t tileY) const;
};

/**
 * @brief Convenience function for consuming tiles from collision callbacks.
 * 
 * This is the typical usage pattern from Phase 6 onCollision callbacks:
 * ```cpp
 * void onCollision(Actor* other) override {
 *     if (other->getUserData()) {
 *         uintptr_t packed = reinterpret_cast<uintptr_t>(other->getUserData());
 *         uint16_t x, y;
 *         TileFlags flags;
 *         unpackTileData(packed, x, y, flags);
 *         
 *         if (flags & TILE_COLLECTIBLE) {
 *             consumeTileFromCollision(other, packed, scene, tilemap);
 *         }
 *     }
 * }
 * ```
 * 
 * @param tileActor Pointer to the tile physics actor
 * @param packedUserData Packed userData from tileActor
 * @param scene Reference to the scene
 * @param tilemap Pointer to the tilemap (TileMapGeneric*)
 * @param config Optional consumption configuration
 * @return true if tile was consumed, false otherwise
 */
inline bool consumeTileFromCollision(pixelroot32::core::Actor* tileActor, uintptr_t packedUserData,
                                     pixelroot32::core::Scene& scene, void* tilemap,
                                     const TileConsumptionConfig& config = TileConsumptionConfig()) {
    TileConsumptionHelper helper(scene, tilemap, config);
    return helper.consumeTileFromUserData(tileActor, packedUserData);
}

/**
 * @brief Batch consumption helper for multiple tiles.
 * 
 * Useful for clearing areas, resetting levels, or special effects.
 * 
 * @param scene Reference to the scene
 * @param tilemap Pointer to the tilemap
 * @param tiles Array of tile coordinates to consume
 * @param count Number of tiles in the array
 * @param config Optional consumption configuration
 * @return Number of tiles successfully consumed
 */
inline int consumeTilesBatch(pixelroot32::core::Scene& scene, void* tilemap,
                            const uint16_t tiles[][2], int count,
                            const TileConsumptionConfig& config = TileConsumptionConfig()) {
    TileConsumptionHelper helper(scene, tilemap, config);
    int consumed = 0;
    for (int i = 0; i < count; ++i) {
        uint16_t tileX = tiles[i][0];
        uint16_t tileY = tiles[i][1];
        // Only updates tilemap runtimeMask (no physics body removal; no actor pointers).
        if (helper.consumeTile(nullptr, tileX, tileY)) {
            consumed++;
        }
    }
    return consumed;
}

} // namespace pixelroot32::physics
