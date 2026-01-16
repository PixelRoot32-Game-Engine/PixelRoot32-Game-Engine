#pragma once
#include "physics/CollisionTypes.h"

namespace geometryjump {

namespace Layers {
    const pixelroot32::physics::CollisionLayer PLAYER = 1 << 0;
    const pixelroot32::physics::CollisionLayer GROUND = 1 << 1;
    const pixelroot32::physics::CollisionLayer OBSTACLE = 1 << 2;
}

}

