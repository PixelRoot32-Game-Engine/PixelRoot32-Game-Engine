/**
 * @file test_collision_system.cpp
 * @brief Unit tests for physics/CollisionSystem module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for collision system including:
 * - Entity management (add/remove)
 * - Collision detection between actors
 * - Layer/mask filtering
 */

#include <unity.h>
#include <vector>
#include <algorithm>
#include "../../test_config.h"

// Mock implementations to avoid Arduino dependencies
namespace pixelroot32 {
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

// Forward declaration
class Actor;

// Base Entity class
class Entity {
public:
    float x, y;
    int width, height;
    EntityType type;
    bool isVisible = true;
    bool isEnabled = true;
    
    Entity(float x, float y, int w, int h, EntityType t) 
        : x(x), y(y), width(w), height(h), type(t) {}
    virtual ~Entity() {}
};

// Mock Actor class
class Actor : public Entity {
public:
    uint16_t layer = 0;
    uint16_t mask = 0;
    Rect hitBox;
    bool collisionCalled = false;
    Actor* collidedWith = nullptr;
    
    Actor(float x, float y, int w, int h) 
        : Entity(x, y, w, h, EntityType::ACTOR) {
        hitBox = {x, y, w, h};
    }
    
    void setCollisionLayer(uint16_t l) { layer = l; }
    void setCollisionMask(uint16_t m) { mask = m; }
    
    Rect getHitBox() { 
        hitBox.x = x;
        hitBox.y = y;
        hitBox.width = width;
        hitBox.height = height;
        return hitBox; 
    }
    
    void onCollision(Actor* other) {
        collisionCalled = true;
        collidedWith = other;
    }
    
    bool isInLayer(uint16_t targetLayer) const {
        return (layer & targetLayer) != 0;
    }
    
    void reset() {
        collisionCalled = false;
        collidedWith = nullptr;
    }
};

}

namespace physics {

using namespace pixelroot32::core;

class CollisionSystem {
public:
    std::vector<Entity*> entities;
    
    void addEntity(Entity* e) { 
        entities.push_back(e); 
    }
    
    void removeEntity(Entity* e) { 
        entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
    }
    
    void update() {
        for (size_t i = 0; i < entities.size(); i++) {
            if (entities[i]->type != EntityType::ACTOR) continue;
            
            Actor* actorA = static_cast<Actor*>(entities[i]);
            
            for (size_t j = i + 1; j < entities.size(); j++) {
                if (entities[j]->type != EntityType::ACTOR) continue;
                
                Actor* actorB = static_cast<Actor*>(entities[j]);
                
                // Check layer masks
                if ((actorA->mask & actorB->layer) || (actorB->mask & actorA->layer)) {
                    if (actorA->getHitBox().intersects(actorB->getHitBox())) {
                        actorA->onCollision(actorB);
                        actorB->onCollision(actorA);
                    }
                }
            }
        }
    }
    
    size_t getEntityCount() const { return entities.size(); }
    void clear() { entities.clear(); }
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
// Tests for entity management
// =============================================================================

void test_collision_system_add_entity(void) {
    CollisionSystem system;
    Actor actor(0, 0, 10, 10);
    
    system.addEntity(&actor);
    TEST_ASSERT_EQUAL_INT(1, system.getEntityCount());
}

void test_collision_system_add_multiple(void) {
    CollisionSystem system;
    Actor a1(0, 0, 10, 10);
    Actor a2(20, 20, 10, 10);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    TEST_ASSERT_EQUAL_INT(2, system.getEntityCount());
}

void test_collision_system_remove_entity(void) {
    CollisionSystem system;
    Actor a1(0, 0, 10, 10);
    Actor a2(20, 20, 10, 10);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.removeEntity(&a1);
    
    TEST_ASSERT_EQUAL_INT(1, system.getEntityCount());
}

void test_collision_system_clear(void) {
    CollisionSystem system;
    Actor a1(0, 0, 10, 10);
    Actor a2(20, 20, 10, 10);
    
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
    Actor a1(0, 0, 10, 10);
    Actor a2(50, 50, 10, 10);
    
    // Set up layers so they can collide
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
    Actor a1(0, 0, 20, 20);
    Actor a2(10, 10, 20, 20);
    
    // Set up layers so they can collide
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
    Actor a1(0, 0, 10, 10);
    Actor a2(10, 0, 10, 10);  // Touching at x=10
    
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

// =============================================================================
// Tests for layer/mask filtering
// =============================================================================

void test_collision_system_layer_mask_no_match(void) {
    CollisionSystem system;
    Actor a1(0, 0, 20, 20);
    Actor a2(10, 10, 20, 20);
    
    // a1 is on layer 1 but only collides with layer 2
    a1.setCollisionLayer(1);
    a1.setCollisionMask(2);
    
    // a2 is on layer 1 but only collides with layer 2
    a2.setCollisionLayer(1);
    a2.setCollisionMask(2);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    // They overlap but masks don't match
    TEST_ASSERT_FALSE(a1.collisionCalled);
    TEST_ASSERT_FALSE(a2.collisionCalled);
}

void test_collision_system_layer_mask_match(void) {
    CollisionSystem system;
    Actor a1(0, 0, 20, 20);
    Actor a2(10, 10, 20, 20);
    
    // a1 is on layer 1 and collides with layer 2
    a1.setCollisionLayer(1);
    a1.setCollisionMask(2);
    
    // a2 is on layer 2 and collides with layer 1
    a2.setCollisionLayer(2);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    // They overlap and masks match
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
}

void test_collision_system_one_way_collision(void) {
    CollisionSystem system;
    Actor player(0, 0, 20, 20);
    Actor enemy(10, 10, 20, 20);
    
    // Player can hit enemies (layer 2) but enemies don't hit player back
    player.setCollisionLayer(1);
    player.setCollisionMask(2);  // Player hits layer 2 (enemies)
    
    enemy.setCollisionLayer(2);   // Enemy is on layer 2
    enemy.setCollisionMask(0);    // Enemy doesn't hit anything
    
    system.addEntity(&player);
    system.addEntity(&enemy);
    system.update();
    
    // Both should be notified (the system notifies both)
    TEST_ASSERT_TRUE(player.collisionCalled);
    TEST_ASSERT_TRUE(enemy.collisionCalled);
}

void test_collision_system_multiple_layers(void) {
    CollisionSystem system;
    Actor a1(0, 0, 20, 20);
    Actor a2(10, 10, 20, 20);
    
    // a1 collides with layers 1, 2, and 4
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1 | 2 | 4);
    
    // a2 is on layer 4
    a2.setCollisionLayer(4);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    system.update();
    
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
}

// =============================================================================
// Tests for multiple entities
// =============================================================================

void test_collision_system_three_actors(void) {
    CollisionSystem system;
    Actor a1(0, 0, 20, 20);
    Actor a2(10, 10, 20, 20);  // Overlaps with a1
    Actor a3(100, 100, 20, 20); // No overlap
    
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

void test_collision_system_chain_collision(void) {
    CollisionSystem system;
    Actor a1(0, 0, 15, 15);
    Actor a2(10, 0, 15, 15);   // Overlaps with a1
    Actor a3(20, 0, 15, 15);   // Overlaps with a2 but not a1
    
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
    
    // a1 collides with a2
    TEST_ASSERT_TRUE(a1.collisionCalled);
    TEST_ASSERT_TRUE(a2.collisionCalled);
    
    // a2 collides with a3
    TEST_ASSERT_TRUE(a3.collisionCalled);
}

void test_collision_system_generic_entity_ignored(void) {
    CollisionSystem system;
    
    // Create a generic entity (not an actor)
    Entity generic(0, 0, 10, 10, EntityType::GENERIC);
    Actor actor(0, 0, 20, 20);
    
    actor.setCollisionLayer(1);
    actor.setCollisionMask(1);
    
    system.addEntity(&generic);
    system.addEntity(&actor);
    system.update();
    
    // Generic entities should be ignored
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

void test_collision_system_single_entity(void) {
    CollisionSystem system;
    Actor a1(0, 0, 10, 10);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.update();
    
    TEST_ASSERT_FALSE(a1.collisionCalled);
}

void test_collision_system_remove_and_update(void) {
    CollisionSystem system;
    Actor a1(0, 0, 20, 20);
    Actor a2(10, 10, 20, 20);
    
    a1.setCollisionLayer(1);
    a1.setCollisionMask(1);
    a2.setCollisionLayer(1);
    a2.setCollisionMask(1);
    
    system.addEntity(&a1);
    system.addEntity(&a2);
    
    // First update - collision detected
    system.update();
    TEST_ASSERT_TRUE(a1.collisionCalled);
    
    // Reset
    a1.reset();
    a2.reset();
    
    // Remove a2 and update again
    system.removeEntity(&a2);
    system.update();
    
    TEST_ASSERT_FALSE(a1.collisionCalled);
    TEST_ASSERT_FALSE(a2.collisionCalled);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Entity management tests
    RUN_TEST(test_collision_system_add_entity);
    RUN_TEST(test_collision_system_add_multiple);
    RUN_TEST(test_collision_system_remove_entity);
    RUN_TEST(test_collision_system_clear);
    
    // Collision detection tests
    RUN_TEST(test_collision_system_no_collision_separate);
    RUN_TEST(test_collision_system_collision_detected);
    RUN_TEST(test_collision_system_touching_edge);
    
    // Layer/mask tests
    RUN_TEST(test_collision_system_layer_mask_no_match);
    RUN_TEST(test_collision_system_layer_mask_match);
    RUN_TEST(test_collision_system_one_way_collision);
    RUN_TEST(test_collision_system_multiple_layers);
    
    // Multiple entities tests
    RUN_TEST(test_collision_system_three_actors);
    RUN_TEST(test_collision_system_chain_collision);
    RUN_TEST(test_collision_system_generic_entity_ignored);
    
    // Edge cases
    RUN_TEST(test_collision_system_empty);
    RUN_TEST(test_collision_system_single_entity);
    RUN_TEST(test_collision_system_remove_and_update);
    
    return UNITY_END();
}
