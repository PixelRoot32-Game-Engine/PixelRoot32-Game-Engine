/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/CollisionTypes.h"
#include "core/Entity.h"
#include "math/MathUtil.h"

namespace pixelroot32::physics {

using pixelroot32::core::Rect;
using pixelroot32::math::Scalar;
using pixelroot32::math::toScalar;

bool intersects(const Circle& a, const Circle& b) {
    Scalar dx = a.x - b.x;
    Scalar dy = a.y - b.y;
    Scalar r = a.radius + b.radius;
    return dx * dx + dy * dy <= r * r;
}

bool intersects(const Circle& c, const Rect& r) {
    Scalar closestX = c.x;
    Scalar closestY = c.y;
    Scalar rX = r.x;
    Scalar rY = r.y;
    Scalar rW = toScalar(r.width);
    Scalar rH = toScalar(r.height);

    if (closestX < rX) closestX = rX;
    else if (closestX > rX + rW) closestX = rX + rW;

    if (closestY < rY) closestY = rY;
    else if (closestY > rY + rH) closestY = rY + rH;

    Scalar dx = c.x - closestX;
    Scalar dy = c.y - closestY;
    return dx * dx + dy * dy <= c.radius * c.radius;
}

bool intersects(const Segment& s, const Rect& r) {
    Scalar x1 = s.x1;
    Scalar y1 = s.y1;
    Scalar x2 = s.x2;
    Scalar y2 = s.y2;
    Scalar rX = r.x;
    Scalar rY = r.y;
    Scalar rW = toScalar(r.width);
    Scalar rH = toScalar(r.height);

    Scalar dx = x2 - x1;
    Scalar dy = y2 - y1;

    Scalar tMin = toScalar(0.0f);
    Scalar tMax = toScalar(1.0f);
    Scalar zero = toScalar(0.0f);

    if (dx != zero) {
        Scalar invDx = toScalar(1.0f) / dx;
        Scalar tx1 = (rX - x1) * invDx;
        Scalar tx2 = (rX + rW - x1) * invDx;
        if (tx1 > tx2) {
            Scalar tmp = tx1;
            tx1 = tx2;
            tx2 = tmp;
        }
        if (tx1 > tMin) tMin = tx1;
        if (tx2 < tMax) tMax = tx2;
        if (tMin > tMax) return false;
    } else {
        if (x1 < rX || x1 > rX + rW) {
            return false;
        }
    }

    if (dy != zero) {
        Scalar invDy = toScalar(1.0f) / dy;
        Scalar ty1 = (rY - y1) * invDy;
        Scalar ty2 = (rY + rH - y1) * invDy;
        if (ty1 > ty2) {
            Scalar tmp = ty1;
            ty1 = ty2;
            ty2 = tmp;
        }
        if (ty1 > tMin) tMin = ty1;
        if (ty2 < tMax) tMax = ty2;
        if (tMin > tMax) return false;
    } else {
        if (y1 < rY || y1 > rY + rH) {
            return false;
        }
    }

    return tMax >= zero && tMin <= toScalar(1.0f);
}

bool sweepCircleVsRect(const Circle& start,
                       const Circle& end,
                       const Rect& rect,
                       Scalar& tHit) {
    // Expand the rectangle by the circle radius and perform a segment cast against it.
    Scalar rxMin = rect.x - start.radius;
    Scalar ryMin = rect.y - start.radius;
    Scalar rxMax = rect.x + toScalar(rect.width)  + start.radius;
    Scalar ryMax = rect.y + toScalar(rect.height) + start.radius;

    // Early out: if start position already inside expanded rect
    if (start.x >= rxMin && start.x <= rxMax &&
        start.y >= ryMin && start.y <= ryMax) {
        tHit = toScalar(0.0f);
        return true;
    }

    Scalar x1 = start.x;
    Scalar y1 = start.y;
    Scalar x2 = end.x;
    Scalar y2 = end.y;

    Scalar dx = x2 - x1;
    Scalar dy = y2 - y1;

    Scalar tEnter = toScalar(0.0f);
    Scalar tExit  = toScalar(1.0f);
    Scalar zero   = toScalar(0.0f);

    // X slab
    if (dx != zero) {
        Scalar invDx = toScalar(1.0f) / dx;
        Scalar tx1 = (rxMin - x1) * invDx;
        Scalar tx2 = (rxMax - x1) * invDx;
        if (tx1 > tx2) { Scalar tmp = tx1; tx1 = tx2; tx2 = tmp; }
        if (tx1 > tEnter) tEnter = tx1;
        if (tx2 < tExit)  tExit  = tx2;
        if (tEnter > tExit) return false;
    } else {
        // Parallel to Y axis: must be within the slab
        if (x1 < rxMin || x1 > rxMax) return false;
    }

    // Y slab
    if (dy != zero) {
        Scalar invDy = toScalar(1.0f) / dy;
        Scalar ty1 = (ryMin - y1) * invDy;
        Scalar ty2 = (ryMax - y1) * invDy;
        if (ty1 > ty2) { Scalar tmp = ty1; ty1 = ty2; ty2 = tmp; }
        if (ty1 > tEnter) tEnter = ty1;
        if (ty2 < tExit)  tExit  = ty2;
        if (tEnter > tExit) return false;
    } else {
        // Parallel to X axis: must be within the slab
        if (y1 < ryMin || y1 > ryMax) return false;
    }

    if (tEnter < toScalar(0.0f) || tEnter > toScalar(1.0f)) return false;
    tHit = tEnter;
    return true;
}

}
