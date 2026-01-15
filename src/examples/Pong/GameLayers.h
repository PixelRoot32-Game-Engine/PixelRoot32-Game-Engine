#pragma once
#include "physics/CollisionTypes.h"

namespace pr = pixelroot32;

namespace Layers {
    const pr::physics::CollisionLayer BALL    = 1 << 0;
    const pr::physics::CollisionLayer PADDLE  = 1 << 1;
}