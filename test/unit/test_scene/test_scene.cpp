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
// Tests for re-init idempotency (Phase 1 — RED)
// =============================================================================

void test_scene_reinit_clears_entities(void) {
    class ReinitTestScene : public Scene {
    public:
        int getEntityCount() const { return entityCount; }
        Entity** getEntities() { return entities; }
    };

    ReinitTestScene scene;
    MockEntity e1(0, 0, 10, 10);
    MockEntity e2(10, 0, 10, 10);
    MockEntity e3(20, 0, 10, 10);

    scene.init();

    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.addEntity(&e3);
    TEST_ASSERT_EQUAL(3, scene.getEntityCount());

    // Second init MUST clear entities (idempotency)
    scene.init();

    TEST_ASSERT_EQUAL(0, scene.getEntityCount());

    // Stale entities MUST NOT be updated after re-init
    scene.update(16);
    TEST_ASSERT_FALSE(e1.updateCalled);
    TEST_ASSERT_FALSE(e2.updateCalled);
    TEST_ASSERT_FALSE(e3.updateCalled);
}

void test_scene_reinit_derived_unique_ptr(void) {
    class DerivedScene : public Scene {
    public:
        std::unique_ptr<MockEntity> entity;
        int getCount() const { return entityCount; }

        void init() override {
            Scene::init();
            entity = std::make_unique<MockEntity>(0, 0, 10, 10);
            addEntity(entity.get());
        }
    };

    DerivedScene scene;
    scene.init();
    TEST_ASSERT_EQUAL(1, scene.getCount());

    // Second init — old unique_ptr destroyed, new one created
    scene.init();

    // After re-init, entityCount must reflect a single fresh init (1, not 2)
    TEST_ASSERT_EQUAL(1, scene.getCount());

    // update MUST not crash or touch the destroyed old entity
    scene.update(16);
}

void test_scene_reinit_idempotent(void) {
    Scene scene;

    scene.init();
    scene.init();  // Must not crash

    // Empty scene re-init must stay empty
    // (verified via no crash + no-op behavior)
}

void test_scene_reinit_physics_clear(void) {
#if PIXELROOT32_ENABLE_PHYSICS
    class PhyTestScene : public Scene {
    public:
        int getCount() const { return entityCount; }
        pixelroot32::physics::CollisionSystem& getCollisionSystem() {
            return collisionSystem;
        }
    };

    PhyTestScene scene;
    MockEntity e1(0, 0, 10, 10);
    MockEntity e2(10, 0, 10, 10);

    scene.init();
    scene.addEntity(&e1);
    scene.addEntity(&e2);

    // Verify collision system has registered entities
    TEST_ASSERT_EQUAL(2, scene.getCollisionSystem().getEntityCount());

    // Re-init MUST clear both scene and collision system state
    scene.init();

    TEST_ASSERT_EQUAL(0, scene.getCount());
    TEST_ASSERT_EQUAL(0, scene.getCollisionSystem().getEntityCount());
#endif
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

void test_scene_draw_with_offset_and_layers(void) {
    Scene scene;
    MockEntity e1(0, 0, 20, 20);
    MockEntity e2(50, 50, 20, 20);
    e1.setRenderLayer(0);
    e2.setRenderLayer(1);
    scene.addEntity(&e1);
    scene.addEntity(&e2);
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);
    renderer.setDisplayOffset(10, 10);
    scene.draw(renderer);
    TEST_ASSERT_TRUE(e1.drawCalled);
    TEST_ASSERT_TRUE(e2.drawCalled);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_scene_add_remove_entity);
    RUN_TEST(test_scene_clear_entities);
    RUN_TEST(test_scene_entity_sorting);
    RUN_TEST(test_scene_update_propagation);
    RUN_TEST(test_scene_draw_propagation);
    RUN_TEST(test_scene_draw_with_offset_and_layers);
    
    // Re-init idempotency tests (RED)
    RUN_TEST(test_scene_reinit_clears_entities);
    RUN_TEST(test_scene_reinit_derived_unique_ptr);
    RUN_TEST(test_scene_reinit_idempotent);
    RUN_TEST(test_scene_reinit_physics_clear);
    
    return UNITY_END();
}
