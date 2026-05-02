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

/**
 * @brief Collision layer identifier type.
 */
typedef uint16_t CollisionLayer;

/**
 * @namespace DefaultLayers
 * @brief Defines standard collision layers.
 */
namespace DefaultLayers {
    const CollisionLayer kNone = 0;
    const CollisionLayer kAll  = 0xFFFF;
}

/**
 * @struct Circle
 * @brief Represents a 2D circle for collision detection.
 */
struct Circle {
    pixelroot32::math::Scalar x;      ///< Center X coordinate.
    pixelroot32::math::Scalar y;      ///< Center Y coordinate.
    pixelroot32::math::Scalar radius; ///< Radius of the circle.
};

/**
 * @struct Segment
 * @brief Represents a 2D line segment for collision detection.
 */
struct Segment {
    pixelroot32::math::Scalar x1; ///< Start X coordinate.
    pixelroot32::math::Scalar y1; ///< Start Y coordinate.
    pixelroot32::math::Scalar x2; ///< End X coordinate.
    pixelroot32::math::Scalar y2; ///< End Y coordinate.
};

/**
 * @brief Checks intersection between two circles.
 * @param a First circle.
 * @param b Second circle.
 * @return True if circles intersect.
 */
bool intersects(const Circle& a, const Circle& b);

/**
 * @brief Checks intersection between a circle and a rectangle.
 * @param c The circle.
 * @param r The rectangle.
 * @return True if they intersect.
 */
bool intersects(const Circle& c, const pixelroot32::core::Rect& r);

/**
 * @brief Checks intersection between a line segment and a rectangle.
 * @param s The line segment.
 * @param r The rectangle.
 * @return True if they intersect.
 */
bool intersects(const Segment& s, const pixelroot32::core::Rect& r);

/**
 * @brief Sweeps a circle against a rectangle to find time of impact.
 * @param start Circle start position.
 * @param end Circle end position.
 * @param rect The static rectangle.
 * @param tHit Reference to store the time of impact.
 * @return True if a hit occurs along the sweep path.
 */
bool sweepCircleVsRect(const Circle& start,
                       const Circle& end,
                       const pixelroot32::core::Rect& rect,
                       pixelroot32::math::Scalar& tHit);

}
