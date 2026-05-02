/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "math/Scalar.h"
#include "math/Vector2.h"

namespace pixelroot32::graphics {

class Renderer;

/**
 * @class Camera2D
 * @brief 2D camera system for managing viewports and scrolling.
 */
class Camera2D {
public:
    /**
     * @brief Constructs a new Camera2D.
     * @param viewportWidth Width of the viewport.
     * @param viewportHeight Height of the viewport.
     */
    Camera2D(int viewportWidth, int viewportHeight);

    /**
     * @brief Sets the horizontal scrolling bounds.
     * @param minX Minimum X coordinate.
     * @param maxX Maximum X coordinate.
     */
    void setBounds(pixelroot32::math::Scalar minX, pixelroot32::math::Scalar maxX);

    /**
     * @brief Sets the vertical scrolling bounds.
     * @param minY Minimum Y coordinate.
     * @param maxY Maximum Y coordinate.
     */
    void setVerticalBounds(pixelroot32::math::Scalar minY, pixelroot32::math::Scalar maxY);

    /**
     * @brief Sets the camera's current position.
     * @param position The new position vector.
     */
    void setPosition(pixelroot32::math::Vector2 position);

    /**
     * @brief Makes the camera follow a target on the X axis.
     * @param targetX The target X coordinate to follow.
     */
    void followTarget(pixelroot32::math::Scalar targetX);

    /**
     * @brief Makes the camera follow a target in 2D space.
     * @param target The target position vector.
     */
    void followTarget(pixelroot32::math::Vector2 target);

    /**
     * @brief Gets the current X position.
     * @return The X coordinate.
     */
    pixelroot32::math::Scalar getX() const;

    /**
     * @brief Gets the current Y position.
     * @return The Y coordinate.
     */
    pixelroot32::math::Scalar getY() const;

    /**
     * @brief Gets the current position vector.
     * @return The position vector.
     */
    pixelroot32::math::Vector2 getPosition() const;

    /**
     * @brief Applies the camera transformation to a renderer.
     * @param renderer The renderer to apply the camera to.
     */
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
