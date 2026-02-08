/**
 * @file test_scene.cpp
 * @brief Unit tests for core/Scene module
 * @version 1.1
 * @date 2026-02-08
 */

#include <unity.h>
#include "../../test_config.h"
#include "core/Scene.h"
#include "core/Entity.h"
#include "graphics/Renderer.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// Mock Entity implementation
class MockEntity : public Entity {
public:
    bool updateCalled = false;
    bool drawCalled = false;
    
    MockEntity(float x, float y, int w, int h, EntityType t = EntityType::GENERIC) 
        : Entity(x, y, w, h, t) {}
    
    virtual void update(unsigned long deltaTime) override {
        (void)deltaTime;
        updateCalled = true;
    }
    
    virtual void draw(Renderer& renderer) override {
        (void)renderer;
        drawCalled = true;
    }
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

void test_scene_add_remove_entity(void) {
    Scene scene;
    MockEntity entity(10, 10, 32, 32);
    
    scene.addEntity(&entity);
    // Note: We don't have a getEntityCount() in the real Scene, but we can verify behavior
    
    scene.update(16);
    TEST_ASSERT_TRUE(entity.updateCalled);
    
    scene.removeEntity(&entity);
    entity.updateCalled = false;
    scene.update(16);
    TEST_ASSERT_FALSE(entity.updateCalled);
}

void test_scene_clear_entities(void) {
    Scene scene;
    MockEntity e1(0, 0, 10, 10);
    MockEntity e2(10, 10, 10, 10);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.clearEntities();
    
    scene.update(16);
    TEST_ASSERT_FALSE(e1.updateCalled);
    TEST_ASSERT_FALSE(e2.updateCalled);
}

// =============================================================================
// Tests for sorting
// =============================================================================

void test_scene_entity_sorting(void) {
    // Note: Since needsSorting and sortEntities are protected/private in real Scene,
    // we verify sorting through draw order if possible, or we might need to make 
    // TestScene subclass to access protected members.
    
    class TestScene : public Scene {
    public:
        void forceSort() { sortEntities(); }
        Entity** getEntities() { return entities; }
        int getEntityCount() { return entityCount; }
    };
    
    TestScene scene;
    MockEntity e1(0, 0, 10, 10);
    MockEntity e2(0, 0, 10, 10);
    MockEntity e3(0, 0, 10, 10);
    
    e1.setRenderLayer(3);
    e2.setRenderLayer(1);
    e3.setRenderLayer(2);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.addEntity(&e3);
    
    scene.forceSort();
    
    Entity** entities = scene.getEntities();
    TEST_ASSERT_EQUAL(1, entities[0]->getRenderLayer());
    TEST_ASSERT_EQUAL(2, entities[1]->getRenderLayer());
    TEST_ASSERT_EQUAL(3, entities[2]->getRenderLayer());
}

// =============================================================================
// Tests for update and draw
// =============================================================================

void test_scene_update_propagation(void) {
    Scene scene;
    MockEntity e1(0, 0, 10, 10);
    MockEntity e2(0, 0, 10, 10);
    
    e2.setEnabled(false);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    
    scene.update(16);
    
    TEST_ASSERT_TRUE(e1.updateCalled);
    TEST_ASSERT_FALSE(e2.updateCalled);
}

void test_scene_draw_propagation(void) {
    Scene scene;
    MockEntity e1(0, 0, 10, 10);
    MockEntity e2(0, 0, 10, 10);
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);
    
    e2.setVisible(false);
    
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    
    scene.draw(renderer);
    
    TEST_ASSERT_TRUE(e1.drawCalled);
    TEST_ASSERT_FALSE(e2.drawCalled);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_scene_add_remove_entity);
    RUN_TEST(test_scene_clear_entities);
    RUN_TEST(test_scene_entity_sorting);
    RUN_TEST(test_scene_update_propagation);
    RUN_TEST(test_scene_draw_propagation);
    
    return UNITY_END();
}
