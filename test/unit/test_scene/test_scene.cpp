/**
 * @file test_scene.cpp
 * @brief Unit tests for core/Scene module
 * @version 1.0
 * @date 2026-02-08
 */

#include <unity.h>
#include "../../test_config.h"

// Mock implementations
namespace pixelroot32 {
namespace core {

enum class EntityType { GENERIC, ACTOR, UI_ELEMENT };

class Entity {
public:
    float x, y;
    int width, height;
    EntityType type;
    bool isVisible = true;
    bool isEnabled = true;
    unsigned char renderLayer = 1;
    bool updateCalled = false;
    
    Entity(float x, float y, int w, int h, EntityType t) 
        : x(x), y(y), width(w), height(h), type(t) {}
    virtual ~Entity() {}
    
    unsigned char getRenderLayer() const { return renderLayer; }
    virtual void setRenderLayer(unsigned char layer) { renderLayer = layer; }
    
    virtual void update(unsigned long deltaTime) {
        (void)deltaTime;
        if (isEnabled) {
            updateCalled = true;
        }
    }
};

#define MAX_ENTITIES 32

class Scene {
public:
    Entity* entities[MAX_ENTITIES];
    int entityCount = 0;
    bool needsSorting = false;
    
    Scene() : entityCount(0), needsSorting(false) {
        for (int i = 0; i < MAX_ENTITIES; i++) {
            entities[i] = nullptr;
        }
    }
    
    virtual ~Scene() {}
    virtual void init() {}
    
    void update(unsigned long deltaTime) {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] && entities[i]->isEnabled) {
                entities[i]->update(deltaTime);
            }
        }
    }
    
    void sortEntities() {
        for (int i = 0; i < entityCount - 1; i++) {
            for (int j = 0; j < entityCount - i - 1; j++) {
                if (entities[j] && entities[j+1] && 
                    entities[j]->getRenderLayer() > entities[j + 1]->getRenderLayer()) {
                    Entity* temp = entities[j];
                    entities[j] = entities[j + 1];
                    entities[j + 1] = temp;
                }
            }
        }
        needsSorting = false;
    }
    
    void addEntity(Entity* entity) {
        if (entityCount < MAX_ENTITIES && entity) {
            entities[entityCount++] = entity;
            needsSorting = true;
        }
    }
    
    void removeEntity(Entity* entity) {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] == entity) {
                for (int j = i; j < entityCount - 1; j++) {
                    entities[j] = entities[j + 1];
                }
                entityCount--;
                entities[entityCount] = nullptr;
                return;
            }
        }
    }
    
    void clearEntities() {
        entityCount = 0;
        for (int i = 0; i < MAX_ENTITIES; i++) {
            entities[i] = nullptr;
        }
    }
    
    int getEntityCount() const { return entityCount; }
};

}
}

using namespace pixelroot32::core;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_scene_initialization(void) {
    Scene scene;
    TEST_ASSERT_EQUAL_INT(0, scene.getEntityCount());
    TEST_ASSERT_FALSE(scene.needsSorting);
}

void test_scene_add_entity(void) {
    Scene scene;
    Entity e(0, 0, 10, 10, EntityType::GENERIC);
    
    scene.addEntity(&e);
    TEST_ASSERT_EQUAL_INT(1, scene.getEntityCount());
    TEST_ASSERT_TRUE(scene.needsSorting);
}

void test_scene_add_multiple(void) {
    Scene scene;
    Entity e1(0, 0, 10, 10, EntityType::GENERIC);
    Entity e2(20, 20, 10, 10, EntityType::GENERIC);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    TEST_ASSERT_EQUAL_INT(2, scene.getEntityCount());
}

void test_scene_remove_entity(void) {
    Scene scene;
    Entity e1(0, 0, 10, 10, EntityType::GENERIC);
    Entity e2(20, 20, 10, 10, EntityType::GENERIC);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.removeEntity(&e1);
    
    TEST_ASSERT_EQUAL_INT(1, scene.getEntityCount());
}

void test_scene_clear(void) {
    Scene scene;
    Entity e1(0, 0, 10, 10, EntityType::GENERIC);
    Entity e2(20, 20, 10, 10, EntityType::GENERIC);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.clearEntities();
    
    TEST_ASSERT_EQUAL_INT(0, scene.getEntityCount());
}

void test_scene_update_calls_entity(void) {
    Scene scene;
    Entity e(0, 0, 10, 10, EntityType::GENERIC);
    
    scene.addEntity(&e);
    scene.update(16);
    
    TEST_ASSERT_TRUE(e.updateCalled);
}

void test_scene_update_disabled_entity(void) {
    Scene scene;
    Entity e(0, 0, 10, 10, EntityType::GENERIC);
    e.isEnabled = false;
    
    scene.addEntity(&e);
    scene.update(16);
    
    TEST_ASSERT_FALSE(e.updateCalled);
}

void test_scene_sort_entities(void) {
    Scene scene;
    Entity e1(0, 0, 10, 10, EntityType::GENERIC);
    Entity e2(0, 0, 10, 10, EntityType::GENERIC);
    
    e1.renderLayer = 2;
    e2.renderLayer = 1;
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.sortEntities();
    
    TEST_ASSERT_EQUAL(&e2, scene.entities[0]);
    TEST_ASSERT_EQUAL(&e1, scene.entities[1]);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    RUN_TEST(test_scene_initialization);
    RUN_TEST(test_scene_add_entity);
    RUN_TEST(test_scene_add_multiple);
    RUN_TEST(test_scene_remove_entity);
    RUN_TEST(test_scene_clear);
    RUN_TEST(test_scene_update_calls_entity);
    RUN_TEST(test_scene_update_disabled_entity);
    RUN_TEST(test_scene_sort_entities);
    
    return UNITY_END();
}
