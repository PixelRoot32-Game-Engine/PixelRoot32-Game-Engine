/**
 * @file test_rigid_actor.cpp
 * @brief Unit tests for RigidActor class.
 * 
 * Tests RigidActor constructors, force application, impulse application,
 * integration, and physics simulation behavior.
 */

#include <unity.h>
#include "physics/RigidActor.h"
#include "physics/CollisionSystem.h"
#include "mocks/MockDrawSurface.h"
#include "../../test_config.h"

using namespace pixelroot32::physics;
using namespace pixelroot32::core;
using namespace pixelroot32::math;
using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Constructors
// =============================================================================

void test_rigid_actor_constructor_scalar(void) {
    RigidActor actor(toScalar(10.0f), toScalar(20.0f), 16, 16);
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(20, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(16, actor.width);
    TEST_ASSERT_EQUAL(16, actor.height);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::RIGID), 
                      static_cast<int>(actor.getBodyType()));
}

void test_rigid_actor_constructor_vector2(void) {
    Vector2 pos(toScalar(30.0f), toScalar(40.0f));
    RigidActor actor(pos, 8, 12);
    
    TEST_ASSERT_EQUAL(30, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(40, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(8, actor.width);
    TEST_ASSERT_EQUAL(12, actor.height);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::RIGID), 
                      static_cast<int>(actor.getBodyType()));
}

// =============================================================================
// Force Application
// =============================================================================

void test_rigid_actor_apply_force(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    
    // Apply force and integrate
    actor.applyForce(Vector2(toScalar(100.0f), toScalar(0)));
    actor.integrate(toScalar(1.0f)); // 1 second dt
    
    // Force = mass * acceleration -> acceleration = force / mass
    // velocity = acceleration * dt = 100 * 1 = 100
    TEST_ASSERT_EQUAL(100, static_cast<int>(actor.getVelocityX()));
}

void test_rigid_actor_apply_force_accumulates(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    
    // Apply multiple forces
    actor.applyForce(Vector2(toScalar(50.0f), toScalar(0)));
    actor.applyForce(Vector2(toScalar(50.0f), toScalar(0)));
    actor.integrate(toScalar(1.0f));
    
    // Total force = 100, velocity = 100
    TEST_ASSERT_EQUAL(100, static_cast<int>(actor.getVelocityX()));
}

void test_rigid_actor_apply_force_with_mass(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(2.0f); // mass = 2
    
    // Apply force and integrate
    actor.applyForce(Vector2(toScalar(100.0f), toScalar(0)));
    actor.integrate(toScalar(1.0f));
    
    // acceleration = force / mass = 100 / 2 = 50
    // velocity = 50 * 1 = 50
    TEST_ASSERT_EQUAL(50, static_cast<int>(actor.getVelocityX()));
}

void test_rigid_actor_force_reset_after_integrate(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    
    actor.applyForce(Vector2(toScalar(100.0f), toScalar(0)));
    actor.integrate(toScalar(1.0f));
    
    // Force should be reset after integrate, so second integrate without new force
    // should only apply gravity
    Vector2 velocityBefore = actor.getVelocity();
    actor.integrate(toScalar(1.0f));
    
    // Velocity should change due to gravity, not the original force
    TEST_ASSERT_TRUE(actor.getVelocityY() > velocityBefore.y);
}

// =============================================================================
// Impulse Application
// =============================================================================

void test_rigid_actor_apply_impulse(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    
    // Apply impulse: velocity change = impulse / mass
    actor.applyImpulse(Vector2(toScalar(50.0f), toScalar(0)));
    
    TEST_ASSERT_EQUAL(50, static_cast<int>(actor.getVelocityX()));
}

void test_rigid_actor_apply_impulse_with_mass(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(2.0f);
    
    // Apply impulse: velocity change = impulse / mass = 100 / 2 = 50
    actor.applyImpulse(Vector2(toScalar(100.0f), toScalar(0)));
    
    TEST_ASSERT_EQUAL(50, static_cast<int>(actor.getVelocityX()));
}

void test_rigid_actor_apply_impulse_accumulates(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    
    actor.applyImpulse(Vector2(toScalar(30.0f), toScalar(0)));
    actor.applyImpulse(Vector2(toScalar(20.0f), toScalar(0)));
    
    TEST_ASSERT_EQUAL(50, static_cast<int>(actor.getVelocityX()));
}

void test_rigid_actor_apply_impulse_zero_mass(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(0.0f); // Zero mass
    
    // Apply impulse with zero mass should not crash
    actor.applyImpulse(Vector2(toScalar(100.0f), toScalar(0)));
    
    // Velocity should remain 0 since mass is 0 (avoiding divide by zero)
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.getVelocityX()));
}

// =============================================================================
// Integration
// =============================================================================

void test_rigid_actor_integrate_applies_gravity(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    actor.setGravityScale(toScalar(1.0f));
    
    // Integrate with gravity
    actor.integrate(toScalar(1.0f));
    
    // Gravity should affect velocity (world gravity is ~200)
    // force.y = 200 * 1.0 * 1.0 = 200
    // acceleration.y = 200 / 1 = 200
    // velocity.y = 200 * 1 = 200
    TEST_ASSERT_TRUE(actor.getVelocityY() > toScalar(0));
}

void test_rigid_actor_integrate_with_gravity_scale(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    actor.setGravityScale(toScalar(0.5f)); // Half gravity
    
    actor.integrate(toScalar(1.0f));
    
    // Half gravity means half the velocity change
    RigidActor actor2(toScalar(0), toScalar(0), 10, 10);
    actor2.setMass(1.0f);
    actor2.setGravityScale(toScalar(1.0f));
    actor2.integrate(toScalar(1.0f));
    
    TEST_ASSERT_TRUE(actor.getVelocityY() < actor2.getVelocityY());
}

void test_rigid_actor_integrate_zero_gravity(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    actor.setGravityScale(toScalar(0.0f)); // No gravity
    
    actor.integrate(toScalar(1.0f));
    
    // No gravity means no vertical velocity change
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.getVelocityY()));
}

void test_rigid_actor_update_calls_integrate(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    
    // update() should call integrate()
    actor.update(16); // 16ms delta time (not used directly)
    
    // Gravity should have been applied
    TEST_ASSERT_TRUE(actor.getVelocityY() > toScalar(0));
}

// =============================================================================
// Friction
// =============================================================================

void test_rigid_actor_friction_reduces_velocity(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(1.0f);
    actor.setVelocity(100.0f, 0.0f);
    actor.setFriction(toScalar(0.1f)); // 10% friction
    
    actor.integrate(toScalar(1.0f));
    
    // Friction should reduce velocity
    // velocity *= (1 - friction * dt) = 100 * (1 - 0.1 * 1) = 90
    TEST_ASSERT_TRUE(actor.getVelocityX() < toScalar(100.0f));
}

// =============================================================================
// Body Type and Properties
// =============================================================================

void test_rigid_actor_is_physics_body(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_TRUE(actor.isPhysicsBody());
}

void test_rigid_actor_default_mass(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, static_cast<float>(actor.getMass()));
}

void test_rigid_actor_set_mass(void) {
    RigidActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(5.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, static_cast<float>(actor.getMass()));
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Constructor tests
    RUN_TEST(test_rigid_actor_constructor_scalar);
    RUN_TEST(test_rigid_actor_constructor_vector2);
    
    // Force tests
    RUN_TEST(test_rigid_actor_apply_force);
    RUN_TEST(test_rigid_actor_apply_force_accumulates);
    RUN_TEST(test_rigid_actor_apply_force_with_mass);
    RUN_TEST(test_rigid_actor_force_reset_after_integrate);
    
    // Impulse tests
    RUN_TEST(test_rigid_actor_apply_impulse);
    RUN_TEST(test_rigid_actor_apply_impulse_with_mass);
    RUN_TEST(test_rigid_actor_apply_impulse_accumulates);
    RUN_TEST(test_rigid_actor_apply_impulse_zero_mass);
    
    // Integration tests
    RUN_TEST(test_rigid_actor_integrate_applies_gravity);
    RUN_TEST(test_rigid_actor_integrate_with_gravity_scale);
    RUN_TEST(test_rigid_actor_integrate_zero_gravity);
    RUN_TEST(test_rigid_actor_update_calls_integrate);
    
    // Friction tests
    RUN_TEST(test_rigid_actor_friction_reduces_velocity);
    
    // Property tests
    RUN_TEST(test_rigid_actor_is_physics_body);
    RUN_TEST(test_rigid_actor_default_mass);
    RUN_TEST(test_rigid_actor_set_mass);
    
    return UNITY_END();
}
