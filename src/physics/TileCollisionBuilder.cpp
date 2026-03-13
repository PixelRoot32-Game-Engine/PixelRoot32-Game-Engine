/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Tile collision builder: creates physics bodies from behavior layers.
 * Bodies are registered with the scene and carry packTileData(x, y, flags) in userData.
 */
#include "physics/TileCollisionBuilder.h"
#include "core/Scene.h"
#include "physics/TileAttributes.h"
#include "physics/StaticActor.h"
#include "physics/SensorActor.h"
#include "math/Vector2.h"
#include "math/Scalar.h"
#include "platforms/EngineConfig.h"
#include <cstdint>

namespace pixelroot32::physics {

// Default collision layer/mask for item bodies so they collide with the player.
// Matches common game convention: layer 16 = ITEM, mask 1 = PLAYER (Pixel Leap GameLayers).
constexpr uint16_t kDefaultItemCollisionLayer = 16;
constexpr uint16_t kDefaultItemCollisionMask  = 1;

TileCollisionBuilder::TileCollisionBuilder(
    pixelroot32::core::Scene& sceneRef,
    const TileCollisionBuilderConfig& cfg)
    : scene(sceneRef), config(cfg), entitiesCreated(0) {}

int TileCollisionBuilder::buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t /* layerIndex */) {
    entitiesCreated = 0;
    if (layer.data == nullptr || layer.width == 0 || layer.height == 0) {
        return 0;
    }

    const uint16_t maxEntities = config.maxEntities;
    if (maxEntities == 0) {
        return -1;
    }

    for (int y = 0; y < static_cast<int>(layer.height); ++y) {
        for (int x = 0; x < static_cast<int>(layer.width); ++x) {
            uint8_t flags = getTileFlags(layer, x, y);
            if (flags == 0) continue;

            pixelroot32::core::PhysicsActor* body = createTileBody(
                static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<TileFlags>(flags));
            if (body == nullptr) {
                return -1;
            }
            configureTileBody(body, static_cast<TileFlags>(flags));
            body->setUserData(reinterpret_cast<void*>(packTileData(
                static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<TileFlags>(flags))));
            body->setCollisionLayer(kDefaultItemCollisionLayer);
            body->setCollisionMask(kDefaultItemCollisionMask);

            scene.addEntity(body);
            entitiesCreated++;
            if (entitiesCreated >= maxEntities) {
                return -1;
            }
        }
    }
    return entitiesCreated;
}

pixelroot32::core::PhysicsActor* TileCollisionBuilder::createTileBody(uint16_t x, uint16_t y, TileFlags flags) {
    pixelroot32::math::Vector2 pos = tileToWorldPosition(x, y);
    const int w = static_cast<int>(config.tileWidth);
    const int h = static_cast<int>(config.tileHeight);

    if (isSensorTile(flags)) {
        return new SensorActor(pos.x, pos.y, w, h);
    }
    return new StaticActor(pos.x, pos.y, w, h);
}

void TileCollisionBuilder::configureTileBody(pixelroot32::core::PhysicsActor* body, TileFlags flags) {
    if (body == nullptr) return;
    if (isSensorTile(flags)) {
        body->setSensor(true);
    }
    if (isOneWayTile(flags)) {
        body->setOneWay(true);
    }
}

pixelroot32::math::Vector2 TileCollisionBuilder::tileToWorldPosition(uint16_t x, uint16_t y) const {
    pixelroot32::math::Scalar wx = pixelroot32::math::toScalar(static_cast<float>(x * config.tileWidth));
    pixelroot32::math::Scalar wy = pixelroot32::math::toScalar(static_cast<float>(y * config.tileHeight));
    return pixelroot32::math::Vector2{ wx, wy };
}

} // namespace pixelroot32::physics
