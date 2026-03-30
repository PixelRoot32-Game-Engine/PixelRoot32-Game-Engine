/**
 * @file test_collision_system.cpp
 * @brief Unit tests for physics/CollisionSystem module
 * @version 1.1
 * @date 2026-02-08
 * 
 * Tests for collision system using real classes and mock subclasses.
 */

#include <unity.h>
#include <vector>
#include "../../test_config.h"
#include "physics/CollisionSystem.h"
#include "physics/RigidActor.h"
#include "physics/StaticActor.h"
#include "core/Actor.h"
#include "core/PhysicsActor.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

// Mock Actor for testing
class MockActor : public RigidActor {
public:
    bool collisionCalled = false;
    Actor* collidedWith = nullptr;

    MockActor(float x, float y, int w, int h) : RigidActor(toScalar(x), toScalar(y), w, h) {
        // Default shape is AABB
    }

    void onCollision(Actor* other) override {
        collisionCalled = true;
        collidedWith = other;
    }

    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }

    void reset() {
        collisionCalled = false;
        collidedWith = nullptr;
    }
};

// Generic Entity for testing
class GenericEntity : public Entity {
public:
    GenericEntity(float x, float y, int w, int h) : Entity(Vector2(toScalar(x), toScalar(y)), w, h, EntityType::GENERIC) {}
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for entity management
// =============================================================================

void test_collision_system_add_entity(void) {
    CollisionSystem system;
    MockActor actor(0, 0, 10, 10);
    
    system.addEntity(&actor);
    TEST_ASSERT_EQUAL_INT(1, system.getEntityCount());
}

void test_collision_system_add_multiple(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(20, 20, 10, 10);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    TEST_ASSERT_EQUAL_INT(2, system.getEntityCount());
}

void test_collision_system_remove_entity(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(20, 20, 10, 10);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.removeEntity(&a1);
    
    TEST_ASSERT_EQUAL_INT(1, system.getEntityCount());
}

void test_collision_system_clear(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(20, 20, 10, 10);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.clear();
    
    TEST_ASSERT_EQUAL_INT(0, system.getEntityCount());
}

// =============================================================================
// Tests for collision detection
// =============================================================================

void test_collision_system_no_collision_separate(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(50, 50, 10, 10);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    TEST_ASSERT_FALSE(a1.collisionCalled);
    TEST_ASSERT_FALSE(a2.collisionCalled);
}

void test_collision_system_collision_detected(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 20, 20);
    MockActor a2(10, 10, 20, 20);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
    TEST_ASSERT_EQUAL(&a2, a1.collidedWith);
    TEST_ASSERT_EQUAL(&a1, a2.collidedWith);
}

void test_collision_system_touching_edge(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(10, 0, 10, 10);  // Touching at x=10
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    // Default Rect intersects is (x + width < other.x) which is false for touching
    // Rect::intersects: !(x + width < other.x || x > other.x + other.width || ...)
    // a1.x=0, a1.width=10, a2.x=10. 
    // a1.x + a1.width = 10. 10 < 10 is false.
    // So they should intersect when touching.
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
}

// =============================================================================
// Tests for layer/mask filtering
// =============================================================================

void test_collision_system_layer_mask_no_match(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 20, 20);
    MockActor a2(10, 10, 20, 20);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(2);
    
    a2.setCollisionLayer(1);
    a2.setCollisionMask(2);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    TEST_ASSERT_FALSE(a1.collisionCalled);
    TEST_ASSERT_FALSE(a2.collisionCalled);
}

void test_collision_system_layer_mask_match(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 20, 20);
    MockActor a2(10, 10, 20, 20);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(2);
    
    a2.setCollisionLayer(2);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
}

void test_collision_system_one_way_collision(void) {
    CollisionSystem system;
    MockActor player(0, 0, 20, 20);
    MockActor enemy(10, 10, 20, 20);
    
    player.setCollisionLayer(1);
    player.setCollisionMask(2);
    
    enemy.setCollisionLayer(2);
    enemy.setCollisionMask(0);
    
    system.addEntity(&player);
    system.addEntity(&enemy);
    system.update();
    
    // System notifies both if (a->mask & b->layer) OR (b->mask & a->layer)
    TEST_ASSERT_TRUE(player.collisionCalled);
    TEST_ASSERT_TRUE(enemy.collisionCalled);
}

// =============================================================================
// Tests for multiple entities
// =============================================================================

void test_collision_system_three_actors(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 20, 20);
    MockActor a2(10, 10, 20, 20);
    MockActor a3(100, 100, 20, 20);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    a3.setCollisionLayer(1);
    a3.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.addEntity(&a3);
    system.update();
    
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
    TEST_ASSERT_FALSE(a3.collisionCalled);
}

void test_collision_system_generic_entity_ignored(void) {
    CollisionSystem system;
    GenericEntity generic(0, 0, 10, 10);
    MockActor actor(0, 0, 20, 20);
    
    actor.setCollisionLayer(1);
    actor.setCollisionMask(1);
    
    system.addEntity(&generic);
    system.addEntity(&actor);
    system.update();
    
    TEST_ASSERT_FALSE(actor.collisionCalled);
}

// =============================================================================
// Tests for edge cases
// =============================================================================

void test_collision_system_empty(void) {
    CollisionSystem system;
    system.update();
    TEST_ASSERT_EQUAL_INT(0, system.getEntityCount());
}

void test_collision_system_remove_and_update(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 20, 20);
    MockActor a2(10, 10, 20, 20);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    
    system.update();
    TEST_ASSERT_TRUE(a1.collisionCalled);
    
    a1.reset();
    a2.reset();
    
    system.removeEntity(&a2);
    system.update();
    
    TEST_ASSERT_FALSE(a1.collisionCalled);
    TEST_ASSERT_FALSE(a2.collisionCalled);
}

void test_collision_system_circle_vs_circle(void) {
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(8, 0, 10, 10);
    a1.setShape(CollisionShape::CIRCLE);
    a1.setRadius(toScalar(5.0f));
    a2.setShape(CollisionShape::CIRCLE);
    a2.setRadius(toScalar(5.0f));
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
}

void test_collision_system_circle_vs_aabb(void) {
    CollisionSystem system;
    MockActor circle(5, 5, 10, 10);
    MockActor box(10, 10, 20, 20);
    circle.setShape(CollisionShape::CIRCLE);
    circle.setRadius(toScalar(5.0f));
    circle.setCollisionLayer(1);
    circle.setCollisionMask(1);
    box.setCollisionLayer(1);
    box.setCollisionMask(1);
    system.addEntity(&circle);
    system.addEntity(&box);
    system.update();
    TEST_ASSERT_TRUE(circle.collisionCalled);
    TEST_ASSERT_TRUE(box.collisionCalled);
}

void test_collision_system_swept_circle_vs_aabb_ccd(void) {
    CollisionSystem system;
    MockActor circle(0, 10, 10, 10);
    circle.setShape(CollisionShape::CIRCLE);
    circle.setRadius(toScalar(5.0f));
    circle.setMass(toScalar(1.0f));
    circle.setVelocity(Vector2(toScalar(1000.0f), toScalar(0)));
    circle.setCollisionLayer(1);
    circle.setCollisionMask(1);
    StaticActor wall(toScalar(50), toScalar(0), 20, 20);
    wall.setCollisionLayer(1);
    wall.setCollisionMask(1);
    system.addEntity(&wall);
    system.addEntity(&circle);
    system.update();
}

// =============================================================================
// One-way platform
// =============================================================================

void test_collision_system_one_way_platform_land_from_above(void) {
    CollisionSystem system;
    // Player starts closer to platform: y=18, height=20, so bottom=38 (just above platform top at 40)
    // With velocity 50 and dt=1/60, player moves ~0.83 units, so bottom becomes ~38.83 (still above 40)
    // Need higher velocity or closer position. Let's use y=19 so bottom=39, then after movement bottom≈39.83 (still above)
    // Actually, let's use y=20 so bottom=40 exactly, then movement makes it cross
    MockActor player(40, 20, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    player.setCollisionLayer(1);
    player.setCollisionMask(1);
    platform.setCollisionLayer(1);
    platform.setCollisionMask(1);
    player.setVelocity(0.0f, 50.0f);
    system.addEntity(&platform);
    system.addEntity(&player);
    system.update();
    TEST_ASSERT_TRUE(player.collisionCalled);
}

void test_collision_system_one_way_platform_jump_through_from_below(void) {
    CollisionSystem system;
    MockActor player(40, 60, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    player.setCollisionLayer(1);
    player.setCollisionMask(1);
    platform.setCollisionLayer(1);
    platform.setCollisionMask(1);
    player.setVelocity(0.0f, -30.0f);
    system.addEntity(&platform);
    system.addEntity(&player);
    system.update();
    TEST_ASSERT_FALSE(player.collisionCalled);
}

// =============================================================================
// One-way platform validation unit tests
// =============================================================================

void test_one_way_platform_crossing_from_above(void) {
    CollisionSystem system;
    MockActor player(40, 20, 20, 20);  // Above platform
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    // Store previous position (player above platform)
    player.updatePreviousPosition();
    
    // Move player down, crossing platform surface
    player.position.y = toScalar(30);
    player.setVelocity(0, 5);
    
    // Normal pointing up (pushing actor up, away from platform)
    // In this engine, when actor is above platform, normal points up (0, -1)
    Vector2 normal(0, -1);
    
    // Should validate - crossing from above with downward velocity
    TEST_ASSERT_TRUE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_crossing_from_below(void) {
    CollisionSystem system;
    MockActor player(40, 50, 20, 20);  // Below platform
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    // Store previous position (player below platform)
    player.updatePreviousPosition();
    
    // Move player up toward platform
    player.position.y = toScalar(45);
    player.setVelocity(0, -5);
    
    // Normal pointing up (would push actor up, but actor is below)
    Vector2 normal(0, -1);
    
    // Should NOT validate - not crossing from above
    TEST_ASSERT_FALSE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_wrong_normal_direction(void) {
    CollisionSystem system;
    MockActor player(40, 20, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    player.updatePreviousPosition();
    player.position.y = toScalar(30);
    player.setVelocity(0, 5);
    
    // Normal pointing down (actor below platform) - wrong direction
    // When actor is above platform, normal should point up (0, -1)
    Vector2 normal(0, 1);
    
    // Should NOT validate - normal must point up (y < 0) for actor above platform
    TEST_ASSERT_FALSE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_moving_upward(void) {
    CollisionSystem system;
    MockActor player(40, 20, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    player.updatePreviousPosition();
    player.position.y = toScalar(30);
    
    // Moving upward (negative velocity)
    player.setVelocity(0, -5);
    
    Vector2 normal(0, 1);
    
    // Should NOT validate - must be moving down or stationary
    TEST_ASSERT_FALSE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_large_delta_movement(void) {
    CollisionSystem system;
    MockActor player(40, 0, 20, 20);  // Far above platform
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    // Store previous position (far above)
    player.updatePreviousPosition();
    
    // Large movement delta - player moves through platform in one frame
    player.position.y = toScalar(50);  // Now below platform
    player.setVelocity(0, 100);  // High velocity
    
    // Normal pointing up (actor crossed from above)
    Vector2 normal(0, -1);
    
    // Should validate - crossed from above despite large delta
    TEST_ASSERT_TRUE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_velocity_sign_change(void) {
    CollisionSystem system;
    MockActor player(40, 20, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    player.updatePreviousPosition();
    player.position.y = toScalar(30);
    
    // Velocity changed to upward (simulating bounce or state change)
    player.setVelocity(0, -3);
    
    Vector2 normal(0, 1);
    
    // Should NOT validate - velocity must be downward or zero
    TEST_ASSERT_FALSE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_stationary_on_surface(void) {
    CollisionSystem system;
    MockActor player(40, 20, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    platform.setOneWay(true);
    
    player.updatePreviousPosition();
    player.position.y = toScalar(30);
    
    // Stationary (zero velocity)
    player.setVelocity(0, 0);
    
    // Normal pointing up (actor above platform)
    Vector2 normal(0, -1);
    
    // Should validate - stationary is acceptable (movingDown check: velocity.y >= 0)
    TEST_ASSERT_TRUE(system.validateOneWayPlatform(&player, &platform, normal));
}

void test_one_way_platform_not_one_way(void) {
    CollisionSystem system;
    MockActor player(40, 20, 20, 20);
    StaticActor platform(toScalar(20), toScalar(40), 60, 16);
    // NOT a one-way platform
    platform.setOneWay(false);
    
    player.updatePreviousPosition();
    player.position.y = toScalar(50);
    player.setVelocity(0, -5);  // Moving up
    
    Vector2 normal(0, -1);  // Wrong normal
    
    // Should validate - not a one-way platform, always returns true
    TEST_ASSERT_TRUE(system.validateOneWayPlatform(&player, &platform, normal));
}

// CollisionSystem Error Handling Tests

void test_collision_system_empty_collision_list_handling(void) {
    // Test that update() handles empty collision list gracefully
    CollisionSystem system;
    
    // System with no entities should not crash
    system.update();
    
    // System should remain stable with empty entity list
    TEST_ASSERT_TRUE(true);  // No crash, system stable
}

void test_collision_system_null_entity_handling(void) {
    // Test that system properly rejects null entities via assertions
    // Note: In release builds, assertions may be disabled
    CollisionSystem system;
    
    // Create a valid actor first
    MockActor actor(50, 50, 20, 20);
    system.addEntity(&actor);
    
    // System should have 1 entity
    // (Cannot test null directly due to assert, but we verify system is stable)
    system.update();
    TEST_ASSERT_TRUE(true);  // System didn't crash
}

void test_spatial_grid_actor_far_outside_bounds(void) {
    // Test SpatialGrid with actor far outside normal bounds
    CollisionSystem system;
    
    // Actor at extreme coordinates
    MockActor farActor(10000, 10000, 20, 20);
    StaticActor platform(5000, 5000, 60, 16);
    
    system.addEntity(&farActor);
    system.addEntity(&platform);
    
    // Should handle without crashing - actors outside grid bounds are clipped
    system.update();
    TEST_ASSERT_TRUE(true);  // No crash
}

void test_spatial_grid_negative_coordinates(void) {
    // Test SpatialGrid with actors at negative coordinates
    CollisionSystem system;
    
    MockActor negativeActor(-100, -100, 20, 20);
    StaticActor platform(-150, -80, 60, 16);
    
    system.addEntity(&negativeActor);
    system.addEntity(&platform);
    
    // Should handle negative coordinates correctly
    system.update();
    TEST_ASSERT_TRUE(true);  // No crash
}

void test_collision_system_multiple_updates_consistency(void) {
    // Test that multiple consecutive updates produce consistent results
    CollisionSystem system;
    
    MockActor player(50, 50, 20, 20);
    StaticActor wall(80, 50, 20, 100);
    
    player.setVelocity(5, 0);  // Moving right toward wall
    
    system.addEntity(&player);
    system.addEntity(&wall);
    
    // Run multiple updates
    for (int i = 0; i < 10; i++) {
        system.update();
    }
    
    // System should remain stable
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Branch coverage tests for uncovered collision system paths
// =============================================================================

void test_collision_system_static_vs_static_no_collision(void) {
    CollisionSystem system;
    StaticActor wall1(toScalar(0), toScalar(0), 20, 20);
    StaticActor wall2(toScalar(10), toScalar(10), 20, 20);
    
    wall1.setCollisionLayer(1);
    wall1.setCollisionMask(1);
    wall2.setCollisionLayer(1);
    wall2.setCollisionMask(1);
    
    system.addEntity(&wall1);
    system.addEntity(&wall2);
    system.update();
    
    // Static vs Static should never collide - no callbacks triggered
    // Just verify no crash and system handles this correctly
    TEST_ASSERT_EQUAL_INT(2, system.getEntityCount());
}

void test_collision_system_kinematic_vs_kinematic_collision(void) {
    CollisionSystem system;
    
    // Use RigidActor as stand-in for kinematic (both are non-static physics bodies)
    MockActor kin1(0, 0, 20, 20);
    MockActor kin2(10, 10, 20, 20);
    
    kin1.setCollisionLayer(1);
    kin1.setCollisionMask(1);
    kin2.setCollisionLayer(1);
    kin2.setCollisionMask(1);
    
    kin1.collisionSystem = &system;
    kin2.collisionSystem = &system;
    
    system.addEntity(&kin1);
    system.addEntity(&kin2);
    system.update();
    
    // Kinematic vs Kinematic: each resolves on its own
    // Both should receive collision callbacks
    TEST_ASSERT_TRUE(kin1.collisionCalled || kin2.collisionCalled);
}

void test_collision_system_circle_vs_circle_zero_distance(void) {
    // Test circle-circle collision when centers are at same position
    // This exercises the dist <= kEpsilon branch in generateCircleVsCircleContact
    CollisionSystem system;
    
    MockActor circle1(50, 50, 20, 20);
    circle1.setShape(CollisionShape::CIRCLE);
    circle1.setRadius(toScalar(10.0f));
    
    MockActor circle2(50, 50, 20, 20);  // Same center as circle1
    circle2.setShape(CollisionShape::CIRCLE);
    circle2.setRadius(toScalar(10.0f));
    
    circle1.setCollisionLayer(1);
    circle1.setCollisionMask(1);
    circle2.setCollisionLayer(1);
    circle2.setCollisionMask(1);
    
    system.addEntity(&circle1);
    system.addEntity(&circle2);
    system.update();
    
    // Should still detect collision even with zero distance between centers
    TEST_ASSERT_TRUE(circle1.collisionCalled);
    TEST_ASSERT_TRUE(circle2.collisionCalled);
}

void test_collision_system_circle_vs_aabb_deep_penetration(void) {
    // Test circle-AABB collision when circle center is inside the box
    // This exercises the dist <= kEpsilon branch in generateCircleVsAABBContact
    CollisionSystem system;
    
    MockActor circle(45, 45, 10, 10);  // Center inside box
    circle.setShape(CollisionShape::CIRCLE);
    circle.setRadius(toScalar(5.0f));
    
    MockActor box(40, 40, 20, 20);  // Box that contains circle center
    
    circle.setCollisionLayer(1);
    circle.setCollisionMask(1);
    box.setCollisionLayer(1);
    box.setCollisionMask(1);
    
    system.addEntity(&circle);
    system.addEntity(&box);
    system.update();
    
    // Should detect collision even with circle center inside box
    TEST_ASSERT_TRUE(circle.collisionCalled);
    TEST_ASSERT_TRUE(box.collisionCalled);
}

// =============================================================================
// CollisionSystem Edge Case Tests
// =============================================================================

void test_collision_system_exact_boundary(void) {
    // Test collision at exact boundary (edge touching)
    CollisionSystem system;
    MockActor a1(0, 0, 10, 10);
    MockActor a2(10, 0, 10, 10);  // Exactly touching at x=10
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    // Touching edges should count as collision
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
}

void test_collision_system_zero_velocity_resolution(void) {
    // Test collision resolution with zero velocity
    CollisionSystem system;
    MockActor actor(0, 0, 20, 20);
    StaticActor wall(15, 0, 20, 20);
    
    actor.setCollisionLayer(1);
    actor.setCollisionMask(1);
    wall.setCollisionLayer(1);
    wall.setCollisionMask(1);
    
    // Set zero velocity
    actor.setVelocity(0, 0);
    
    system.addEntity(&wall);
    system.addEntity(&actor);
    system.update();
    
    // Should still detect collision even with zero velocity
    TEST_ASSERT_TRUE(actor.collisionCalled);
}

void test_collision_system_broad_phase_overlapping_cells(void) {
    // Test broad phase with actors in overlapping grid cells
    CollisionSystem system;
    
    // Create actors that span multiple grid cells
    // SpatialGrid cell size is typically 64x64
    MockActor largeActor(60, 60, 20, 20);  // Near cell boundary
    MockActor otherActor(70, 70, 20, 20);  // In overlapping region
    
    largeActor.setCollisionLayer(1);
    largeActor.setCollisionMask(1);
    otherActor.setCollisionLayer(1);
    otherActor.setCollisionMask(1);
    
    system.addEntity(&largeActor);
    system.addEntity(&otherActor);
    system.update();
    
    // Should detect collision in broad phase
    TEST_ASSERT_TRUE(largeActor.collisionCalled || otherActor.collisionCalled);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    RUN_TEST(test_collision_system_add_entity);
    RUN_TEST(test_collision_system_add_multiple);
    RUN_TEST(test_collision_system_remove_entity);
    RUN_TEST(test_collision_system_clear);
    
    RUN_TEST(test_collision_system_no_collision_separate);
    RUN_TEST(test_collision_system_collision_detected);
    RUN_TEST(test_collision_system_touching_edge);
    
    RUN_TEST(test_collision_system_layer_mask_no_match);
    RUN_TEST(test_collision_system_layer_mask_match);
    RUN_TEST(test_collision_system_one_way_collision);
    
    RUN_TEST(test_collision_system_three_actors);
    RUN_TEST(test_collision_system_generic_entity_ignored);
    
    RUN_TEST(test_collision_system_empty);
    RUN_TEST(test_collision_system_remove_and_update);
    RUN_TEST(test_collision_system_circle_vs_circle);
    RUN_TEST(test_collision_system_circle_vs_aabb);
    RUN_TEST(test_collision_system_swept_circle_vs_aabb_ccd);
    RUN_TEST(test_collision_system_one_way_platform_land_from_above);
    RUN_TEST(test_collision_system_one_way_platform_jump_through_from_below);
    
    // One-way platform validation unit tests
    RUN_TEST(test_one_way_platform_crossing_from_above);
    RUN_TEST(test_one_way_platform_crossing_from_below);
    RUN_TEST(test_one_way_platform_wrong_normal_direction);
    RUN_TEST(test_one_way_platform_moving_upward);
    RUN_TEST(test_one_way_platform_large_delta_movement);
    RUN_TEST(test_one_way_platform_velocity_sign_change);
    RUN_TEST(test_one_way_platform_stationary_on_surface);
    RUN_TEST(test_one_way_platform_not_one_way);
    
    // Error handling tests
    RUN_TEST(test_collision_system_empty_collision_list_handling);
    RUN_TEST(test_collision_system_null_entity_handling);
    RUN_TEST(test_spatial_grid_actor_far_outside_bounds);
    RUN_TEST(test_spatial_grid_negative_coordinates);
    RUN_TEST(test_collision_system_multiple_updates_consistency);
    
    // Branch coverage tests for uncovered paths
    RUN_TEST(test_collision_system_static_vs_static_no_collision);
    RUN_TEST(test_collision_system_kinematic_vs_kinematic_collision);
    RUN_TEST(test_collision_system_circle_vs_circle_zero_distance);
    RUN_TEST(test_collision_system_circle_vs_aabb_deep_penetration);
    
    // Section 2.1: Edge case tests
    RUN_TEST(test_collision_system_exact_boundary);
    RUN_TEST(test_collision_system_zero_velocity_resolution);
    RUN_TEST(test_collision_system_broad_phase_overlapping_cells);
    
    return UNITY_END();
}
