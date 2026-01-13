#pragma once
#include "physics/CollisionTypes.h"

namespace Layers {
    const CollisionLayer BALL    = 1 << 0;
    const CollisionLayer BRICK   = 1 << 1;
    const CollisionLayer PADDLE  = 1 << 2;
}