#pragma once
#include <cstdint>

typedef uint16_t CollisionLayer; 

namespace DefaultLayers {
    const CollisionLayer kNone = 0;
    const CollisionLayer kAll  = 0xFFFF;
}