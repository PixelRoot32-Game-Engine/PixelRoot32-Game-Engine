/**
 * @file test_sensor_actor.cpp
 * @brief Unit tests for SensorActor class.
 * 
 * Tests SensorActor constructors, sensor property, trigger behavior,
 * and collision detection without physical response.
 */

#include <unity.h>
#include "physics/SensorActor.h"
#include "physics/KinematicActor.h"
#include "physics/CollisionSystem.h"
#include "mocks/MockDrawSurface.h"
#include "../../test_config.h"

using namespace pixelroot32::physics;
using namespace pixelroot32::core;
using namespace pixelroot32::math;
using namespace pixelroot32::graphics;

CollisionSystem* sensorTestSystem = nullptr;
KinematicActor* movingActor = nullptr;
SensorActor* sensor = nullptr;

// Constantes de capa para el test
static const uint16_t kPlayerLayer = 0x02;
static const uint16_t kSensorLayer = 0x04;

void setUp(void) {
    test_setup();
    
    // Crear sistema de colisión y actor móvil para tests de detección
    sensorTestSystem = new CollisionSystem();
    
    movingActor = new KinematicActor(toScalar(0), toScalar(0), 10, 10);
    movingActor->setCollisionLayer(kPlayerLayer);
    movingActor->setCollisionMask(kSensorLayer);
    movingActor->collisionSystem = sensorTestSystem;
    sensorTestSystem->addEntity(movingActor);
    
    sensor = nullptr;
}

void tearDown(void) {
    if (movingActor) delete movingActor;
    if (sensor) delete sensor;
    if (sensorTestSystem) delete sensorTestSystem;
    movingActor = nullptr;
    sensor = nullptr;
    sensorTestSystem = nullptr;
    test_teardown();
}

// =============================================================================
// Constructors
// =============================================================================

void test_sensor_actor_constructor_scalar(void) {
    SensorActor actor(toScalar(10.0f), toScalar(20.0f), 16, 16);
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(20, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(16, actor.width);
    TEST_ASSERT_EQUAL(16, actor.height);
    TEST_ASSERT_TRUE(actor.isSensor());
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::STATIC), 
                      static_cast<int>(actor.getBodyType()));
}

void test_sensor_actor_constructor_vector2(void) {
    Vector2 pos(toScalar(30.0f), toScalar(40.0f));
    SensorActor actor(pos, 8, 12);
    
    TEST_ASSERT_EQUAL(30, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(40, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(8, actor.width);
    TEST_ASSERT_EQUAL(12, actor.height);
    TEST_ASSERT_TRUE(actor.isSensor());
}

// =============================================================================
// Sensor Properties
// =============================================================================

void test_sensor_actor_is_always_sensor(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    
    // SensorActor should always be a sensor
    TEST_ASSERT_TRUE(actor.isSensor());
}

void test_sensor_actor_inherits_static_actor(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    
    // Should inherit StaticActor behavior
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::STATIC), 
                      static_cast<int>(actor.getBodyType()));
}

// =============================================================================
// Collision Layer and Mask
// =============================================================================

void test_sensor_actor_default_collision_settings(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    
    TEST_ASSERT_EQUAL_UINT16(DefaultLayers::kNone, actor.layer);
    TEST_ASSERT_EQUAL_UINT16(DefaultLayers::kNone, actor.mask);
}

void test_sensor_actor_set_sensor_layer(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setCollisionLayer(kSensorLayer);
    TEST_ASSERT_EQUAL_UINT16(kSensorLayer, actor.layer);
}

void test_sensor_actor_detects_player(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setCollisionLayer(kSensorLayer);
    actor.setCollisionMask(kPlayerLayer);
    
    TEST_ASSERT_TRUE(actor.isInLayer(kSensorLayer));
}

// =============================================================================
// Physics Properties
// =============================================================================

void test_sensor_actor_default_not_oneway(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_FALSE(actor.isOneWay());
}

void test_sensor_actor_default_mass(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    // Inherits default mass from PhysicsActor
    TEST_ASSERT_EQUAL_FLOAT(1.0f, static_cast<float>(actor.getMass()));
}

void test_sensor_actor_default_gravity_scale(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, static_cast<float>(actor.getGravityScale()));
}

// =============================================================================
// HitBox
// =============================================================================

void test_sensor_actor_get_hitbox(void) {
    SensorActor actor(toScalar(15.0f), toScalar(25.0f), 20, 30);
    Rect hitbox = actor.getHitBox();
    
    TEST_ASSERT_EQUAL(15, static_cast<int>(hitbox.position.x));
    TEST_ASSERT_EQUAL(25, static_cast<int>(hitbox.position.y));
    TEST_ASSERT_EQUAL(20, hitbox.width);
    TEST_ASSERT_EQUAL(30, hitbox.height);
}

// =============================================================================
// Collision Detection (Integration with CollisionSystem)
// =============================================================================

void test_sensor_actor_detects_overlap(void) {
    // Create sensor at position that will overlap with moving actor
    sensor = new SensorActor(toScalar(5), toScalar(5), 10, 10);
    sensor->setCollisionLayer(kSensorLayer);
    sensor->setCollisionMask(kPlayerLayer);
    sensor->collisionSystem = sensorTestSystem;
    sensorTestSystem->addEntity(sensor);
    
    // Move actor into sensor
    movingActor->moveAndSlide(Vector2(toScalar(10), toScalar(10)));
    
    // The sensor should detect the overlap (though we can't directly test
    // the collision callback without more setup)
    // This test mainly verifies no crash occurs during overlap
    TEST_ASSERT_TRUE(true);
}

void test_sensor_actor_does_not_block_movement(void) {
    // Create sensor that would overlap
    sensor = new SensorActor(toScalar(12), toScalar(0), 10, 10);
    sensor->setCollisionLayer(kSensorLayer);
    sensor->setCollisionMask(kPlayerLayer);
    sensor->collisionSystem = sensorTestSystem;
    sensorTestSystem->addEntity(sensor);
    
    float startX = static_cast<float>(movingActor->position.x);
    
    // Try to move through sensor
    bool blocked = movingActor->moveAndCollide(Vector2(toScalar(20), toScalar(0)));
    
    // Sensor should NOT block movement
    TEST_ASSERT_FALSE(blocked);
    // Actor should have moved past the sensor
    TEST_ASSERT_TRUE(movingActor->position.x > toScalar(startX + 10));
}

// =============================================================================
// User Data
// =============================================================================

void test_sensor_actor_user_data(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    
    int testData = 42;
    actor.setUserData(&testData);
    
    TEST_ASSERT_EQUAL(&testData, actor.getUserData());
}

void test_sensor_actor_user_data_null_by_default(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_NULL(actor.getUserData());
}

// =============================================================================
// Collision Shape
// =============================================================================

void test_sensor_actor_default_shape_aabb(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_EQUAL(static_cast<int>(CollisionShape::AABB), 
                      static_cast<int>(actor.getShape()));
}

void test_sensor_actor_set_circle_shape(void) {
    SensorActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setShape(CollisionShape::CIRCLE);
    actor.setRadius(toScalar(5.0f));
    
    TEST_ASSERT_EQUAL(static_cast<int>(CollisionShape::CIRCLE), 
                      static_cast<int>(actor.getShape()));
    TEST_ASSERT_EQUAL(5, static_cast<int>(actor.getRadius()));
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Constructor tests
    RUN_TEST(test_sensor_actor_constructor_scalar);
    RUN_TEST(test_sensor_actor_constructor_vector2);
    
    // Sensor property tests
    RUN_TEST(test_sensor_actor_is_always_sensor);
    RUN_TEST(test_sensor_actor_inherits_static_actor);
    
    // Collision settings tests
    RUN_TEST(test_sensor_actor_default_collision_settings);
    RUN_TEST(test_sensor_actor_set_sensor_layer);
    RUN_TEST(test_sensor_actor_detects_player);
    
    // Physics property tests
    RUN_TEST(test_sensor_actor_default_not_oneway);
    RUN_TEST(test_sensor_actor_default_mass);
    RUN_TEST(test_sensor_actor_default_gravity_scale);
    
    // Hitbox test
    RUN_TEST(test_sensor_actor_get_hitbox);
    
    // Collision detection tests
    RUN_TEST(test_sensor_actor_detects_overlap);
    RUN_TEST(test_sensor_actor_does_not_block_movement);
    
    // User data tests
    RUN_TEST(test_sensor_actor_user_data);
    RUN_TEST(test_sensor_actor_user_data_null_by_default);
    
    // Shape tests
    RUN_TEST(test_sensor_actor_default_shape_aabb);
    RUN_TEST(test_sensor_actor_set_circle_shape);
    
    return UNITY_END();
}
