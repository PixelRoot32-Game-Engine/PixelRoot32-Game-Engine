/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include <unity.h>
#include "physics/StaticActor.h"
#include "physics/KinematicActor.h"
#include "physics/RigidActor.h"
#include "physics/CollisionSystem.h"
#include "../../test_config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_static_actor_immobility() {
    StaticActor wall(toScalar(0), toScalar(100), toScalar(100), toScalar(10));
    
    // Static bodies should skip update and integrate
    wall.setVelocity(toScalar(10), toScalar(10));
    wall.update(16);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, static_cast<float>(wall.position.x));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, static_cast<float>(wall.position.y));
}

void test_rigid_actor_gravity() {
    RigidActor box(toScalar(0), toScalar(0), toScalar(10), toScalar(10));
    box.setGravityScale(1.0f);
    box.setMass(1.0f);
    
    // Simulate one frame (16ms)
    box.update(16);
    
    // Should have moved down
    TEST_ASSERT_TRUE(box.position.y > toScalar(0));
    TEST_ASSERT_TRUE(box.getVelocityY() > toScalar(0));
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
    
    // Move player towards wall
    bool collided = player.moveAndCollide(Vector2(toScalar(0), toScalar(100)));
    
    TEST_ASSERT_TRUE(collided);
    // Player should stop before wall
    TEST_ASSERT_TRUE(player.position.y < toScalar(50));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    RUN_TEST(test_static_actor_immobility);
    RUN_TEST(test_rigid_actor_gravity);
    RUN_TEST(test_kinematic_move_and_collide);
    return UNITY_END();
}
