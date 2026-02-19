/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "math/Scalar.h"
#include "math/Vector2.h"

namespace pixelroot32::graphics {

class Renderer;

class Camera2D {
public:
    Camera2D(int viewportWidth, int viewportHeight);

    void setBounds(pixelroot32::math::Scalar minX, pixelroot32::math::Scalar maxX);

    void setVerticalBounds(pixelroot32::math::Scalar minY, pixelroot32::math::Scalar maxY);

    void setPosition(pixelroot32::math::Vector2 position);

    void followTarget(pixelroot32::math::Scalar targetX);
    void followTarget(pixelroot32::math::Vector2 target);

    pixelroot32::math::Scalar getX() const;
    pixelroot32::math::Scalar getY() const;

    pixelroot32::math::Vector2 getPosition() const;

    void apply(Renderer& renderer) const;

    /**
     * @brief Sets the viewport size (usually logical resolution).
     * @param width Viewport width.
     * @param height Viewport height.
     */
    void setViewportSize(int width, int height);

private:
    pixelroot32::math::Vector2 position;
    int viewportWidth;
    int viewportHeight;
    pixelroot32::math::Scalar minX;
    pixelroot32::math::Scalar maxX;
    pixelroot32::math::Scalar minY;
    pixelroot32::math::Scalar maxY;
};

} // namespace pixelroot32::graphics
