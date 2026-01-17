#pragma once
#include <cstdint>

namespace pixelroot32::physics {
/**
 * @typedef CollisionLayer
 * @brief Bitmask representing collision layers.
 * 
 * Used to filter collisions between entities.
 * Example: PLAYER (0x1) | ENEMY (0x2).
 */
typedef uint16_t CollisionLayer; 

/**
 * @namespace DefaultLayers
 * @brief Common collision layer constants.
 */
namespace DefaultLayers {
    const CollisionLayer kNone = 0;      ///< No collision.
    const CollisionLayer kAll  = 0xFFFF; ///< Collides with everything.
}

}