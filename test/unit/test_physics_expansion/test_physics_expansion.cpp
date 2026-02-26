/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include <unity.h>
#include "physics/StaticActor.h"
#include "physics/KinematicActor.h"
#include "physics/RigidActor.h"
#include "physics/CollisionSystem.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"
#include "../../test_config.h"
#include <memory>

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;
using namespace pixelroot32::graphics;

namespace {
class MockDrawSurface : public BaseDrawSurface {
public:
    void init() override {}
    void clearBuffer() override {}
    void sendBuffer() override {}
    void drawRectangle(int, int, int, int, uint16_t) override {}
    void drawFilledRectangle(int, int, int, int, uint16_t) override {}
    void drawPixel(int, int, uint16_t) override {}
};
}

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_static_actor_immobility() {
    StaticActor wall(toScalar(0), toScalar(100), 100, 10);
    
    // Static bodies should skip update and integrate
    wall.setVelocity(toScalar(10), toScalar(10));
    wall.update(16);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, static_cast<float>(wall.position.x));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, static_cast<float>(wall.position.y));
}

void test_static_actor_vector2_ctor_and_draw() {
    StaticActor wall(Vector2(toScalar(10), toScalar(20)), 50, 10);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, static_cast<float>(wall.position.x));
    TEST_ASSERT_EQUAL_FLOAT(20.0f, static_cast<float>(wall.position.y));
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    wall.draw(renderer);
}

void test_rigid_actor_gravity() {
    RigidActor box(toScalar(0), toScalar(0), 10, 10);
    box.setGravityScale(toScalar(1.0f));
    box.setMass(toScalar(1.0f));
    
    // Simulate one frame (16ms)
    box.update(16);
    
    // Should have accumulated downward velocity due to gravity
    TEST_ASSERT_TRUE(box.getVelocityY() > toScalar(0));
}

void test_rigid_actor_vector2_ctor_force_impulse_draw() {
    RigidActor box(Vector2(toScalar(5), toScalar(5)), 8, 8);
    box.setMass(toScalar(1.0f));
    box.applyForce(Vector2(toScalar(10), toScalar(0)));
    box.applyImpulse(Vector2(toScalar(1), toScalar(0)));
    box.update(16);
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    box.draw(renderer);
}

void test_kinematic_move_and_collide() {
    CollisionSystem system;
    
    StaticActor wall(toScalar(0), toScalar(50), toScalar(100), toScalar(10));
    wall.setCollisionLayer(1);
    wall.setCollisionMask(1);
    
    KinematicActor player(toScalar(0), toScalar(0), toScalar(10), toScalar(10));
    player.setCollisionLayer(1);
    player.setCollisionMask(1);
    
    system.addEntity(&wall);
    system.addEntity(&player);
    player.collisionSystem = &system;
    
    // Ensure spatial grid is updated with static actor
    system.update();
    
    // Move player towards wall
    // Note: KinematicActor::moveAndCollide performs discrete collision checks at the target position.
    // It does NOT perform continuous collision detection (CCD) or sweeping.
    // Therefore, we must ensure the movement target actually overlaps with the wall to trigger collision logic.
    // Wall is at y=50, height=10. Player height=10. Collision starts at y=40.
    // Moving to 55 ensures overlap. Moving to 100 causes tunneling (skipping the wall).
    bool collided = player.moveAndCollide(Vector2(toScalar(0), toScalar(55)));
    
    TEST_ASSERT_TRUE(collided);
    // Player should stop before wall (at y=40 approx)
    TEST_ASSERT_TRUE(player.position.y < toScalar(50));
    TEST_ASSERT_TRUE(player.position.y >= toScalar(39)); // Allow some precision error
}

void test_kinematic_actor_vector2_ctor_and_draw() {
    KinematicActor kinematic(Vector2(toScalar(20), toScalar(30)), 16, 16);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, static_cast<float>(kinematic.position.x));
    TEST_ASSERT_EQUAL_FLOAT(30.0f, static_cast<float>(kinematic.position.y));
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    kinematic.draw(renderer);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    RUN_TEST(test_static_actor_immobility);
    RUN_TEST(test_static_actor_vector2_ctor_and_draw);
    RUN_TEST(test_rigid_actor_gravity);
    RUN_TEST(test_rigid_actor_vector2_ctor_force_impulse_draw);
    RUN_TEST(test_kinematic_move_and_collide);
    RUN_TEST(test_kinematic_actor_vector2_ctor_and_draw);
    return UNITY_END();
}
