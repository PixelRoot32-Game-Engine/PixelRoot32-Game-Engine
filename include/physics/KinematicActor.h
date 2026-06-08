/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/PhysicsActor.h"
#include "physics/CollisionSystem.h"

namespace pixelroot32::physics {

/**
 * @enum SnapPolicy
 * @brief Controls floor snap behavior after slide resolution.
 *
 * - None: slide only (default).
 * - Step: guarded snap post-step when wasSnapFloor was true at entry.
 * - Continuous: reserved for future continuous floor adhesion; currently no-op.
 */
enum class SnapPolicy { None, Step, Continuous };

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
    static constexpr pixelroot32::math::Scalar MIN_SNAP = pixelroot32::math::toScalar(4.0f); ///< Minimum snap magnitude for SnapPolicy::Step.

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
     * @brief Moves the body while sliding along surfaces, with optional floor snap.
     * @param velocity World velocity in units/sec (motion = velocity * dt).
     * @param dt Delta time in seconds. Required — no default.
     * @param upDirection Up vector for floor/ceiling/wall classification.
     * @param snapPolicy Snap behavior after slide (default: None).
     * @param snapVector Max snap distance toward -upDirection; zero disables snap.
     * @param outFloorBody Optional pointer to receive the floor PhysicsActor if on a KINEMATIC floor.
     * @return Resolved velocity after slide (+ snap when Step). Assign to caller velocity.
     *
     * @note SnapPolicy::Continuous is reserved; debug builds assert once and behave as None.
     * @note When jumping, pass snapVector=zero explicitly — snap is not auto-disabled on upward velocity.
     */
    [[nodiscard]] pixelroot32::math::Vector2 moveAndSlide(
        pixelroot32::math::Vector2 velocity,
        pixelroot32::math::Scalar dt,
        pixelroot32::math::Vector2 upDirection = {0, -1},
        SnapPolicy snapPolicy = SnapPolicy::None,
        pixelroot32::math::Vector2 snapVector = {},
        pixelroot32::core::PhysicsActor** outFloorBody = nullptr);

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
     * @brief Enables or disables top-surface validation for floor state and kinematic carry.
     * @param strict When true (default), floor contact requires horizontal overlap on the top face.
     */
    void setStrictTopSurfaceFloor(bool strict) { strictTopSurfaceFloor = strict; }

    /**
     * @brief Returns whether top-surface floor validation is active.
     */
    inline bool isStrictTopSurfaceFloor() const { return strictTopSurfaceFloor; }

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

    /// @brief Tracks whether the body was on floor at the end of the last moveAndSlide call.
    ///
    /// Used INTERNALLY by SnapPolicy::Step snap guard: snap only fires if the body
    /// was on floor the previous frame. This prevents snap from re-engaging when the body
    /// has left the floor (e.g., after a jump or walking off a ledge).
    bool wasSnapFloor = false;

    pixelroot32::math::Vector2 floorVelocity;                 ///< Persisted floor velocity from last KINEMATIC floor contact.
    pixelroot32::core::PhysicsActor* floorBody = nullptr;     ///< Current floor body pointer.
    pixelroot32::math::Vector2 lastFloorNormal;                ///< Last floor collision normal.
    bool strictTopSurfaceFloor = true;                        ///< When true, floor/carry requires top-surface support.

    bool hasTopSurfaceSupport(pixelroot32::core::PhysicsActor* floor);
    void resolvePreSlideDepenetration();
    void resolvePostSlideKinematicDepenetration(pixelroot32::core::PhysicsActor* targetBody);
    void applySnapStep(pixelroot32::math::Vector2 snapVector, pixelroot32::math::Vector2 upDirection,
                       pixelroot32::core::PhysicsActor*& localFloorBody);

    /**
     * @brief Internal slide loop. Iterates moveAndCollide to slide along surfaces.
     * @param currentMotion In/out: the motion vector to process (modified by slides).
     * @param upDirection Up vector for floor/ceiling/wall classification.
     * @param localFloorBody Out: set if floor collision is on a KINEMATIC body.
     */
    void slide(pixelroot32::math::Vector2& currentMotion, pixelroot32::math::Vector2 upDirection, pixelroot32::core::PhysicsActor*& localFloorBody);
};

} // namespace pixelroot32::physics
