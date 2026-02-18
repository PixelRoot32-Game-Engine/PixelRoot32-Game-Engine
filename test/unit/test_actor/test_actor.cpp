/**
 * @file test_actor.cpp
 * @brief Unit tests for core/Actor module using real engine headers.
 */

#include <unity.h>
#include "../../test_config.h"
#include "core/Actor.h"
#include "core/Engine.h"
#include "graphics/Renderer.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::physics;

// Concrete implementation of Actor for testing
class TestActor : public Actor {
public:
    bool collisionCallbackCalled = false;
    Actor* collidedActor = nullptr;

    TestActor(float x, float y, int w, int h) : Actor(x, y, w, h) {}

    Rect getHitBox() override {
        return {position, width, height};
    }

    void onCollision(Actor* other) override {
        collisionCallbackCalled = true;
        collidedActor = other;
    }

    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }

    void reset() {
        collisionCallbackCalled = false;
        collidedActor = nullptr;
    }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for Actor initialization
// =============================================================================

void test_actor_initialization(void) {
    TestActor a(10.0f, 20.0f, 30, 40);
    
    TEST_ASSERT_EQUAL_FLOAT(10.0f, a.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, a.position.y);
    TEST_ASSERT_EQUAL_INT(30, a.width);
    TEST_ASSERT_EQUAL_INT(40, a.height);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(EntityType::ACTOR), static_cast<int>(a.type));
}

void test_actor_default_layer(void) {
    TestActor a(0, 0, 10, 10);
    TEST_ASSERT_EQUAL_UINT16(DefaultLayers::kNone, a.layer);
}

void test_actor_default_mask(void) {
    TestActor a(0, 0, 10, 10);
    TEST_ASSERT_EQUAL_UINT16(DefaultLayers::kNone, a.mask);
}

// =============================================================================
// Tests for collision layers
// =============================================================================

void test_actor_set_layer(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(1);
    TEST_ASSERT_EQUAL_UINT16(1, a.layer);
}

void test_actor_set_mask(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionMask(2);
    TEST_ASSERT_EQUAL_UINT16(2, a.mask);
}

void test_actor_layer_and_mask_independent(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(1);
    a.setCollisionMask(2);
    TEST_ASSERT_EQUAL_UINT16(1, a.layer);
    TEST_ASSERT_EQUAL_UINT16(2, a.mask);
}

void test_actor_multiple_layers(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(1 | 2 | 4);  // Layers 1, 2, and 4
    TEST_ASSERT_EQUAL_UINT16(7, a.layer);
}

void test_actor_all_layers(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(DefaultLayers::kAll);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, a.layer);
}

// =============================================================================
// Tests for isInLayer
// =============================================================================

void test_actor_is_in_layer_true(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(1 | 2);
    TEST_ASSERT_TRUE(a.isInLayer(1));
    TEST_ASSERT_TRUE(a.isInLayer(2));
}

void test_actor_is_in_layer_false(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(1);
    TEST_ASSERT_FALSE(a.isInLayer(2));
    TEST_ASSERT_FALSE(a.isInLayer(4));
}

void test_actor_is_in_layer_multiple(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(1 | 2 | 4);
    TEST_ASSERT_TRUE(a.isInLayer(1));
    TEST_ASSERT_TRUE(a.isInLayer(2));
    TEST_ASSERT_TRUE(a.isInLayer(4));
    TEST_ASSERT_FALSE(a.isInLayer(8));
}

void test_actor_is_in_layer_zero(void) {
    TestActor a(0, 0, 10, 10);
    a.setCollisionLayer(0);
    TEST_ASSERT_FALSE(a.isInLayer(1));
}

// =============================================================================
// Tests for hitbox
// =============================================================================

void test_actor_hitbox_basic(void) {
    TestActor a(10.0f, 20.0f, 30, 40);
    Rect hitbox = a.getHitBox();
    
    TEST_ASSERT_EQUAL_FLOAT(10.0f, hitbox.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, hitbox.position.y);
    TEST_ASSERT_EQUAL_INT(30, hitbox.width);
    TEST_ASSERT_EQUAL_INT(40, hitbox.height);
}

void test_actor_hitbox_follows_position(void) {
    TestActor a(0, 0, 10, 10);
    a.position.x = 50.0f;
    a.position.y = 60.0f;
    
    Rect hitbox = a.getHitBox();
    TEST_ASSERT_EQUAL_FLOAT(50.0f, hitbox.position.x);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, hitbox.position.y);
}

void test_actor_hitbox_size_matches(void) {
    TestActor a(0, 0, 10, 10);
    a.width = 50;
    a.height = 60;
    
    Rect hitbox = a.getHitBox();
    TEST_ASSERT_EQUAL_INT(50, hitbox.width);
    TEST_ASSERT_EQUAL_INT(60, hitbox.height);
}

// =============================================================================
// Tests for collision callback
// =============================================================================

void test_actor_on_collision_called(void) {
    TestActor a(0, 0, 10, 10);
    TestActor b(5, 5, 10, 10);
    
    a.onCollision(&b);
    TEST_ASSERT_TRUE(a.collisionCallbackCalled);
    TEST_ASSERT_EQUAL(&b, a.collidedActor);
}

void test_actor_collision_callback_reset(void) {
    TestActor a(0, 0, 10, 10);
    TestActor b(5, 5, 10, 10);
    
    a.onCollision(&b);
    a.reset();
    TEST_ASSERT_FALSE(a.collisionCallbackCalled);
    TEST_ASSERT_NULL(a.collidedActor);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    
    RUN_TEST(test_actor_initialization);
    RUN_TEST(test_actor_default_layer);
    RUN_TEST(test_actor_default_mask);
    RUN_TEST(test_actor_set_layer);
    RUN_TEST(test_actor_set_mask);
    RUN_TEST(test_actor_layer_and_mask_independent);
    RUN_TEST(test_actor_multiple_layers);
    RUN_TEST(test_actor_all_layers);
    RUN_TEST(test_actor_is_in_layer_true);
    RUN_TEST(test_actor_is_in_layer_false);
    RUN_TEST(test_actor_is_in_layer_multiple);
    RUN_TEST(test_actor_is_in_layer_zero);
    RUN_TEST(test_actor_hitbox_basic);
    RUN_TEST(test_actor_hitbox_follows_position);
    RUN_TEST(test_actor_hitbox_size_matches);
    RUN_TEST(test_actor_on_collision_called);
    RUN_TEST(test_actor_collision_callback_reset);

    return UNITY_END();
}
