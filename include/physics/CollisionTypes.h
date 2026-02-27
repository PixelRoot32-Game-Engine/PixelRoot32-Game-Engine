/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <cstdint>
#include "math/Scalar.h"

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
    pixelroot32::math::Scalar x;
    pixelroot32::math::Scalar y;
    pixelroot32::math::Scalar radius;
};

struct Segment {
    pixelroot32::math::Scalar x1;
    pixelroot32::math::Scalar y1;
    pixelroot32::math::Scalar x2;
    pixelroot32::math::Scalar y2;
};

bool intersects(const Circle& a, const Circle& b);

bool intersects(const Circle& c, const pixelroot32::core::Rect& r);

bool intersects(const Segment& s, const pixelroot32::core::Rect& r);

bool sweepCircleVsRect(const Circle& start,
                       const Circle& end,
                       const pixelroot32::core::Rect& rect,
                       pixelroot32::math::Scalar& tHit);

}
