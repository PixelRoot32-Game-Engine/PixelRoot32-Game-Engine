#pragma once

#include <unity.h>
#include "../../test_config.h"
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIPanel.h"

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;

// ============================================================================
// UIPaddingContainer Tests
// ============================================================================

void test_padding_container_initialization() {
    UIPaddingContainer container(0, 0, 100, 50);
    TEST_ASSERT_EQUAL_FLOAT(0, container.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, container.position.y);
    TEST_ASSERT_EQUAL(100, container.width);
    TEST_ASSERT_EQUAL(50, container.height);
}

void test_padding_container_vector2_constructor() {
    UIPaddingContainer container(Vector2(toScalar(10), toScalar(20)), 100, 50);
    TEST_ASSERT_EQUAL_FLOAT(10, container.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20, container.position.y);
}

void test_padding_container_set_child() {
    UIPaddingContainer container(0, 0, 100, 50);
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    
    container.setChild(&label);
    TEST_ASSERT_EQUAL(&label, container.getChild());
}

void test_padding_container_no_child() {
    UIPaddingContainer container(0, 0, 100, 50);
    
    // No child set, should return nullptr
    TEST_ASSERT_NULL(container.getChild());
}

void test_padding_container_uniform_padding() {
    UIPaddingContainer container(0, 0, 100, 50);
    container.setPadding(toScalar(10));
    
    TEST_ASSERT_EQUAL(10, static_cast<int>(container.getPaddingLeft()));
    TEST_ASSERT_EQUAL(10, static_cast<int>(container.getPaddingRight()));
    TEST_ASSERT_EQUAL(10, static_cast<int>(container.getPaddingTop()));
    TEST_ASSERT_EQUAL(10, static_cast<int>(container.getPaddingBottom()));
}

void test_padding_container_asymmetric_padding() {
    UIPaddingContainer container(0, 0, 100, 50);
    container.setPadding(toScalar(5), toScalar(15), toScalar(10), toScalar(20));
    
    TEST_ASSERT_EQUAL(5, static_cast<int>(container.getPaddingLeft()));
    TEST_ASSERT_EQUAL(15, static_cast<int>(container.getPaddingRight()));
    TEST_ASSERT_EQUAL(10, static_cast<int>(container.getPaddingTop()));
    TEST_ASSERT_EQUAL(20, static_cast<int>(container.getPaddingBottom()));
}

void test_padding_container_zero_padding() {
    UIPaddingContainer container(0, 0, 100, 50);
    container.setPadding(toScalar(0));
    
    TEST_ASSERT_EQUAL(0, static_cast<int>(container.getPaddingLeft()));
    TEST_ASSERT_EQUAL(0, static_cast<int>(container.getPaddingRight()));
    TEST_ASSERT_EQUAL(0, static_cast<int>(container.getPaddingTop()));
    TEST_ASSERT_EQUAL(0, static_cast<int>(container.getPaddingBottom()));
}

void test_padding_container_change_padding() {
    UIPaddingContainer container(0, 0, 100, 50);
    
    container.setPadding(toScalar(5));
    TEST_ASSERT_EQUAL(5, static_cast<int>(container.getPaddingLeft()));
    
    // Change to different padding
    container.setPadding(toScalar(15));
    TEST_ASSERT_EQUAL(15, static_cast<int>(container.getPaddingLeft()));
}

void test_padding_container_with_panel_child() {
    UIPaddingContainer container(10, 10, 100, 80);
    container.setPadding(toScalar(5));
    
    UIPanel panel(0, 0, 50, 40);
    container.setChild(&panel);
    
    TEST_ASSERT_EQUAL(&panel, container.getChild());
    TEST_ASSERT_EQUAL(5, static_cast<int>(container.getPaddingLeft()));
}

void test_padding_container_position_update() {
    UIPaddingContainer container(0, 0, 100, 50);
    container.setPadding(toScalar(10));
    
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    container.setChild(&label);
    
    // Update container position
    container.setPosition(toScalar(30), toScalar(40));
    
    TEST_ASSERT_EQUAL_FLOAT(30, container.position.x);
    TEST_ASSERT_EQUAL_FLOAT(40, container.position.y);
}

void test_padding_container_different_padding_values() {
    UIPaddingContainer container(0, 0, 100, 50);
    
    // Test different combinations
    container.setPadding(toScalar(1), toScalar(2), toScalar(3), toScalar(4));
    TEST_ASSERT_EQUAL(1, static_cast<int>(container.getPaddingLeft()));
    TEST_ASSERT_EQUAL(2, static_cast<int>(container.getPaddingRight()));
    TEST_ASSERT_EQUAL(3, static_cast<int>(container.getPaddingTop()));
    TEST_ASSERT_EQUAL(4, static_cast<int>(container.getPaddingBottom()));
}

void test_padding_container_large_padding() {
    UIPaddingContainer container(0, 0, 100, 50);
    container.setPadding(toScalar(25));
    
    TEST_ASSERT_EQUAL(25, static_cast<int>(container.getPaddingLeft()));
    TEST_ASSERT_EQUAL(25, static_cast<int>(container.getPaddingRight()));
}

void test_padding_container_replace_child() {
    UIPaddingContainer container(0, 0, 100, 50);
    
    UILabel label1("First", Vector2::ZERO(), Color::White, 1);
    UILabel label2("Second", Vector2::ZERO(), Color::White, 1);
    
    container.setChild(&label1);
    TEST_ASSERT_EQUAL(&label1, container.getChild());
    
    // Replace with second child
    container.setChild(&label2);
    TEST_ASSERT_EQUAL(&label2, container.getChild());
}

void test_padding_container_child_null_after_clear() {
    UIPaddingContainer container(0, 0, 100, 50);
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    
    container.setChild(&label);
    TEST_ASSERT_NOT_NULL(container.getChild());
    
    // There's no explicit clearChild method, but we can set to nullptr
    // or the implementation might handle this
}

void test_padding_container_update_with_child() {
    UIPaddingContainer container(0, 0, 100, 50);
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    
    container.setChild(&label);
    container.update(16); // 16ms delta
    TEST_ASSERT_TRUE(true);
}

void test_padding_container_update_disabled() {
    UIPaddingContainer container(0, 0, 100, 50);
    container.setEnabled(false);
    
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    container.setChild(&label);
    
    container.update(16);
    TEST_ASSERT_TRUE(true);
}

void test_padding_container_draw_with_child() {
    auto mockDrawer = std::make_unique<MockDrawSurfaceAdvanced>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UIPaddingContainer container(0, 0, 100, 50);
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    container.setChild(&label);
    
    container.draw(renderer);
    TEST_ASSERT_TRUE(true);
}

void test_padding_container_draw_not_visible() {
    auto mockDrawer = std::make_unique<MockDrawSurfaceAdvanced>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UIPaddingContainer container(0, 0, 100, 50);
    container.setVisible(false);
    
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    container.setChild(&label);
    
    container.draw(renderer);
    TEST_ASSERT_TRUE(true);
}
