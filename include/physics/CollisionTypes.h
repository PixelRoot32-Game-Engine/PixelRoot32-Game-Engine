#pragma once
#include <cstdint>

namespace pixelroot32 {
namespace core {
    struct Rect;
}
}

namespace pixelroot32::physics {

typedef uint16_t CollisionLayer;

namespace DefaultLayers {
    const CollisionLayer kNone = 0;
    const CollisionLayer kAll  = 0xFFFF;
}

struct Circle {
    float x;
    float y;
    float radius;
};

struct Segment {
    float x1;
    float y1;
    float x2;
    float y2;
};

bool intersects(const Circle& a, const Circle& b);

bool intersects(const Circle& c, const pixelroot32::core::Rect& r);

bool intersects(const Segment& s, const pixelroot32::core::Rect& r);

bool sweepCircleVsRect(const Circle& start,
                       const Circle& end,
                       const pixelroot32::core::Rect& rect,
                       float& tHit);

}
