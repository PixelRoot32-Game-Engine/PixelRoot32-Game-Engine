/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver - Minimalist Physics Pipeline
 * Order: Detect → Solve Velocity → Integrate Position → Solve Penetration
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include "physics/CollisionTypes.h"
#include "physics/SpatialGrid.h"
#include "math/Vector2.h"
#include "math/Scalar.h"
#include "core/Entity.h"
#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
#include "gameplay/InteractionTracker.h"
#endif

namespace pixelroot32::core { class Actor; class PhysicsActor; }

namespace pixelroot32::physics {

/**
 * @struct KinematicCollision
 * @brief Contains information about a collision involving a KinematicActor.
 */
struct KinematicCollision {
    pixelroot32::core::Actor* collider = nullptr; ///< The other actor involved in the collision.
    pixelroot32::math::Vector2 normal;            ///< The collision normal vector.
    pixelroot32::math::Vector2 position;          ///< The position of the collision.
    pixelroot32::math::Scalar travel;             ///< Distance traveled before collision.
    pixelroot32::math::Scalar remainder;          ///< Remaining distance to travel.
};

/**
 * @struct Contact
 * @brief Represents a contact point between two physics bodies.
 */
struct Contact {
    pixelroot32::core::PhysicsActor* bodyA = nullptr; ///< First body in the contact.
    pixelroot32::core::PhysicsActor* bodyB = nullptr; ///< Second body in the contact.
    pixelroot32::math::Vector2 normal;                ///< Contact normal vector.
    pixelroot32::math::Vector2 contactPoint;          ///< Point of contact.
    pixelroot32::math::Scalar penetration = pixelroot32::math::toScalar(0); ///< Penetration depth.
    pixelroot32::math::Scalar restitution = pixelroot32::math::toScalar(0); ///< Combined restitution coefficient.
    bool isSensorContact = false;  ///< True if either body is a sensor; no physics response applied.
};

/**
 * @class CollisionSystem
 * @brief Manages physics simulation and collision detection for all actors.
 */
class CollisionSystem {
public:
    static constexpr pixelroot32::math::Scalar FIXED_DT = pixelroot32::math::toScalar(1.0f / 60.0f);
    static constexpr pixelroot32::math::Scalar SLOP = pixelroot32::math::toScalar(0.02f);
    static constexpr pixelroot32::math::Scalar BIAS = pixelroot32::math::toScalar(0.2f);
    static constexpr pixelroot32::math::Scalar VELOCITY_THRESHOLD = pixelroot32::math::toScalar(0.5f);
    static constexpr pixelroot32::math::Scalar MIN_VELOCITY = pixelroot32::math::toScalar(0.01f);
    
    // Velocity enhancements (Phase 3)
    #ifdef PIXELROOT32_VELOCITY_DAMPING
        static constexpr pixelroot32::math::Scalar VELOCITY_DAMPING = pixelroot32::math::toScalar(PIXELROOT32_VELOCITY_DAMPING);
    #else
        static constexpr pixelroot32::math::Scalar VELOCITY_DAMPING = pixelroot32::math::toScalar(0.999f);
    #endif
    
    #ifdef PIXELROOT32_MAX_VELOCITY
        static constexpr pixelroot32::math::Scalar MAX_VELOCITY = pixelroot32::math::toScalar(PIXELROOT32_MAX_VELOCITY);
    #else
        static constexpr pixelroot32::math::Scalar MAX_VELOCITY = pixelroot32::math::toScalar(500.0f);
    #endif
    static constexpr int VELOCITY_ITERATIONS = pixelroot32::platforms::config::VelocityIterations;
    static constexpr pixelroot32::math::Scalar CCD_THRESHOLD = pixelroot32::math::toScalar(3.0f);
    
    /**
     * @brief Adds an entity to the collision system.
     * @param e Pointer to the entity to add.
     */
    void addEntity(pixelroot32::core::Entity* e);

    /**
     * @brief Removes an entity from the collision system.
     * @param e Pointer to the entity to remove.
     */
    void removeEntity(pixelroot32::core::Entity* e);

    /**
     * @brief Performs one complete physics update step.
     */
    void update();

    /**
     * @brief Detects collisions between all registered bodies.
     */
    void detectCollisions();

    /**
     * @brief Solves velocities for all contacts.
     */
    void solveVelocity();

    /**
     * @brief Integrates positions for all dynamic bodies.
     */
    void integratePositions();

    /**
     * @brief Solves penetration to separate overlapping bodies.
     */
    void solvePenetration();

    /**
     * @brief Triggers collision callbacks for all valid contacts.
     */
    void triggerCallbacks();

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    /**
     * @brief Sets the (optional, non-owning) interaction tracker that
     *        observes trigger enter/exit edges from triggerCallbacks().
     *
     * With no tracker set, triggerCallbacks() behaves exactly as it does
     * with the flag off (design.md D3). CollisionSystem does not own the
     * tracker; the caller (game or Scene::init()) is responsible for its
     * lifetime, mirroring SceneManager::setTransitionEffect().
     */
    void setInteractionTracker(pixelroot32::gameplay::InteractionTracker* tracker) {
        interactionTracker_ = tracker;
    }
#endif

    /**
     * @brief Gets the total number of registered entities.
     * @return Number of entities.
     */
    size_t getEntityCount() const { return entityCount; }

    /**
     * @brief Clears the collision system state.
     */
    void clear() {
        entityCount = 0;
        contactCount = 0;
        grid.clear();
#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
        if (interactionTracker_) interactionTracker_->reset();
#endif
    }

    /**
     * @brief Checks for collisions with a specific actor.
     * @param actor The actor to check.
     * @param outArray Array to store colliding actors.
     * @param count Reference to store the number of collisions found.
     * @param maxCount Maximum number of collisions to return.
     * @return True if any collisions were found.
     */
    bool checkCollision(pixelroot32::core::Actor* actor,
                       pixelroot32::core::Actor** outArray,
                       int& count, int maxCount);

#if PIXELROOT32_ENABLE_SPATIAL_QUERY
    /**
     * @brief Layer-aware radius query, backed by SpatialGrid::queryRadius().
     *
     * Returns every actor whose hitbox intersects the query circle AND whose
     * collision `layer` matches the caller-supplied @p mask.
     *
     * **Deliberate asymmetry vs. checkCollision().** checkCollision() tests
     * `(actor->mask & other->layer) || (other->mask & actor->layer)` because
     * both sides are actors with their own mask. An area query has no
     * anchor actor, so only the caller-supplied mask applies:
     * `(mask & other->layer) != 0`. This is a genuine behavioural
     * difference from checkCollision(), not an oversight (design.md D4).
     *
     * **Radius is clamped to prevent Q16.16 overflow.** In Q16.16,
     * squared-distance terms must fit the ±32768 range. The grid's cell-range
     * restriction bounds candidate separation to roughly
     * `radius + kCellSize`, so a radius up to `SPATIAL_QUERY_MAX_RADIUS`
     * (default 128, mirror `config::SpatialQueryMaxRadius`) keeps
     * `dx^2 <= (radius + kCellSize)^2 = 160^2 = 25600 < 32768`. Debug builds
     * `assert` on a radius above the limit; release builds clamp instead of
     * asserting. Distance comparisons are squared throughout — never `sqrt`.
     *
     * @warning Cross-scene visibility limitation (accepted, not fixed by
     * this capability): see SpatialGrid::queryRadius(). A query issued
     * against this CollisionSystem's grid MAY return static geometry
     * registered by another simultaneously-alive scene's
     * CollisionSystem/SpatialGrid. Pre-existing, documented, unguarded.
     *
     * @param center Query circle center.
     * @param radius Query circle radius (clamped/asserted, see above).
     * @param mask Caller-supplied layer mask; only `other->layer` is tested.
     * @param outArray Output array to store matching actors.
     * @param maxCount Maximum number of actors to write to outArray.
     * @return Number of actors written to outArray.
     */
    int queryRadius(pixelroot32::math::Vector2 center,
                    pixelroot32::math::Scalar radius,
                    CollisionLayer mask,
                    pixelroot32::core::Actor** outArray,
                    int maxCount);

    /**
     * @brief Layer-aware box query, backed by SpatialGrid::queryBox().
     *
     * Returns every actor whose hitbox intersects the query axis-aligned
     * rectangle AND whose collision `layer` matches the caller-supplied
     * @p mask. See queryRadius() for the deliberate asymmetry vs.
     * checkCollision() (only the caller-supplied mask is tested, not a
     * bidirectional actor/actor mask test) and for the cross-scene
     * static-geometry visibility limitation, both of which apply identically
     * here.
     *
     * @param box Query rectangle.
     * @param mask Caller-supplied layer mask; only `other->layer` is tested.
     * @param outArray Output array to store matching actors.
     * @param maxCount Maximum number of actors to write to outArray.
     * @return Number of actors written to outArray.
     */
    int queryBox(const pixelroot32::core::Rect& box,
                CollisionLayer mask,
                pixelroot32::core::Actor** outArray,
                int maxCount);
#endif

    /**
     * @brief Checks if a body requires continuous collision detection (CCD).
     * @param body The physics actor to check.
     * @return True if CCD is required.
     */
    bool needsCCD(pixelroot32::core::PhysicsActor* body) const;

    /**
     * @brief Performs a swept collision test between a circle and an AABB.
     * @param circle The moving circle.
     * @param box The static AABB.
     * @param outTime Reference to store the time of impact [0, 1].
     * @param outNormal Reference to store the collision normal.
     * @return True if a collision occurs.
     */
    bool sweptCircleVsAABB(pixelroot32::core::PhysicsActor* circle,
                          pixelroot32::core::PhysicsActor* box,
                          pixelroot32::math::Scalar& outTime,
                          pixelroot32::math::Vector2& outNormal);

    /**
     * @brief Validates if a collision with a one-way platform should occur.
     * @param actor The moving actor.
     * @param platform The one-way platform.
     * @param collisionNormal The contact normal.
     * @param motionStartHitboxBottom Hitbox bottom at the start of the current move probe
     *        (used by kinematic moveAndCollide). Negative to use previousPosition instead.
     * @return True if the collision is valid and should be resolved.
     */
    bool validateOneWayPlatform(
        pixelroot32::core::PhysicsActor* actor,
        pixelroot32::core::PhysicsActor* platform,
        const pixelroot32::math::Vector2& collisionNormal,
        pixelroot32::math::Scalar motionStartHitboxBottom = pixelroot32::math::toScalar(-1.0f)
    );

private:
    static constexpr int kMaxPairs = pixelroot32::platforms::config::PhysicsMaxPairs;
    static constexpr int kMaxContacts = pixelroot32::platforms::config::PhysicsMaxContacts;
    static constexpr int kVelocityIterations = pixelroot32::platforms::config::VelocityIterations;
    static constexpr uint16_t kMaxEntities = pixelroot32::platforms::config::PhysicsMaxEntities;

    struct CollisionPair {
        pixelroot32::core::Actor* a;
        pixelroot32::core::Actor* b;
    };

    // Fixed-size array instead of std::vector (zero heap allocation)
    pixelroot32::core::Entity* entities[kMaxEntities];
    uint16_t entityCount = 0;
    Contact contacts[kMaxContacts];
    int contactCount = 0;
    SpatialGrid grid;
    uint16_t nextEntityId = 1;  ///< Next id to assign on addEntity; 0 is reserved for "unregistered".

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    pixelroot32::gameplay::InteractionTracker* interactionTracker_ = nullptr; ///< Non-owning; set via setInteractionTracker().
#endif

    bool generateContact(pixelroot32::core::PhysicsActor* a, 
                         pixelroot32::core::PhysicsActor* b);
    bool generateCircleVsCircleContact(Contact& contact);
    bool generateAABBVsAABBContact(Contact& contact);
    bool generateCircleVsAABBContact(Contact& contact, 
                                     pixelroot32::core::PhysicsActor* circle,
                                     pixelroot32::core::PhysicsActor* box);
};

}
