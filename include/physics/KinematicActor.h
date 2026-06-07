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
 * Inherits from PhysicsActor.
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
     * @param outFloorBody Optional pointer to receive the floor PhysicsActor if on a KINEMATIC floor.
     * @param dt Delta time used to calculate the movement (default: FIXED_DT).
     */
    void moveAndSlide(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 upDirection = {0, -1}, pixelroot32::core::PhysicsActor** outFloorBody = nullptr, pixelroot32::math::Scalar dt = pixelroot32::physics::CollisionSystem::FIXED_DT);

    /**
     * @brief Moves the body while sliding along surfaces, then snaps to floor.
     * @param velocity The velocity vector (already scaled by dt).
     * @param snap Snap vector toward floor. Pass zero to disable.
     * @param upDirection Up direction for floor detection (default: {0,-1}).
     * @param dt Delta time (default: CollisionSystem::FIXED_DT).
     * @return The actual velocity after slide and snap processing.
     *         Assign to your velocity variable to replace post-slide zeroing.
     * 
     * Performs standard moveAndSlide along velocity, then pushes the AABB
     * along -upDirection by |snap| to attach to the floor. Returns the
     * actual velocity after collisions and snap are resolved.
     * 
     * @note When jumping, caller MUST pass a zero snap vector explicitly.
     *       The engine does NOT auto-disable snap on upward velocity.
     * @note Snap magnitudes below MIN_SNAP (4.0) are treated as disabled.
     */
    pixelroot32::math::Vector2 moveAndSlideWithSnap(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 snap, pixelroot32::math::Vector2 upDirection = {0,-1}, pixelroot32::math::Scalar dt = pixelroot32::physics::CollisionSystem::FIXED_DT);

    /**
     * @brief Returns true if the body collided with the ceiling.
     */
    inline bool is_on_ceiling() const { return onCeiling; }

    /**
     * @brief Returns true if the body collided with the floor this frame.
     * 
     * Returns only the current-frame raw contact state, with no
     * persistence across frames.
     */
    inline bool is_on_floor() const { return onFloor; }

    /**
     * @brief Gets the persisted floor velocity from the last KINEMATIC floor contact.
     * @return Reference to the floor velocity vector.
     */
    const pixelroot32::math::Vector2& getFloorVelocity() const { return floorVelocity; }

    /**
     * @brief Clears floor velocity and state.
     * 
     * Call this on jump to prevent platform velocity inheritance when airborne.
     */
     void clearFloorVelocity() {
        floorVelocity = pixelroot32::math::Vector2::ZERO();
        floorBody = nullptr;
    }

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
    static constexpr pixelroot32::math::Scalar MIN_SNAP = pixelroot32::math::toScalar(4.0f); ///< Minimum snap magnitude for moveAndSlideWithSnap.
    int maxSlides = 4; ///< Maximum number of slide iterations to prevent infinite loops.
    bool onFloor = false;
    bool onCeiling = false;
    bool onWall = false;

    // Floor state storage (v2 readiness: unused in v1, stores for platform velocity injection)
    pixelroot32::math::Vector2 floorVelocity;                 ///< Persisted floor velocity from last KINEMATIC floor contact.
    pixelroot32::core::PhysicsActor* floorBody = nullptr;     ///< Current floor body pointer.
    pixelroot32::math::Vector2 lastFloorNormal;                ///< Last floor collision normal.

    /**
     * @brief Internal slide loop. Iterates moveAndCollide to slide along surfaces.
     * @param currentMotion In/out: the motion vector to process (modified by slides).
     * @param upDirection Up vector for floor/ceiling/wall classification.
     * @param localFloorBody Out: set if floor collision is on a KINEMATIC body.
     */
    void slide(pixelroot32::math::Vector2& currentMotion, pixelroot32::math::Vector2 upDirection, pixelroot32::core::PhysicsActor*& localFloorBody);
};

} // namespace pixelroot32::physics
