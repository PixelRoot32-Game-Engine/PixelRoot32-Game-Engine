/**
 * @file test_physics_actor.cpp
 * @brief Unit tests for PhysicsActor base class.
 * 
 * Tests constructors, physics properties, world bounds resolution,
 * body type behavior, and all getters/setters.
 */

#include <unity.h>
#include "core/PhysicsActor.h"
#include "graphics/Renderer.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "../../test_config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::math;

// Helper functions for coordinate encoding tests
inline uintptr_t packCoord(uint16_t x, uint16_t y) {
    return (static_cast<uintptr_t>(y) << 16) | x;
}

inline void unpackCoord(uintptr_t packed, uint16_t& x, uint16_t& y) {
    x = static_cast<uint16_t>(packed & 0xFFFF);
    y = static_cast<uint16_t>(packed >> 16);
}

// PhysicsActor is abstract (Entity::draw() is pure virtual).
// Concrete subclass for testing.
class TestPhysicsActor : public PhysicsActor {
public:
    using PhysicsActor::PhysicsActor;
    void draw(pixelroot32::graphics::Renderer& r) override { (void)r; }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Constructors
// =============================================================================


void test_physics_actor_constructor_scalar(void) {
    TestPhysicsActor actor(toScalar(10.0f), toScalar(20.0f), 16, 16);
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(20, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(16, actor.width);
    TEST_ASSERT_EQUAL(16, actor.height);
    TEST_ASSERT_TRUE(actor.isPhysicsBody());
}

void test_physics_actor_constructor_vector2(void) {
    Vector2 pos(toScalar(30.0f), toScalar(40.0f));
    TestPhysicsActor actor(pos, 8, 12);
    
    TEST_ASSERT_EQUAL(30, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(40, static_cast<int>(actor.position.y));
    TEST_ASSERT_EQUAL(8, actor.width);
    TEST_ASSERT_EQUAL(12, actor.height);
}

void test_physics_actor_default_body_type(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::KINEMATIC), 
                      static_cast<int>(actor.getBodyType()));
}

void test_physics_actor_default_velocity(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.getVelocityX()));
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.getVelocityY()));
}

// =============================================================================
// Setters & Getters
// =============================================================================

void test_physics_actor_set_velocity_float(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setVelocity(5.0f, -3.0f);
    
    TEST_ASSERT_EQUAL(5, static_cast<int>(actor.getVelocityX()));
    TEST_ASSERT_EQUAL(-3, static_cast<int>(actor.getVelocityY()));
}

void test_physics_actor_set_velocity_scalar(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setVelocity(toScalar(10.0f), toScalar(20.0f));
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(actor.getVelocityX()));
    TEST_ASSERT_EQUAL(20, static_cast<int>(actor.getVelocityY()));
}

void test_physics_actor_set_velocity_vector2(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    Vector2 v(toScalar(7.0f), toScalar(-7.0f));
    actor.setVelocity(v);
    
    TEST_ASSERT_EQUAL(7, static_cast<int>(actor.getVelocity().x));
    TEST_ASSERT_EQUAL(-7, static_cast<int>(actor.getVelocity().y));
}

void test_physics_actor_set_body_type(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    
    actor.setBodyType(PhysicsBodyType::STATIC);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::STATIC),
                      static_cast<int>(actor.getBodyType()));
    
    actor.setBodyType(PhysicsBodyType::RIGID);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::RIGID),
                      static_cast<int>(actor.getBodyType()));
    
    actor.setBodyType(PhysicsBodyType::KINEMATIC);
    TEST_ASSERT_EQUAL(static_cast<int>(PhysicsBodyType::KINEMATIC),
                      static_cast<int>(actor.getBodyType()));
}

void test_physics_actor_set_mass(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setMass(5.0f);
    TEST_ASSERT_EQUAL(5, static_cast<int>(actor.getMass()));
}

void test_physics_actor_set_gravity_scale(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setGravityScale(toScalar(2.0f));
    TEST_ASSERT_EQUAL(2, static_cast<int>(actor.getGravityScale()));
}

void test_physics_actor_set_restitution(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setRestitution(toScalar(0.5f));
    // 0.5 as int is 0, so check it's not 1 (default)
    Scalar r = actor.getRestitution();
    TEST_ASSERT_TRUE(r < toScalar(1.0f));
    TEST_ASSERT_TRUE(r > toScalar(0.0f));
}

void test_physics_actor_set_friction(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setFriction(toScalar(0.3f));
    // Just make sure it doesn't crash and setter runs
}

void test_physics_actor_set_shape(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_EQUAL(static_cast<int>(CollisionShape::AABB),
                      static_cast<int>(actor.getShape()));
    
    actor.setShape(CollisionShape::CIRCLE);
    TEST_ASSERT_EQUAL(static_cast<int>(CollisionShape::CIRCLE),
                      static_cast<int>(actor.getShape()));
}

void test_physics_actor_set_radius(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setRadius(toScalar(8.0f));
    
    TEST_ASSERT_EQUAL(8, static_cast<int>(actor.getRadius()));
    // setRadius should update width/height to diameter
    TEST_ASSERT_EQUAL(16, actor.width);
    TEST_ASSERT_EQUAL(16, actor.height);
}

void test_physics_actor_bounce_default(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_TRUE(actor.bounce);
}

// =============================================================================
// getHitBox
// =============================================================================

void test_physics_actor_get_hitbox(void) {
    TestPhysicsActor actor(toScalar(10.0f), toScalar(20.0f), 30, 40);
    Rect hb = actor.getHitBox();
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(hb.position.x));
    TEST_ASSERT_EQUAL(20, static_cast<int>(hb.position.y));
    TEST_ASSERT_EQUAL(30, hb.width);
    TEST_ASSERT_EQUAL(40, hb.height);
}

// =============================================================================
// update() — body type branching
// =============================================================================

void test_physics_actor_update_static_noop(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setBodyType(PhysicsBodyType::STATIC);
    actor.setVelocity(100.0f, 100.0f);
    
    // Static bodies should return early from update
    actor.update(16);
    
    // Position unchanged (no integration for static)
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.position.y));
}

void test_physics_actor_update_kinematic(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setBodyType(PhysicsBodyType::KINEMATIC);
    
    // Kinematic update doesn't return early, but base integrate is no-op
    actor.update(16);
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.position.x));
}

void test_physics_actor_integrate_base_noop(void) {
    TestPhysicsActor actor(toScalar(50.0f), toScalar(50.0f), 10, 10);
    actor.setVelocity(10.0f, 10.0f);
    
    // Base class integrate is a no-op
    actor.integrate(toScalar(0.016f));
    
    // Position unchanged
    TEST_ASSERT_EQUAL(50, static_cast<int>(actor.position.x));
    TEST_ASSERT_EQUAL(50, static_cast<int>(actor.position.y));
}

// =============================================================================
// setLimits & setWorldBounds
// =============================================================================

void test_physics_actor_set_limits_ints(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setLimits(10, 20, 200, 300);
    
    // Move beyond the right limit and resolve
    actor.position.x = toScalar(250.0f);
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.right);
    TEST_ASSERT_EQUAL(190, static_cast<int>(actor.position.x)); // 200 - 10 (width)
}

void test_physics_actor_set_limits_struct(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    LimitRect lr(5, 5, 100, 100);
    actor.setLimits(lr);
    
    actor.position.x = toScalar(-10.0f);
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.left);
    TEST_ASSERT_EQUAL(5, static_cast<int>(actor.position.x));
}

void test_physics_actor_set_world_bounds(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setWorldBounds(320, 240);
    
    // Move beyond world width
    actor.position.x = toScalar(350.0f);
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.right);
    TEST_ASSERT_EQUAL(310, static_cast<int>(actor.position.x)); // 320 - 10
}

void test_physics_actor_set_world_size_alias(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setWorldSize(160, 120);
    
    actor.position.y = toScalar(130.0f);
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.bottom);
    TEST_ASSERT_EQUAL(110, static_cast<int>(actor.position.y)); // 120 - 10
}

// =============================================================================
// resolveWorldBounds — all 4 boundaries
// =============================================================================

void test_physics_actor_resolve_left_boundary(void) {
    TestPhysicsActor actor(toScalar(-5.0f), toScalar(50.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.setVelocity(-10.0f, 0.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.left);
    TEST_ASSERT_FALSE(info.right);
    TEST_ASSERT_FALSE(info.top);
    TEST_ASSERT_FALSE(info.bottom);
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.position.x));
}

void test_physics_actor_resolve_right_boundary(void) {
    TestPhysicsActor actor(toScalar(195.0f), toScalar(50.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.setVelocity(10.0f, 0.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.right);
    TEST_ASSERT_EQUAL(190, static_cast<int>(actor.position.x)); // 200 - 10
}

void test_physics_actor_resolve_top_boundary(void) {
    TestPhysicsActor actor(toScalar(50.0f), toScalar(-5.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.setVelocity(0.0f, -10.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.top);
    TEST_ASSERT_FALSE(info.bottom);
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.position.y));
}

void test_physics_actor_resolve_bottom_boundary(void) {
    TestPhysicsActor actor(toScalar(50.0f), toScalar(195.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.setVelocity(0.0f, 10.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.bottom);
    TEST_ASSERT_EQUAL(190, static_cast<int>(actor.position.y)); // 200 - 10
}

void test_physics_actor_resolve_corner_collision(void) {
    // Actor in the top-left corner → both left and top
    TestPhysicsActor actor(toScalar(-5.0f), toScalar(-5.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.setVelocity(-10.0f, -10.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.left);
    TEST_ASSERT_TRUE(info.top);
    TEST_ASSERT_FALSE(info.right);
    TEST_ASSERT_FALSE(info.bottom);
}

// =============================================================================
// resolveWorldBounds — bounce vs no-bounce
// =============================================================================

void test_physics_actor_bounce_reverses_velocity(void) {
    TestPhysicsActor actor(toScalar(-5.0f), toScalar(50.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.bounce = true;
    actor.setRestitution(toScalar(1.0f)); // Perfect bounce
    actor.setVelocity(-10.0f, 0.0f);
    
    actor.resolveWorldBounds();
    
    // Velocity should be reversed (bounce)
    TEST_ASSERT_TRUE(static_cast<int>(actor.getVelocityX()) > 0);
}

void test_physics_actor_no_bounce_zeroes_velocity(void) {
    TestPhysicsActor actor(toScalar(-5.0f), toScalar(50.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.bounce = false;
    actor.setVelocity(-10.0f, 0.0f);
    
    actor.resolveWorldBounds();
    
    // With bounce = false, effectiveRestitution = 0 → velocity = 0
    TEST_ASSERT_EQUAL(0, static_cast<int>(actor.getVelocityX()));
}

void test_physics_actor_bounce_with_partial_restitution(void) {
    TestPhysicsActor actor(toScalar(-5.0f), toScalar(50.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.bounce = true;
    actor.setRestitution(toScalar(0.5f));
    actor.setVelocity(toScalar(-10.0f), toScalar(0.0f));
    
    actor.resolveWorldBounds();
    
    // Velocity should be reversed but reduced by 0.5
    Scalar vx = actor.getVelocityX();
    TEST_ASSERT_TRUE(vx > toScalar(0.0f));
    TEST_ASSERT_TRUE(vx < toScalar(10.0f));
}

// =============================================================================
// resolveWorldBounds — custom limits
// =============================================================================

void test_physics_actor_custom_limits_left(void) {
    TestPhysicsActor actor(toScalar(5.0f), toScalar(50.0f), 10, 10);
    actor.setLimits(20, 0, 200, 200);
    actor.setVelocity(-10.0f, 0.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.left);
    TEST_ASSERT_EQUAL(20, static_cast<int>(actor.position.x));
}

void test_physics_actor_custom_limits_bottom(void) {
    TestPhysicsActor actor(toScalar(50.0f), toScalar(180.0f), 10, 10);
    actor.setLimits(0, 0, 200, 150);
    actor.setVelocity(0.0f, 10.0f);
    
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info.bottom);
    TEST_ASSERT_EQUAL(140, static_cast<int>(actor.position.y)); // 150 - 10
}

// =============================================================================
// resetWorldCollisionInfo
// =============================================================================

void test_physics_actor_reset_world_collision_info(void) {
    TestPhysicsActor actor(toScalar(-5.0f), toScalar(-5.0f), 10, 10);
    actor.setWorldBounds(200, 200);
    actor.resolveWorldBounds();
    
    WorldCollisionInfo info1 = actor.getWorldCollisionInfo();
    TEST_ASSERT_TRUE(info1.left);
    TEST_ASSERT_TRUE(info1.top);
    
    actor.resetWorldCollisionInfo();
    WorldCollisionInfo info2 = actor.getWorldCollisionInfo();
    TEST_ASSERT_FALSE(info2.left);
    TEST_ASSERT_FALSE(info2.right);
    TEST_ASSERT_FALSE(info2.top);
    TEST_ASSERT_FALSE(info2.bottom);
}

// =============================================================================
// onCollision — virtual callback (no-op base)
// =============================================================================

void test_physics_actor_on_collision_noop(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    // Should not crash — base implementation is empty
    actor.onCollision(nullptr);
}

// =============================================================================
// WorldCollisionInfo struct
// =============================================================================

void test_world_collision_info_default(void) {
    WorldCollisionInfo info;
    TEST_ASSERT_FALSE(info.left);
    TEST_ASSERT_FALSE(info.right);
    TEST_ASSERT_FALSE(info.top);
    TEST_ASSERT_FALSE(info.bottom);
}

void test_world_collision_info_parameterized(void) {
    WorldCollisionInfo info(true, false, true, false);
    TEST_ASSERT_TRUE(info.left);
    TEST_ASSERT_FALSE(info.right);
    TEST_ASSERT_TRUE(info.top);
    TEST_ASSERT_FALSE(info.bottom);
}

// =============================================================================
// LimitRect struct
// =============================================================================

void test_limit_rect_default(void) {
    LimitRect lr;
    TEST_ASSERT_EQUAL(-1, lr.left);
    TEST_ASSERT_EQUAL(-1, lr.top);
    TEST_ASSERT_EQUAL(-1, lr.right);
    TEST_ASSERT_EQUAL(-1, lr.bottom);
}

void test_limit_rect_parameterized(void) {
    LimitRect lr(10, 20, 200, 300);
    TEST_ASSERT_EQUAL(10, lr.left);
    TEST_ASSERT_EQUAL(20, lr.top);
    TEST_ASSERT_EQUAL(200, lr.right);
    TEST_ASSERT_EQUAL(300, lr.bottom);
    TEST_ASSERT_EQUAL(190, lr.width());
    TEST_ASSERT_EQUAL(280, lr.height());
}

// =============================================================================
// userData Tests
// =============================================================================

void test_user_data_default_null(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    TEST_ASSERT_NULL(actor.getUserData());
}

void test_user_data_set_get_pointer(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    int data = 42;
    actor.setUserData(&data);
    TEST_ASSERT_EQUAL(&data, actor.getUserData());
    TEST_ASSERT_EQUAL(42, *(static_cast<int*>(actor.getUserData())));
}

void test_user_data_set_get_null(void) {
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    int data = 123;
    actor.setUserData(&data);
    TEST_ASSERT_EQUAL(&data, actor.getUserData());
    
    // Set back to null
    actor.setUserData(nullptr);
    TEST_ASSERT_NULL(actor.getUserData());
}

void test_user_data_coord_encoding(void) {
    // Test encoding/decoding of coordinates
    uint16_t x = 1234, y = 5678;
    uintptr_t packed = packCoord(x, y);
    uint16_t x2, y2;
    unpackCoord(packed, x2, y2);
    TEST_ASSERT_EQUAL(x, x2);
    TEST_ASSERT_EQUAL(y, y2);
}

void test_user_data_coord_encoding_limits(void) {
    // Test maximum coordinate values (65535x65535)
    uint16_t maxX = 65535, maxY = 65535;
    uintptr_t packed = packCoord(maxX, maxY);
    uint16_t x, y;
    unpackCoord(packed, x, y);
    TEST_ASSERT_EQUAL(maxX, x);
    TEST_ASSERT_EQUAL(maxY, y);
    
    // Test minimum coordinate values (0x0)
    uint16_t minX = 0, minY = 0;
    packed = packCoord(minX, minY);
    unpackCoord(packed, x, y);
    TEST_ASSERT_EQUAL(minX, x);
    TEST_ASSERT_EQUAL(minY, y);
}

void test_user_data_coord_encoding_real_world(void) {
    // Test typical tilemap coordinates
    uint16_t tileX = 10, tileY = 15;
    uintptr_t packed = packCoord(tileX, tileY);
    
    // Store in userData and retrieve
    TestPhysicsActor actor(toScalar(0), toScalar(0), 10, 10);
    actor.setUserData(reinterpret_cast<void*>(packed));
    
    uintptr_t retrieved = reinterpret_cast<uintptr_t>(actor.getUserData());
    uint16_t x, y;
    unpackCoord(retrieved, x, y);
    
    TEST_ASSERT_EQUAL(tileX, x);
    TEST_ASSERT_EQUAL(tileY, y);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // Constructors
    RUN_TEST(test_physics_actor_constructor_scalar);
    RUN_TEST(test_physics_actor_constructor_vector2);
    RUN_TEST(test_physics_actor_default_body_type);
    RUN_TEST(test_physics_actor_default_velocity);
    
    // Setters & Getters
    RUN_TEST(test_physics_actor_set_velocity_float);
    RUN_TEST(test_physics_actor_set_velocity_scalar);
    RUN_TEST(test_physics_actor_set_velocity_vector2);
    RUN_TEST(test_physics_actor_set_body_type);
    RUN_TEST(test_physics_actor_set_mass);
    RUN_TEST(test_physics_actor_set_gravity_scale);
    RUN_TEST(test_physics_actor_set_restitution);
    RUN_TEST(test_physics_actor_set_friction);
    RUN_TEST(test_physics_actor_set_shape);
    RUN_TEST(test_physics_actor_set_radius);
    RUN_TEST(test_physics_actor_bounce_default);
    
    // getHitBox
    RUN_TEST(test_physics_actor_get_hitbox);
    
    // update paths
    RUN_TEST(test_physics_actor_update_static_noop);
    RUN_TEST(test_physics_actor_update_kinematic);
    RUN_TEST(test_physics_actor_integrate_base_noop);
    
    // setLimits & setWorldBounds
    RUN_TEST(test_physics_actor_set_limits_ints);
    RUN_TEST(test_physics_actor_set_limits_struct);
    RUN_TEST(test_physics_actor_set_world_bounds);
    RUN_TEST(test_physics_actor_set_world_size_alias);
    
    // resolveWorldBounds — all 4 boundaries
    RUN_TEST(test_physics_actor_resolve_left_boundary);
    RUN_TEST(test_physics_actor_resolve_right_boundary);
    RUN_TEST(test_physics_actor_resolve_top_boundary);
    RUN_TEST(test_physics_actor_resolve_bottom_boundary);
    RUN_TEST(test_physics_actor_resolve_corner_collision);
    
    // bounce vs no-bounce
    RUN_TEST(test_physics_actor_bounce_reverses_velocity);
    RUN_TEST(test_physics_actor_no_bounce_zeroes_velocity);
    RUN_TEST(test_physics_actor_bounce_with_partial_restitution);
    
    // custom limits
    RUN_TEST(test_physics_actor_custom_limits_left);
    RUN_TEST(test_physics_actor_custom_limits_bottom);
    
    // resetWorldCollisionInfo
    RUN_TEST(test_physics_actor_reset_world_collision_info);
    
    // onCollision
    RUN_TEST(test_physics_actor_on_collision_noop);
    
    // Structs
    RUN_TEST(test_world_collision_info_default);
    RUN_TEST(test_world_collision_info_parameterized);
    RUN_TEST(test_limit_rect_default);
    RUN_TEST(test_limit_rect_parameterized);
    
    // userData
    RUN_TEST(test_user_data_default_null);
    RUN_TEST(test_user_data_set_get_pointer);
    RUN_TEST(test_user_data_set_get_null);
    RUN_TEST(test_user_data_coord_encoding);
    RUN_TEST(test_user_data_coord_encoding_limits);
    RUN_TEST(test_user_data_coord_encoding_real_world);
    
    return UNITY_END();
}
