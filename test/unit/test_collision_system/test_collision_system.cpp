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
#include "core/Actor.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;

// Mock Actor for testing
class MockActor : public Actor {
public:
    bool collisionCalled = false;
    Actor* collidedWith = nullptr;
    Rect hitbox;

    MockActor(float x, float y, int w, int h) : Actor(x, y, w, h) {
        hitbox = {x, y, w, h};
    }

    Rect getHitBox() override {
        hitbox.x = x;
        hitbox.y = y;
        hitbox.width = width;
        hitbox.height = height;
        return hitbox;
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
    GenericEntity(float x, float y, int w, int h) : Entity(x, y, w, h, EntityType::GENERIC) {}
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
    
    return UNITY_END();
}
