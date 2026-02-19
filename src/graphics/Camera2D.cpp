/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics {

using pixelroot32::math::Scalar;
using pixelroot32::math::toScalar;
using pixelroot32::math::Vector2;

Camera2D::Camera2D(int viewportWidth, int viewportHeight)
    : position(0, 0)
    , viewportWidth(viewportWidth)
    , viewportHeight(viewportHeight)
    , minX(0)
    , maxX(0)
    , minY(0)
    , maxY(0) {
}

void Camera2D::setBounds(Scalar minXValue, Scalar maxXValue) {
    minX = minXValue;
    maxX = maxXValue;
}

void Camera2D::setVerticalBounds(Scalar minYValue, Scalar maxYValue) {
    minY = minYValue;
    maxY = maxYValue;
}

void Camera2D::setPosition(Vector2 newPos) {
    position = newPos;

    if (position.x < minX) position.x = minX;
    if (position.x > maxX) position.x = maxX;
    if (position.y < minY) position.y = minY;
    if (position.y > maxY) position.y = maxY;
}

void Camera2D::followTarget(Scalar targetX) {
    Scalar deadZoneLeft = Scalar(viewportWidth) * Scalar(0.3f);
    Scalar deadZoneRight = Scalar(viewportWidth) * Scalar(0.7f);

    Scalar screenX = targetX - position.x;

    if (screenX < deadZoneLeft) {
        position.x = targetX - deadZoneLeft;
        // Re-clamp
        if (position.x < minX) position.x = minX;
        if (position.x > maxX) position.x = maxX;
    } else if (screenX > deadZoneRight) {
        position.x = targetX - deadZoneRight;
        // Re-clamp
        if (position.x < minX) position.x = minX;
        if (position.x > maxX) position.x = maxX;
    }
}

void Camera2D::followTarget(Vector2 target) {
    // Horizontal follow
    followTarget(target.x);

    // Vertical follow
    Scalar deadZoneTop = Scalar(viewportHeight) * Scalar(0.3f);
    Scalar deadZoneBottom = Scalar(viewportHeight) * Scalar(0.7f);

    Scalar screenY = target.y - position.y;

    if (screenY < deadZoneTop) {
        position.y = target.y - deadZoneTop;
    } else if (screenY > deadZoneBottom) {
        position.y = target.y - deadZoneBottom;
    }

    // Re-clamp Y
    if (position.y < minY) position.y = minY;
    if (position.y > maxY) position.y = maxY;
}

Scalar Camera2D::getX() const {
    return position.x;
}

Scalar Camera2D::getY() const {
    return position.y;
}

Vector2 Camera2D::getPosition() const {
    return position;
}

void Camera2D::apply(Renderer& renderer) const {
    renderer.setDisplayOffset(static_cast<int>(-position.x), static_cast<int>(-position.y));
}

void Camera2D::setViewportSize(int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
}

} // namespace pixelroot32::graphics
