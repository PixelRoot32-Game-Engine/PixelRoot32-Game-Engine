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
     * @brief Returns true if the body collided with the ceiling.
     */
    inline bool is_on_ceiling() const { return onCeiling; }

    /**
     * @brief Returns true if the body collided with the floor.
     * 
     * Includes persistence: returns true for up to MAX_FLOOR_LOST_FRAMES
     * frames after losing floor contact (walk-off tolerance).
     */
    inline bool is_on_floor() const { return onFloor || (wasOnFloor && floorLostCounter < MAX_FLOOR_LOST_FRAMES); }

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
        wasOnFloor = false;
        floorLostCounter = 0;
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
    int maxSlides = 4; ///< Maximum number of slide iterations to prevent infinite loops.
    bool onFloor = false;
    bool onCeiling = false;
    bool onWall = false;

    // Floor state persistence (Godot-inspired moving platform riding)
    pixelroot32::math::Vector2 floorVelocity;                 ///< Persisted floor velocity from last KINEMATIC floor contact.
    pixelroot32::core::PhysicsActor* floorBody = nullptr;     ///< Current floor body pointer.
    bool wasOnFloor = false;                                   ///< Floor state persistence flag.
    int floorLostCounter = 0;                                  ///< Frames without floor contact.
    pixelroot32::math::Vector2 lastFloorNormal;                ///< Last floor collision normal.
    static constexpr int MAX_FLOOR_LOST_FRAMES = 2;            ///< Tolerance before losing floor.

    /**
     * @brief Updates floor persistence state.
     * @param onFloorThisFrame Whether floor contact was detected this frame.
     * @param floorBodyResult The floor body if on floor, nullptr otherwise.
     */
    void updateFloorState(bool onFloorThisFrame, pixelroot32::core::PhysicsActor* floorBodyResult);
};

} // namespace pixelroot32::physics
