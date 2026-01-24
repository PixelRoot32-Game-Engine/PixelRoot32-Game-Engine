/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics {

Camera2D::Camera2D(int viewportWidth, int viewportHeight)
    : x(0.0f)
    , y(0.0f)
    , viewportWidth(viewportWidth)
    , viewportHeight(viewportHeight)
    , minX(0.0f)
    , maxX(0.0f)
    , minY(0.0f)
    , maxY(0.0f) {
}

void Camera2D::setBounds(float minXValue, float maxXValue) {
    minX = minXValue;
    maxX = maxXValue;
}

void Camera2D::setVerticalBounds(float minYValue, float maxYValue) {
    minY = minYValue;
    maxY = maxYValue;
}

void Camera2D::setPosition(float newX, float newY) {
    x = newX;
    y = newY;

    if (x < minX) x = minX;
    if (x > maxX) x = maxX;
    if (y < minY) y = minY;
    if (y > maxY) y = maxY;
}

void Camera2D::followTarget(float targetX) {
    float deadZoneLeft = viewportWidth * 0.3f;
    float deadZoneRight = viewportWidth * 0.7f;

    float screenX = targetX - x;

    if (screenX < deadZoneLeft) {
        float newX = targetX - deadZoneLeft;
        setPosition(newX, y);
    } else if (screenX > deadZoneRight) {
        float newX = targetX - deadZoneRight;
        setPosition(newX, y);
    }
}

void Camera2D::followTarget(float targetX, float targetY) {
    float deadZoneLeft = viewportWidth * 0.3f;
    float deadZoneRight = viewportWidth * 0.7f;

    float screenX = targetX - x;

    if (screenX < deadZoneLeft) {
        float newX = targetX - deadZoneLeft;
        setPosition(newX, y);
    } else if (screenX > deadZoneRight) {
        float newX = targetX - deadZoneRight;
        setPosition(newX, y);
    }

    float deadZoneTop = viewportHeight * 0.3f;
    float deadZoneBottom = viewportHeight * 0.7f;

    float screenY = targetY - y;

    if (screenY < deadZoneTop) {
        float newY = targetY - deadZoneTop;
        setPosition(x, newY);
    } else if (screenY > deadZoneBottom) {
        float newY = targetY - deadZoneBottom;
        setPosition(x, newY);
    }
}

float Camera2D::getX() const {
    return x;
}

float Camera2D::getY() const {
    return y;
}

void Camera2D::apply(Renderer& renderer) const {
    renderer.setDisplayOffset(static_cast<int>(-x), static_cast<int>(-y));
}

} // namespace pixelroot32::graphics
