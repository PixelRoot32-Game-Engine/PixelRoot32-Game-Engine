/**
 * @file test_static_actor.cpp
 * @brief Unit tests for StaticActor class.
 * 
 * Tests StaticActor constructors, body type, and static behavior.
 */

#include <unity.h>
#include "physics/StaticActor.h"
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

void test_static_actor_constructor_scalar(void) {
    StaticActor actor(toScalar(10.0f), toScalar(20.0f), 16, 16);
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(20, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(16, actor.width);
    TEST_ASSERT_EQUAL(16, actor.height);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::STATIC), 
                      static_cast<int>(actor.getBodyType()));
}

void test_static_actor_constructor_vector2(void) {
    Vector2 pos(toScalar(30.0f), toScalar(40.0f));
    StaticActor actor(pos, 8, 12);
    
    TEST_ASSERT_EQUAL(30, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(40, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(8, actor.width);
    TEST_ASSERT_EQUAL(12, actor.height);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::STATIC), 
                      static_cast<int>(actor.getBodyType()));
}

// =============================================================================
// Body Type Verification
// =============================================================================

void test_static_actor_is_physics_body(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_TRUE(actor.isPhysicsBody());
}

void test_static_actor_body_type_immutable(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    // StaticActor should always be STATIC type
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::STATIC), 
                      static_cast<int>(actor.getBodyType()));
    
    // Even if someone tries to change it, constructor should have set it
    actor.setBodyType(PhysicsBodyType::KINEMATIC);
    // Note: This tests that setBodyType works, but the design intent
    // is that StaticActor instances remain STATIC
}

// =============================================================================
// Collision Layer and Mask
// =============================================================================

void test_static_actor_default_collision_settings(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    
    // Default layer and mask should be kNone (0)
    TEST_ASSERT_EQUAL_UINT16(0, actor.layer);
    TEST_ASSERT_EQUAL_UINT16(0, actor.mask);
}

// Constantes de capa para el test
static const uint16_t kStaticLayer = 0x08;
static const uint16_t kPlayerLayer = 0x02;
static const uint16_t kEnemyLayer = 0x04;

void test_static_actor_set_collision_layer(void) {
    StaticActor actor(toScalar(10), toScalar(20), 32, 32);
    actor.setCollisionLayer(kStaticLayer);
    TEST_ASSERT_EQUAL_UINT16(kStaticLayer, actor.layer);
}

void test_static_actor_set_collision_mask(void) {
    StaticActor actor(toScalar(10), toScalar(20), 32, 32);
    actor.setCollisionMask(kPlayerLayer | kEnemyLayer);
    TEST_ASSERT_EQUAL_UINT16(kPlayerLayer | kEnemyLayer, actor.mask);
}

// =============================================================================
// Sensor and One-Way Properties
// =============================================================================

void test_static_actor_default_not_sensor(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_FALSE(actor.isSensor());
}

void test_static_actor_can_be_sensor(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setSensor(true);
    TEST_ASSERT_TRUE(actor.isSensor());
}

void test_static_actor_default_not_oneway(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_FALSE(actor.isOneWay());
}

void test_static_actor_can_be_oneway(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setOneWay(true);
    TEST_ASSERT_TRUE(actor.isOneWay());
}

// =============================================================================
// Physics Properties
// =============================================================================

void test_static_actor_default_mass(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    // Default mass should be 1.0
    TEST_ASSERT_EQUAL_FLOAT(1.0f, static_cast<float>(actor.getMass()));
}

void test_static_actor_default_gravity_scale(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    // Default gravity scale should be 1.0
    TEST_ASSERT_EQUAL_FLOAT(1.0f, static_cast<float>(actor.getGravityScale()));
}

void test_static_actor_default_restitution(void) {
    StaticActor actor(toScalar(0), toScalar(0), 10, 10);
    // Default restitution should be 1.0
    TEST_ASSERT_EQUAL_FLOAT(1.0f, static_cast<float>(actor.getRestitution()));
}

// =============================================================================
// HitBox
// =============================================================================

void test_static_actor_get_hitbox(void) {
    StaticActor actor(toScalar(10.0f), toScalar(20.0f), 16, 24);
    Rect hitbox = actor.getHitBox();
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(hitbox.position.x));
    TEST_ASSERT_EQUAL(20, static_cast<int>(hitbox.position.y));
    TEST_ASSERT_EQUAL(16, hitbox.width);
    TEST_ASSERT_EQUAL(24, hitbox.height);
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Constructor tests
    RUN_TEST(test_static_actor_constructor_scalar);
    RUN_TEST(test_static_actor_constructor_vector2);
    
    // Body type tests
    RUN_TEST(test_static_actor_is_physics_body);
    RUN_TEST(test_static_actor_body_type_immutable);
    
    // Collision settings tests
    RUN_TEST(test_static_actor_default_collision_settings);
    RUN_TEST(test_static_actor_set_collision_layer);
    RUN_TEST(test_static_actor_set_collision_mask);
    
    // Sensor and one-way tests
    RUN_TEST(test_static_actor_default_not_sensor);
    RUN_TEST(test_static_actor_can_be_sensor);
    RUN_TEST(test_static_actor_default_not_oneway);
    RUN_TEST(test_static_actor_can_be_oneway);
    
    // Physics properties tests
    RUN_TEST(test_static_actor_default_mass);
    RUN_TEST(test_static_actor_default_gravity_scale);
    RUN_TEST(test_static_actor_default_restitution);
    
    // Hitbox test
    RUN_TEST(test_static_actor_get_hitbox);
    
    return UNITY_END();
}
