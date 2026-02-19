/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/PhysicsActor.h"

namespace pixelroot32::physics {

/**
 * @class RigidActor
 * @brief A physics body fully simulated by the engine.
 * 
 * Rigid actors respond to gravity, forces, and impulses. They are used for
 * dynamic objects that should behave naturally, like falling crates or debris.
 */
class RigidActor : public pixelroot32::core::PhysicsActor {
protected:
    pixelroot32::math::Vector2 force; ///< Accumulated force for the current frame.

public:
    /**
     * @brief Constructs a new RigidActor.
     * @param x X position.
     * @param y Y position.
     * @param w Width.
     * @param h Height.
     */
    RigidActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    /**
     * @brief Constructs a new RigidActor.
     * @param position Position vector.
     * @param w Width.
     * @param h Height.
     */
    RigidActor(pixelroot32::math::Vector2 position, int w, int h);

    /**
     * @brief Applies a force to the center of mass.
     * @param f Force vector.
     */
    void applyForce(const pixelroot32::math::Vector2& f);

    /**
     * @brief Applies an instantaneous impulse (velocity change).
     * @param j Impulse vector.
     */
    void applyImpulse(const pixelroot32::math::Vector2& j);

    /**
     * @brief Integrates forces and velocity to update position.
     * @param dt Delta time as Scalar.
     */
    void integrate(pixelroot32::math::Scalar dt) override;

    /**
     * @brief Logic update called every frame.
     * @param deltaTime Elapsed time in ms.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the actor.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

} // namespace pixelroot32::physics
