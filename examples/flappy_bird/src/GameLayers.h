#pragma once
#include <physics/CollisionTypes.h>

namespace flappy {

/**
 * @file GameLayers.h
 * @brief Collision layer bitmasks for Flappy Bird.
 */
namespace pr = pixelroot32;

namespace Layers {
    const pr::physics::CollisionLayer BIRD     = 1 << 0;
    const pr::physics::CollisionLayer PIPE     = 1 << 1;
    const pr::physics::CollisionLayer BOUNDS   = 1 << 2;
}
}
