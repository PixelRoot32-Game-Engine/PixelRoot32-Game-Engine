#pragma once
#include <physics/CollisionTypes.h>

/**
 * @file GameLayers.h
 * @brief Bitmask definitions for collision filtering
 * 
 * Usage:
 * - Layer: What am I? (e.g., setCollisionLayer(BALL))
 * - Mask: What do I hit? (e.g., setCollisionMask(WALL | PADDLE))
 */

namespace brickbreaker {

namespace Layers {
    const pixelroot32::physics::CollisionLayer BALL    = 1 << 0;
    const pixelroot32::physics::CollisionLayer BRICK   = 1 << 1;
    const pixelroot32::physics::CollisionLayer PADDLE  = 1 << 2;
    const pixelroot32::physics::CollisionLayer WALL    = 1 << 3;
}

}
