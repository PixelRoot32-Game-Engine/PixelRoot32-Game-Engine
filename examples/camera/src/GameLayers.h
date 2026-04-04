#pragma once
#include "physics/CollisionTypes.h"

/**
 * @file GameLayers.h
 * @brief Bitmask definitions for collision filtering
 * 
 * Usage:
 * - Layer: What am I? (e.g., setCollisionLayer(PLAYER))
 * - Mask: What do I hit? (e.g., setCollisionMask(GROUND))
 *  - PLAYER: Hits GROUND and PLATFORM
 *  - GROUND: Hits PLAYER
 *  - PLATFORM: Hits PLAYER
 */

namespace camerademo {

namespace Layers {
    const pixelroot32::physics::CollisionLayer PLAYER   = 1 << 0;
    const pixelroot32::physics::CollisionLayer GROUND   = 1 << 1;
    const pixelroot32::physics::CollisionLayer PLATFORM = 1 << 2;
}

}
