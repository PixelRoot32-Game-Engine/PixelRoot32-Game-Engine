/**
 * @file test_entity.cpp
 * @brief Unit tests for core/Entity module using real engine headers.
 * @version 1.1
 * @date 2026-02-08
 */

#include <unity.h>
#include "../../test_config.h"
#include "core/Entity.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// Mock Entity implementation for testing real Entity class
class MockEntity : public Entity {
public:
    bool updateCalled = false;
    bool drawCalled = false;
    unsigned long lastDeltaTime = 0;
    
    MockEntity(float x, float y, int w, int h, EntityType t = EntityType::GENERIC)
        : Entity(pixelroot32::math::Vector2(x, y), w, h, t) {}
    
    void update(unsigned long deltaTime) override {
        updateCalled = true;
        lastDeltaTime = deltaTime;
    }
    
    void draw(pixelroot32::graphics::Renderer& renderer) override {
        (void)renderer;
        drawCalled = true;
    }
    
    void reset() {
        updateCalled = false;
        drawCalled = false;
        lastDeltaTime = 0;
    }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for Entity initialization
// =============================================================================

void test_entity_initialization(void) {
    MockEntity e(10.0f, 20.0f, 30, 40, EntityType::GENERIC);
    
    TEST_ASSERT_EQUAL_FLOAT(10.0f, e.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, e.position.y);
    TEST_ASSERT_EQUAL_INT(30, e.width);
    TEST_ASSERT_EQUAL_INT(40, e.height);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(EntityType::GENERIC), static_cast<int>(e.type));
}

void test_entity_default_visibility(void) {
    MockEntity e(0, 0, 10, 10);
    TEST_ASSERT_TRUE(e.isVisible);
}

void test_entity_default_enabled(void) {
    MockEntity e(0, 0, 10, 10);
    TEST_ASSERT_TRUE(e.isEnabled);
}

void test_entity_default_render_layer(void) {
    MockEntity e(0, 0, 10, 10);
    TEST_ASSERT_EQUAL_INT(1, e.renderLayer);
}

// =============================================================================
// Tests for visibility
// =============================================================================

void test_entity_set_visible_true(void) {
    MockEntity e(0, 0, 10, 10);
    e.setVisible(true);
    TEST_ASSERT_TRUE(e.isVisible);
}

void test_entity_set_visible_false(void) {
    MockEntity e(0, 0, 10, 10);
    e.setVisible(false);
    TEST_ASSERT_FALSE(e.isVisible);
}

void test_entity_toggle_visibility(void) {
    MockEntity e(0, 0, 10, 10);
    e.setVisible(false);
    TEST_ASSERT_FALSE(e.isVisible);
    e.setVisible(true);
    TEST_ASSERT_TRUE(e.isVisible);
}

// =============================================================================
// Tests for enabled state
// =============================================================================

void test_entity_set_enabled_true(void) {
    MockEntity e(0, 0, 10, 10);
    e.setEnabled(true);
    TEST_ASSERT_TRUE(e.isEnabled);
}

void test_entity_set_enabled_false(void) {
    MockEntity e(0, 0, 10, 10);
    e.setEnabled(false);
    TEST_ASSERT_FALSE(e.isEnabled);
}

void test_entity_toggle_enabled(void) {
    MockEntity e(0, 0, 10, 10);
    e.setEnabled(false);
    TEST_ASSERT_FALSE(e.isEnabled);
    e.setEnabled(true);
    TEST_ASSERT_TRUE(e.isEnabled);
}

// =============================================================================
// Tests for render layer
// =============================================================================

void test_entity_set_render_layer(void) {
    MockEntity e(0, 0, 10, 10);
    e.setRenderLayer(5);
    TEST_ASSERT_EQUAL_INT(5, e.renderLayer);
}

void test_entity_get_render_layer(void) {
    MockEntity e(0, 0, 10, 10);
    e.renderLayer = 3;
    TEST_ASSERT_EQUAL_INT(3, e.getRenderLayer());
}

void test_entity_render_layer_zero(void) {
    MockEntity e(0, 0, 10, 10);
    e.setRenderLayer(0);
    TEST_ASSERT_EQUAL_INT(0, e.getRenderLayer());
}

void test_entity_render_layer_max(void) {
    MockEntity e(0, 0, 10, 10);
    e.setRenderLayer(255);
    TEST_ASSERT_EQUAL_INT(255, e.getRenderLayer());
}

// =============================================================================
// Tests for position manipulation
// =============================================================================

void test_entity_position_change(void) {
    MockEntity e(0, 0, 10, 10);
    e.position.x = 100.0f;
    e.position.y = 200.0f;
    TEST_ASSERT_EQUAL_FLOAT(100.0f, e.position.x);
    TEST_ASSERT_EQUAL_FLOAT(200.0f, e.position.y);
}

void test_entity_negative_position(void) {
    MockEntity e(-50.0f, -100.0f, 10, 10);
    TEST_ASSERT_EQUAL_FLOAT(-50.0f, e.position.x);
    TEST_ASSERT_EQUAL_FLOAT(-100.0f, e.position.y);
}

void test_entity_decimal_position(void) {
    MockEntity e(10.5f, 20.75f, 10, 10);
    TEST_ASSERT_EQUAL_FLOAT(10.5f, e.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20.75f, e.position.y);
}

// =============================================================================
// Tests for dimensions
// =============================================================================

void test_entity_dimensions_change(void) {
    MockEntity e(0, 0, 10, 10);
    e.width = 50;
    e.height = 60;
    TEST_ASSERT_EQUAL_INT(50, e.width);
    TEST_ASSERT_EQUAL_INT(60, e.height);
}

void test_entity_zero_dimensions(void) {
    MockEntity e(0, 0, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, e.width);
    TEST_ASSERT_EQUAL_INT(0, e.height);
}

void test_entity_large_dimensions(void) {
    MockEntity e(0, 0, 1000, 2000);
    TEST_ASSERT_EQUAL_INT(1000, e.width);
    TEST_ASSERT_EQUAL_INT(2000, e.height);
}

// =============================================================================
// Tests for entity types
// =============================================================================

void test_entity_type_generic(void) {
    MockEntity e(0, 0, 10, 10, EntityType::GENERIC);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(EntityType::GENERIC), static_cast<int>(e.type));
}

void test_entity_type_actor(void) {
    MockEntity e(0, 0, 10, 10, EntityType::ACTOR);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(EntityType::ACTOR), static_cast<int>(e.type));
}

void test_entity_type_ui(void) {
    MockEntity e(0, 0, 10, 10, EntityType::UI_ELEMENT);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(EntityType::UI_ELEMENT), static_cast<int>(e.type));
}

// =============================================================================
// Tests for virtual methods
// =============================================================================

void test_entity_update_called(void) {
    MockEntity e(0, 0, 10, 10);
    e.update(16);
    TEST_ASSERT_TRUE(e.updateCalled);
    TEST_ASSERT_EQUAL_INT(16, e.lastDeltaTime);
}

void test_entity_draw_called(void) {
    MockEntity e(0, 0, 10, 10);
    // Use MockRenderer instead of real Renderer if needed, 
    // but here we just need any Renderer reference.
    // Since Renderer is now real, we need a DisplayConfig to init it.
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 240, 240, 0, 0);
    Renderer r(config);
    e.draw(r);
    TEST_ASSERT_TRUE(e.drawCalled);
}

void test_entity_update_with_different_delta(void) {
    MockEntity e(0, 0, 10, 10);
    e.update(33);
    TEST_ASSERT_EQUAL_INT(33, e.lastDeltaTime);
    e.update(16);
    TEST_ASSERT_EQUAL_INT(16, e.lastDeltaTime);
}

// =============================================================================
// Tests for combined state
// =============================================================================

void test_entity_visibility_and_enabled(void) {
    MockEntity e(0, 0, 10, 10);
    
    // Both true by default
    TEST_ASSERT_TRUE(e.isVisible);
    TEST_ASSERT_TRUE(e.isEnabled);
    
    // Set visibility false
    e.setVisible(false);
    TEST_ASSERT_FALSE(e.isVisible);
    TEST_ASSERT_TRUE(e.isEnabled);
    
    // Set enabled false
    e.setEnabled(false);
    TEST_ASSERT_FALSE(e.isVisible);
    TEST_ASSERT_FALSE(e.isEnabled);
}

void test_entity_state_independence(void) {
    MockEntity e(0, 0, 10, 10);
    
    // Visibility and enabled are independent
    e.setVisible(false);
    e.setEnabled(true);
    TEST_ASSERT_FALSE(e.isVisible);
    TEST_ASSERT_TRUE(e.isEnabled);
    
    e.setVisible(true);
    e.setEnabled(false);
    TEST_ASSERT_TRUE(e.isVisible);
    TEST_ASSERT_FALSE(e.isEnabled);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_entity_initialization);
    RUN_TEST(test_entity_default_visibility);
    RUN_TEST(test_entity_default_enabled);
    RUN_TEST(test_entity_default_render_layer);
    
    // Visibility tests
    RUN_TEST(test_entity_set_visible_true);
    RUN_TEST(test_entity_set_visible_false);
    RUN_TEST(test_entity_toggle_visibility);
    
    // Enabled tests
    RUN_TEST(test_entity_set_enabled_true);
    RUN_TEST(test_entity_set_enabled_false);
    RUN_TEST(test_entity_toggle_enabled);
    
    // Render layer tests
    RUN_TEST(test_entity_set_render_layer);
    RUN_TEST(test_entity_get_render_layer);
    RUN_TEST(test_entity_render_layer_zero);
    RUN_TEST(test_entity_render_layer_max);
    
    // Position tests
    RUN_TEST(test_entity_position_change);
    RUN_TEST(test_entity_negative_position);
    RUN_TEST(test_entity_decimal_position);
    
    // Dimension tests
    RUN_TEST(test_entity_dimensions_change);
    RUN_TEST(test_entity_zero_dimensions);
    RUN_TEST(test_entity_large_dimensions);
    
    // Type tests
    RUN_TEST(test_entity_type_generic);
    RUN_TEST(test_entity_type_actor);
    RUN_TEST(test_entity_type_ui);
    
    // Virtual method tests
    RUN_TEST(test_entity_update_called);
    RUN_TEST(test_entity_draw_called);
    RUN_TEST(test_entity_update_with_different_delta);
    
    // Combined state tests
    RUN_TEST(test_entity_visibility_and_enabled);
    RUN_TEST(test_entity_state_independence);
    
    return UNITY_END();
}
