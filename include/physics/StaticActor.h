/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/PhysicsActor.h"

namespace pixelroot32::physics {

/**
 * @class StaticActor
 * @brief A physics body that does not move.
 * 
 * Static actors are used for environment elements like floors, walls, and platforms
 * that should block other actors but are themselves immovable.
 * They are optimized to skip integration and world bound resolution.
 */
class StaticActor : public pixelroot32::core::PhysicsActor {
public:
    /**
     * @brief Constructs a new StaticActor.
     * @param x X position.
     * @param y Y position.
     * @param w Width.
     * @param h Height.
     */
    StaticActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    /**
     * @brief Constructs a new StaticActor.
     * @param position Position vector.
     * @param w Width.
     * @param h Height.
     */
    StaticActor(pixelroot32::math::Vector2 position, int w, int h);

    /**
     * @brief Draws the actor.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

} // namespace pixelroot32::physics
