/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Tile collision builder helper for creating physics bodies from behavior layers.
 * Integrates with Scene, Entity, and CollisionSystem without changing architecture.
 */
#pragma once
#include "physics/TileAttributes.h"
#include "physics/StaticActor.h"
#include "physics/SensorActor.h"
#include "core/Scene.h"
#include "math/Vector2.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::physics {

/**
 * @struct TileCollisionBuilderConfig
 * @brief Configuration for tile collision building.
 */
struct TileCollisionBuilderConfig {
    uint8_t tileWidth;      ///< Width of each tile in world units
    uint8_t tileHeight;     ///< Height of each tile in world units
    uint16_t maxEntities;   ///< Maximum entities to create (safety limit)
    
    TileCollisionBuilderConfig(uint8_t w = 16, uint8_t h = 16) 
        : tileWidth(w), tileHeight(h), maxEntities(pixelroot32::platforms::config::MaxEntities / 2) {}
};

/**
 * @class TileCollisionBuilder
 * @brief Helper class for creating physics bodies from exported behavior layers.
 * 
 * This builder follows the exact plan specification:
 * - Iterates behavior layers looking for non-TILE_NONE flags
 * - Creates StaticActor or SensorActor based on TileFlags
 * - Configures sensor/one-way properties from flags
 * - Packs tile coordinates and flags into userData for gameplay callbacks
 * - Registers bodies with the scene's entity system and collision system
 * 
 * Usage:
 * ```cpp
 * TileBehaviorLayer layer = { behaviorData, 32, 32 };
 * TileCollisionBuilder builder(scene, config);
 * int entitiesCreated = builder.buildFromBehaviorLayer(layer, 0);
 * ```
 */
class TileCollisionBuilder {
private:
    pixelroot32::core::Scene& scene;
    TileCollisionBuilderConfig config;
    int entitiesCreated = 0;

public:
    /**
     * @brief Constructs a new TileCollisionBuilder.
     * @param scene Reference to the scene where entities will be added
     * @param config Configuration for tile size and entity limits
     */
    TileCollisionBuilder(pixelroot32::core::Scene& scene, const TileCollisionBuilderConfig& config = TileCollisionBuilderConfig());

    /**
     * @brief Creates physics bodies from a behavior layer.
     * 
     * Iterates through all tiles in the layer and creates StaticActor or SensorActor
     * for tiles with non-zero flags. Bodies are configured based on TileFlags:
     * - SENSOR/DAMAGE/COLLECTIBLE/TRIGGER → SensorActor (setSensor=true)
     * - ONEWAY → setOneWay=true
     * - SOLID → StaticActor (default)
     * 
     * Each body receives packed userData containing (x, y, flags) for gameplay callbacks.
     * 
     * @param layer Behavior layer containing tile flags
     * @param layerIndex Index of the layer (for debugging/logging)
     * @return Number of entities created, or -1 if entity limit was exceeded
     */
    int buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t layerIndex = 0);

    /**
     * @brief Gets the number of entities created by this builder.
     * @return Entity count
     */
    int getEntitiesCreated() const { return entitiesCreated; }

    /**
     * @brief Resets the entity counter.
     */
    void reset() { entitiesCreated = 0; }

private:
    /**
     * @brief Creates a single tile physics body.
     * @param x Tile X coordinate
     * @param y Tile Y coordinate  
     * @param flags TileFlags for this tile
     * @return Pointer to created actor, or nullptr if entity limit reached
     */
    pixelroot32::core::PhysicsActor* createTileBody(uint16_t x, uint16_t y, TileFlags flags);

    /**
     * @brief Configures a tile body based on its flags.
     * @param body The physics actor to configure
     * @param flags TileFlags for configuration
     */
    void configureTileBody(pixelroot32::core::PhysicsActor* body, TileFlags flags);

    /**
     * @brief Converts tile coordinates to world position.
     * @param x Tile X coordinate
     * @param y Tile Y coordinate
     * @return World position vector
     */
    pixelroot32::math::Vector2 tileToWorldPosition(uint16_t x, uint16_t y) const;
};

/**
 * @brief Convenience function for building collision bodies from behavior layers.
 * 
 * This is a helper function that creates a TileCollisionBuilder and builds
 * collision bodies in a single call, following the plan's workflow.
 * 
 * @param scene Scene to add entities to
 * @param layer Behavior layer to process
 * @param tileWidth Width of tiles in world units
 * @param tileHeight Height of tiles in world units
 * @param layerIndex Layer index for debugging
 * @return Number of entities created, or -1 if entity limit exceeded
 */
inline int buildTileCollisions(
    pixelroot32::core::Scene& scene,
    const TileBehaviorLayer& layer,
    uint8_t tileWidth = 16,
    uint8_t tileHeight = 16,
    uint8_t layerIndex = 0
) {
    TileCollisionBuilderConfig config(tileWidth, tileHeight);
    TileCollisionBuilder builder(scene, config);
    return builder.buildFromBehaviorLayer(layer, layerIndex);
}

} // namespace pixelroot32::physics
