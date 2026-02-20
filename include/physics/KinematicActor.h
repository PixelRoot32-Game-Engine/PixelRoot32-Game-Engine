/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/PhysicsActor.h"
#include "physics/CollisionSystem.h"

namespace pixelroot32::physics {

/**
 * @class KinematicActor
 * @brief A physics body moved via script/manual velocity with collision detection.
 * 
 * Kinematic actors are not affected by world gravity or forces but can detect
 * and react to collisions during movement. They provide methods like 
 * moveAndSlide for complex character movement.
 */
class KinematicActor : public pixelroot32::core::PhysicsActor {
public:
    /**
     * @brief Constructs a new KinematicActor.
     * @param x X position.
     * @param y Y position.
     * @param w Width.
     * @param h Height.
     */
    KinematicActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h);

    /**
     * @brief Constructs a new KinematicActor.
     * @param position Position vector.
     * @param w Width.
     * @param h Height.
     */
    KinematicActor(pixelroot32::math::Vector2 position, int w, int h);

    /**
     * @brief Moves the body along a vector and stops at the first collision.
     * @param motion The relative movement vector.
     * @param outCollision Pointer to store collision data if a hit occurs.
     * @param testOnly If true, checks for collision without moving.
     * @param safeMargin Extra margin for collision recovery.
     * @param recoveryAsCollision If true, depenetration is reported as collision.
     * @return true if a collision occurred.
     */
    bool moveAndCollide(pixelroot32::math::Vector2 motion, KinematicCollision* outCollision = nullptr, 
                        bool testOnly = false, pixelroot32::math::Scalar safeMargin = pixelroot32::math::Scalar(0.08f), 
                        bool recoveryAsCollision = false);

    /**
     * @brief Moves the body while sliding along surfaces.
     * @param velocity The velocity vector.
     * @param upDirection The up vector used to differentiate floor/ceiling (optional).
     */
    void moveAndSlide(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 upDirection = {0, -1});

    /**
     * @brief Returns true if the body collided with the ceiling.
     */
    inline bool is_on_ceiling() const { return onCeiling; }

    /**
     * @brief Returns true if the body collided with the floor.
     */
    inline bool is_on_floor() const { return onFloor; }

    /**
     * @brief Returns true if the body collided with a wall.
     */
    inline bool is_on_wall() const { return onWall; }

    /**
     * @brief Draws the actor.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    int maxSlides = 4; ///< Maximum number of slide iterations to prevent infinite loops.
    bool onFloor = false;
    bool onCeiling = false;
    bool onWall = false;
};

} // namespace pixelroot32::physics
