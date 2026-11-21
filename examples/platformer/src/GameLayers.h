#pragma once
#include "physics/CollisionTypes.h"

/**
 * @file GameLayers.h
 * @brief Collision layers and masks, with the role each bit plays in the game.
 *
 * The engine's filter is `(a.mask & b.layer) || (b.mask & a.layer)`, so one
 * side naming the other is enough for a contact. The masks below still state
 * the intent on both sides, which is what keeps a new actor type from
 * colliding with something it should ignore by accident.
 */

namespace platformerdemo {

namespace Layers {

constexpr pixelroot32::physics::CollisionLayer PLAYER = 1 << 0;   ///< The character.
constexpr pixelroot32::physics::CollisionLayer GROUND = 1 << 1;   ///< Solid scenery: floor, walls, the crate shelf.
constexpr pixelroot32::physics::CollisionLayer PLATFORM = 1 << 2; ///< One-way platforms: land from above, pass from below.
constexpr pixelroot32::physics::CollisionLayer PROP = 1 << 3;     ///< Rigid crates.

constexpr pixelroot32::physics::CollisionLayer PLAYER_MASK = GROUND | PLATFORM | PROP;
constexpr pixelroot32::physics::CollisionLayer GROUND_MASK = PLAYER | PROP;
/// Crates deliberately do not list PLATFORM, and one-way platforms do not list
/// PROP: a crate falls through a one-way platform instead of resting in mid-air.
constexpr pixelroot32::physics::CollisionLayer PLATFORM_MASK = PLAYER;
constexpr pixelroot32::physics::CollisionLayer PROP_MASK = GROUND | PLAYER | PROP;

}

}
