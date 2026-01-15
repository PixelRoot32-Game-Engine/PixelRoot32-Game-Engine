#pragma once
#include "physics/CollisionTypes.h"

namespace brickbreaker {

namespace Layers {
    const pixelroot32::physics::CollisionLayer BALL    = 1 << 0;
    const pixelroot32::physics::CollisionLayer BRICK   = 1 << 1;
    const pixelroot32::physics::CollisionLayer PADDLE  = 1 << 2;
}

}