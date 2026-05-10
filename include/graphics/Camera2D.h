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
 * @brief 2D camera for viewport management and smooth scrolling.
 *
 * Manages the viewable area of a scene, providing bounds clamping and
 * smooth follow-target behavior. The camera sits at a fixed position
 * until instructed to follow a target; its X/Y coordinates are negated
 * to become the Renderer's X/Y offset (so content scrolls "under" the camera).
 *
 * Bounds are enforced per-axis; if no bounds are set the camera is unclamped
 * on that axis. A target X or Y position outside bounds is clamped before
 * the camera moves.
 *
 * @note Camera2D does not own a Renderer. Call apply(renderer) to push the
 *       camera transform after updating.
 */
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
    /** @brief Pushes the camera transform into the renderer (sets display offset). */
    void apply(Renderer& renderer) const;
    void setViewportSize(int width, int height);

private:
    pixelroot32::math::Vector2 position;   ///< Camera position in world units.
    int viewportWidth;                    ///< Viewport width in pixels.
    int viewportHeight;                   ///< Viewport height in pixels.
    pixelroot32::math::Scalar minX;      ///< Left boundary in world units (unbounded if > maxX).
    pixelroot32::math::Scalar maxX;      ///< Right boundary in world units.
    pixelroot32::math::Scalar minY;      ///< Top boundary in world units.
    pixelroot32::math::Scalar maxY;      ///< Bottom boundary in world units.
};

} // namespace pixelroot32::graphics
