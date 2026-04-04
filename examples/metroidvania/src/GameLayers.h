#pragma once
#include "physics/CollisionTypes.h"

namespace metroidvania {

namespace pr = pixelroot32;

/**
 * @file GameLayers.h
 * @brief Collision layer bitmasks for Metroidvania.
 *
 * Platform collision is manual (tilemap); engine only detects Actor-vs-Actor.
 */
namespace Layers {
    const pr::physics::CollisionLayer PLAYER = 1 << 0;
    const pr::physics::CollisionLayer PLATFORM = 1 << 1;
    const pr::physics::CollisionLayer GROUND = 1 << 2;
    const pr::physics::CollisionLayer ENEMY  = 1 << 3;  // for future use (enemies, projectiles, pickups)
}

} // namespace metroidvania
