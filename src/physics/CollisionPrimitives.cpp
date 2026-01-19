/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#include "physics/CollisionTypes.h"
#include "core/Entity.h"

namespace pixelroot32::physics {

using pixelroot32::core::Rect;

bool intersects(const Circle& a, const Circle& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float r = a.radius + b.radius;
    return dx * dx + dy * dy <= r * r;
}

bool intersects(const Circle& c, const Rect& r) {
    float closestX = c.x;
    float closestY = c.y;

    if (closestX < r.x) closestX = r.x;
    else if (closestX > r.x + r.width) closestX = r.x + r.width;

    if (closestY < r.y) closestY = r.y;
    else if (closestY > r.y + r.height) closestY = r.y + r.height;

    float dx = c.x - closestX;
    float dy = c.y - closestY;
    return dx * dx + dy * dy <= c.radius * c.radius;
}

bool intersects(const Segment& s, const Rect& r) {
    float x1 = s.x1;
    float y1 = s.y1;
    float x2 = s.x2;
    float y2 = s.y2;

    float dx = x2 - x1;
    float dy = y2 - y1;

    float tMin = 0.0f;
    float tMax = 1.0f;

    if (dx != 0.0f) {
        float invDx = 1.0f / dx;
        float tx1 = (r.x - x1) * invDx;
        float tx2 = (r.x + r.width - x1) * invDx;
        if (tx1 > tx2) {
            float tmp = tx1;
            tx1 = tx2;
            tx2 = tmp;
        }
        if (tx1 > tMin) tMin = tx1;
        if (tx2 < tMax) tMax = tx2;
        if (tMin > tMax) return false;
    } else {
        if (x1 < r.x || x1 > r.x + r.width) {
            return false;
        }
    }

    if (dy != 0.0f) {
        float invDy = 1.0f / dy;
        float ty1 = (r.y - y1) * invDy;
        float ty2 = (r.y + r.height - y1) * invDy;
        if (ty1 > ty2) {
            float tmp = ty1;
            ty1 = ty2;
            ty2 = tmp;
        }
        if (ty1 > tMin) tMin = ty1;
        if (ty2 < tMax) tMax = ty2;
        if (tMin > tMax) return false;
    } else {
        if (y1 < r.y || y1 > r.y + r.height) {
            return false;
        }
    }

    return tMax >= 0.0f && tMin <= 1.0f;
}

bool sweepCircleVsRect(const Circle& start,
                       const Circle& end,
                       const Rect& rect,
                       float& tHit) {
    Rect expanded;
    expanded.x = rect.x - start.radius;
    expanded.y = rect.y - start.radius;
    expanded.width = rect.width + static_cast<int>(start.radius * 2.0f);
    expanded.height = rect.height + static_cast<int>(start.radius * 2.0f);

    Segment path;
    path.x1 = start.x;
    path.y1 = start.y;
    path.x2 = end.x;
    path.y2 = end.y;

    float x1 = path.x1;
    float y1 = path.y1;
    float x2 = path.x2;
    float y2 = path.y2;

    float dx = x2 - x1;
    float dy = y2 - y1;

    float tMin = 0.0f;
    float tMax = 1.0f;

    if (dx != 0.0f) {
        float invDx = 1.0f / dx;
        float tx1 = (expanded.x - x1) * invDx;
        float tx2 = (expanded.x + expanded.width - x1) * invDx;
        if (tx1 > tx2) {
            float tmp = tx1;
            tx1 = tx2;
            tx2 = tmp;
        }
        if (tx1 > tMin) tMin = tx1;
        if (tx2 < tMax) tMax = tx2;
        if (tMin > tMax) return false;
    } else {
        if (x1 < expanded.x || x1 > expanded.x + expanded.width) {
            return false;
        }
    }

    if (dy != 0.0f) {
        float invDy = 1.0f / dy;
        float ty1 = (expanded.y - y1) * invDy;
        float ty2 = (expanded.y + expanded.height - y1) * invDy;
        if (ty1 > ty2) {
            float tmp = ty1;
            ty1 = ty2;
            ty2 = tmp;
        }
        if (ty1 > tMin) tMin = ty1;
        if (ty2 < tMax) tMax = ty2;
        if (tMin > tMax) return false;
    } else {
        if (y1 < expanded.y || y1 > expanded.y + expanded.height) {
            return false;
        }
    }

    if (tMax < 0.0f || tMin > 1.0f) {
        return false;
    }

    tHit = tMin;
    return true;
}

}

