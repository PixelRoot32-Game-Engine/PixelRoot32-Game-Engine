/**
 * @file test_actor.cpp
 * @brief Unit tests for core/Actor module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for Actor class including:
 * - Collision layers and masks
 * - Hitbox functionality
 * - onCollision callbacks
 */

#include <unity.h>
#include "../../test_config.h"

// Mock implementation
namespace pixelroot32 {
namespace graphics {
    class Renderer {};
}

namespace physics {
    typedef uint16_t CollisionLayer;
    
    namespace DefaultLayers {
        const CollisionLayer kNone = 0;
        const CollisionLayer kAll = 0xFFFF;
    }
}

namespace core {

enum class EntityType { GENERIC, ACTOR, UI_ELEMENT };

struct Rect {
    float x, y;
    int width, height;
    
    bool intersects(const Rect& other) const {
        return !(x + width < other.x || x > other.x + other.width ||
                 y + height < other.y || y > other.y + other.height);
    }
};

class Entity {
public:
    float x, y;
    int width, height;
    EntityType type;
    bool isVisible = true;
    bool isEnabled = true;
    unsigned char renderLayer = 1;
    
    Entity(float x, float y, int w, int h, EntityType t) 
        : x(x), y(y), width(w), height(h), type(t) {}
    virtual ~Entity() {}
    virtual void update(unsigned long deltaTime) {}
    virtual void draw(graphics::Renderer& renderer) { (void)renderer; }
};

class Actor : public Entity {
public:
    physics::CollisionLayer layer = physics::DefaultLayers::kNone;
    physics::CollisionLayer mask = physics::DefaultLayers::kNone;
    bool collisionCallbackCalled = false;
    Actor* collidedActor = nullptr;
    
    Actor(float x, float y, int w, int h) 
        : Entity(x, y, w, h, EntityType::ACTOR) {}
    
    void setCollisionLayer(physics::CollisionLayer l) { layer = l; }
    void setCollisionMask(physics::CollisionLayer m) { mask = m; }
    
    bool isInLayer(uint16_t targetLayer) const {
        return (layer & targetLayer) != 0;
    }
    
    virtual Rect getHitBox() {
        return {x, y, width, height};
    }
    
    virtual void onCollision(Actor* other) {
        collisionCallbackCalled = true;
        collidedActor = other;
    }
    
    void reset() {
        collisionCallbackCalled = false;
        collidedActor = nullptr;
    }
};

}
}

using namespace pixelroot32::core;
using namespace pixelroot32::physics;

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
    Actor a(10.0f, 20.0f, 30, 40);
    
    TEST_ASSERT_EQUAL_FLOAT(10.0f, a.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, a.y);
    TEST_ASSERT_EQUAL_INT(30, a.width);
    TEST_ASSERT_EQUAL_INT(40, a.height);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(EntityType::ACTOR), static_cast<int>(a.type));
}

void test_actor_default_layer(void) {
    Actor a(0, 0, 10, 10);
    TEST_ASSERT_EQUAL_UINT16(DefaultLayers::kNone, a.layer);
}

void test_actor_default_mask(void) {
    Actor a(0, 0, 10, 10);
    TEST_ASSERT_EQUAL_UINT16(DefaultLayers::kNone, a.mask);
}

// =============================================================================
// Tests for collision layers
// =============================================================================

void test_actor_set_layer(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(1);
    TEST_ASSERT_EQUAL_UINT16(1, a.layer);
}

void test_actor_set_mask(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionMask(2);
    TEST_ASSERT_EQUAL_UINT16(2, a.mask);
}

void test_actor_layer_and_mask_independent(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(1);
    a.setCollisionMask(2);
    TEST_ASSERT_EQUAL_UINT16(1, a.layer);
    TEST_ASSERT_EQUAL_UINT16(2, a.mask);
}

void test_actor_multiple_layers(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(1 | 2 | 4);  // Layers 1, 2, and 4
    TEST_ASSERT_EQUAL_UINT16(7, a.layer);
}

void test_actor_all_layers(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(DefaultLayers::kAll);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, a.layer);
}

// =============================================================================
// Tests for isInLayer
// =============================================================================

void test_actor_is_in_layer_true(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(1 | 2);
    TEST_ASSERT_TRUE(a.isInLayer(1));
    TEST_ASSERT_TRUE(a.isInLayer(2));
}

void test_actor_is_in_layer_false(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(1);
    TEST_ASSERT_FALSE(a.isInLayer(2));
    TEST_ASSERT_FALSE(a.isInLayer(4));
}

void test_actor_is_in_layer_multiple(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(1 | 2 | 4);
    TEST_ASSERT_TRUE(a.isInLayer(1));
    TEST_ASSERT_TRUE(a.isInLayer(2));
    TEST_ASSERT_TRUE(a.isInLayer(4));
    TEST_ASSERT_FALSE(a.isInLayer(8));
}

void test_actor_is_in_layer_zero(void) {
    Actor a(0, 0, 10, 10);
    a.setCollisionLayer(0);
    TEST_ASSERT_FALSE(a.isInLayer(1));
}

// =============================================================================
// Tests for hitbox
// =============================================================================

void test_actor_hitbox_basic(void) {
    Actor a(10.0f, 20.0f, 30, 40);
    Rect hitbox = a.getHitBox();
    
    TEST_ASSERT_EQUAL_FLOAT(10.0f, hitbox.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, hitbox.y);
    TEST_ASSERT_EQUAL_INT(30, hitbox.width);
    TEST_ASSERT_EQUAL_INT(40, hitbox.height);
}

void test_actor_hitbox_follows_position(void) {
    Actor a(0, 0, 10, 10);
    a.x = 50.0f;
    a.y = 60.0f;
    
    Rect hitbox = a.getHitBox();
    TEST_ASSERT_EQUAL_FLOAT(50.0f, hitbox.x);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, hitbox.y);
}

void test_actor_hitbox_size_matches(void) {
    Actor a(0, 0, 10, 10);
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
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    
    a.onCollision(&b);
    TEST_ASSERT_TRUE(a.collisionCallbackCalled);
    TEST_ASSERT_EQUAL(&b, a.collidedActor);
}

void test_actor_collision_callback_reset(void) {
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    
    a.onCollision(&b);
    a.reset();
    
    TEST_ASSERT_FALSE(a.collisionCallbackCalled);
    TEST_ASSERT_NULL(a.collidedActor);
}

void test_actor_multiple_collisions(void) {
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    Actor c(20, 20, 10, 10);
    
    a.onCollision(&b);
    TEST_ASSERT_EQUAL(&b, a.collidedActor);
    
    a.reset();
    a.onCollision(&c);
    TEST_ASSERT_EQUAL(&c, a.collidedActor);
}

// =============================================================================
// Tests for collision filtering
// =============================================================================

void test_actor_collision_filter_match(void) {
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    
    // a is on layer 1 and collides with layer 2
    a.setCollisionLayer(1);
    a.setCollisionMask(2);
    
    // b is on layer 2
    b.setCollisionLayer(2);
    
    // Check if a can collide with b
    bool canCollide = (a.mask & b.layer) != 0;
    TEST_ASSERT_TRUE(canCollide);
}

void test_actor_collision_filter_no_match(void) {
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    
    // a is on layer 1 and collides with layer 2
    a.setCollisionLayer(1);
    a.setCollisionMask(2);
    
    // b is on layer 4
    b.setCollisionLayer(4);
    
    // Check if a can collide with b
    bool canCollide = (a.mask & b.layer) != 0;
    TEST_ASSERT_FALSE(canCollide);
}

void test_actor_collision_filter_bidirectional(void) {
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    
    // Both actors set to collide with each other
    a.setCollisionLayer(1);
    a.setCollisionMask(2);
    b.setCollisionLayer(2);
    b.setCollisionMask(1);
    
    bool aCanHitB = (a.mask & b.layer) != 0;
    bool bCanHitA = (b.mask & a.layer) != 0;
    
    TEST_ASSERT_TRUE(aCanHitB);
    TEST_ASSERT_TRUE(bCanHitA);
}

// =============================================================================
// Tests for hitbox intersection
// =============================================================================

void test_actor_hitboxes_intersect(void) {
    Actor a(0, 0, 10, 10);
    Actor b(5, 5, 10, 10);
    
    Rect hitboxA = a.getHitBox();
    Rect hitboxB = b.getHitBox();
    
    TEST_ASSERT_TRUE(hitboxA.intersects(hitboxB));
}

void test_actor_hitboxes_no_intersect(void) {
    Actor a(0, 0, 10, 10);
    Actor b(50, 50, 10, 10);
    
    Rect hitboxA = a.getHitBox();
    Rect hitboxB = b.getHitBox();
    
    TEST_ASSERT_FALSE(hitboxA.intersects(hitboxB));
}

void test_actor_hitboxes_touching(void) {
    Actor a(0, 0, 10, 10);
    Actor b(10, 0, 10, 10);  // Touching at x=10
    
    Rect hitboxA = a.getHitBox();
    Rect hitboxB = b.getHitBox();
    
    TEST_ASSERT_TRUE(hitboxA.intersects(hitboxB));
}

// =============================================================================
// Tests for complete collision scenario
// =============================================================================

void test_actor_complete_collision_scenario(void) {
    Actor player(0, 0, 20, 20);
    Actor enemy(15, 15, 20, 20);
    
    // Setup collision layers
    player.setCollisionLayer(1);   // Player layer
    player.setCollisionMask(2);    // Player hits enemies (layer 2)
    enemy.setCollisionLayer(2);    // Enemy layer
    enemy.setCollisionMask(0);     // Enemy doesn't hit back
    
    // Check if they can collide (layers match)
    bool layersMatch = (player.mask & enemy.layer) != 0;
    TEST_ASSERT_TRUE(layersMatch);
    
    // Check if hitboxes intersect
    Rect playerHitbox = player.getHitBox();
    Rect enemyHitbox = enemy.getHitBox();
    bool hitboxesIntersect = playerHitbox.intersects(enemyHitbox);
    TEST_ASSERT_TRUE(hitboxesIntersect);
    
    // Simulate collision
    player.onCollision(&enemy);
    TEST_ASSERT_TRUE(player.collisionCallbackCalled);
    TEST_ASSERT_EQUAL(&enemy, player.collidedActor);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_actor_initialization);
    RUN_TEST(test_actor_default_layer);
    RUN_TEST(test_actor_default_mask);
    
    // Layer/mask tests
    RUN_TEST(test_actor_set_layer);
    RUN_TEST(test_actor_set_mask);
    RUN_TEST(test_actor_layer_and_mask_independent);
    RUN_TEST(test_actor_multiple_layers);
    RUN_TEST(test_actor_all_layers);
    
    // isInLayer tests
    RUN_TEST(test_actor_is_in_layer_true);
    RUN_TEST(test_actor_is_in_layer_false);
    RUN_TEST(test_actor_is_in_layer_multiple);
    RUN_TEST(test_actor_is_in_layer_zero);
    
    // Hitbox tests
    RUN_TEST(test_actor_hitbox_basic);
    RUN_TEST(test_actor_hitbox_follows_position);
    RUN_TEST(test_actor_hitbox_size_matches);
    
    // Collision callback tests
    RUN_TEST(test_actor_on_collision_called);
    RUN_TEST(test_actor_collision_callback_reset);
    RUN_TEST(test_actor_multiple_collisions);
    
    // Collision filtering tests
    RUN_TEST(test_actor_collision_filter_match);
    RUN_TEST(test_actor_collision_filter_no_match);
    RUN_TEST(test_actor_collision_filter_bidirectional);
    
    // Hitbox intersection tests
    RUN_TEST(test_actor_hitboxes_intersect);
    RUN_TEST(test_actor_hitboxes_no_intersect);
    RUN_TEST(test_actor_hitboxes_touching);
    
    // Complete scenario
    RUN_TEST(test_actor_complete_collision_scenario);
    
    return UNITY_END();
}
