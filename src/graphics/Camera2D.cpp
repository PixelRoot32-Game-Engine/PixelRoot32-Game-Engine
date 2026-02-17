/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics {

using pixelroot32::math::Scalar;
using pixelroot32::math::toScalar;

Camera2D::Camera2D(int viewportWidth, int viewportHeight)
    : x(0)
    , y(0)
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

void Camera2D::setPosition(Scalar newX, Scalar newY) {
    x = newX;
    y = newY;

    if (x < minX) x = minX;
    if (x > maxX) x = maxX;
    if (y < minY) y = minY;
    if (y > maxY) y = maxY;
}

void Camera2D::followTarget(Scalar targetX) {
    Scalar deadZoneLeft = Scalar(viewportWidth) * Scalar(0.3f);
    Scalar deadZoneRight = Scalar(viewportWidth) * Scalar(0.7f);

    Scalar screenX = targetX - x;

    if (screenX < deadZoneLeft) {
        Scalar newX = targetX - deadZoneLeft;
        setPosition(newX, y);
    } else if (screenX > deadZoneRight) {
        Scalar newX = targetX - deadZoneRight;
        setPosition(newX, y);
    }
}

void Camera2D::followTarget(Scalar targetX, Scalar targetY) {
    Scalar deadZoneLeft = Scalar(viewportWidth) * Scalar(0.3f);
    Scalar deadZoneRight = Scalar(viewportWidth) * Scalar(0.7f);

    Scalar screenX = targetX - x;

    if (screenX < deadZoneLeft) {
        Scalar newX = targetX - deadZoneLeft;
        setPosition(newX, y);
    } else if (screenX > deadZoneRight) {
        Scalar newX = targetX - deadZoneRight;
        setPosition(newX, y);
    }

    Scalar deadZoneTop = Scalar(viewportHeight) * Scalar(0.3f);
    Scalar deadZoneBottom = Scalar(viewportHeight) * Scalar(0.7f);

    Scalar screenY = targetY - y;

    if (screenY < deadZoneTop) {
        Scalar newY = targetY - deadZoneTop;
        setPosition(x, newY);
    } else if (screenY > deadZoneBottom) {
        Scalar newY = targetY - deadZoneBottom;
        setPosition(x, newY);
    }
}

Scalar Camera2D::getX() const {
    return x;
}

Scalar Camera2D::getY() const {
    return y;
}

void Camera2D::apply(Renderer& renderer) const {
    renderer.setDisplayOffset(static_cast<int>(-x), static_cast<int>(-y));
}

void Camera2D::setViewportSize(int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
}

} // namespace pixelroot32::graphics
