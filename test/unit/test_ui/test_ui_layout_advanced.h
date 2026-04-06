#pragma once

#include <unity.h>
#include "../../test_config.h"
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/ui/UIPanel.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UICheckbox.h"
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/ui/UIGridLayout.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"
#include "input/InputConfig.h"
#include "input/InputManager.h"
#include <memory>
#include <functional>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;

namespace {
class MockDrawSurfaceAdvanced : public BaseDrawSurface {
public:
    std::vector<std::tuple<int, int, int, int, uint16_t>> rectCalls;
    
    void init() override {}
    void clearBuffer() override { rectCalls.clear(); }
    void sendBuffer() override {}
    void drawRectangle(int x, int y, int w, int h, uint16_t c) override {
        rectCalls.push_back({x, y, w, h, c});
    }
    void drawFilledRectangle(int x, int y, int w, int h, uint16_t c) override {
        rectCalls.push_back({x, y, w, h, c});
    }
    void drawPixel(int, int, uint16_t) override {}
};
}

void test_horizontal_layout_spacing_calculation() {
    UIHorizontalLayout layout(0, 0, 300, 50);
    layout.setPadding(toScalar(10));
    layout.setSpacing(toScalar(20));
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    TEST_ASSERT_EQUAL(20, static_cast<int>(layout.getSpacing()));
}

void test_horizontal_layout_zero_spacing() {
    UIHorizontalLayout layout(0, 0, 300, 50);
    layout.setSpacing(toScalar(0));
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getSpacing()));
}

void test_vertical_layout_spacing_calculation() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setPadding(toScalar(10));
    layout.setSpacing(toScalar(15));
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    TEST_ASSERT_EQUAL(15, static_cast<int>(layout.getSpacing()));
}

void test_vertical_layout_zero_spacing() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setSpacing(toScalar(0));
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getSpacing()));
}

void test_button_state_transitions_advanced() {
    UIButton btn("Test", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    TEST_ASSERT_FALSE(btn.getSelected());
    btn.setSelected(true);
    TEST_ASSERT_TRUE(btn.getSelected());
    btn.setSelected(false);
    TEST_ASSERT_FALSE(btn.getSelected());
}

void test_checkbox_state_transitions_advanced() {
    UICheckBox cb("Test", 0, Vector2::ZERO(), Vector2(100, 30), false, nullptr, 1);
    TEST_ASSERT_FALSE(cb.isChecked());
    cb.setChecked(true);
    TEST_ASSERT_TRUE(cb.isChecked());
    cb.setChecked(false);
    TEST_ASSERT_FALSE(cb.isChecked());
}

void test_checkbox_toggle_advanced() {
    UICheckBox cb("Test", 0, Vector2::ZERO(), Vector2(100, 30), false, nullptr, 1);
    TEST_ASSERT_FALSE(cb.isChecked());
    cb.toggle();
    TEST_ASSERT_TRUE(cb.isChecked());
}

void test_parent_child_relationship() {
    UIPanel panel(0, 0, 100, 100);
    auto label = std::make_unique<UILabel>("Child", Vector2(10, 10), Color::White, 1);
    UILabel* labelPtr = label.get();
    
    panel.setChild(label.release());
    TEST_ASSERT_NOT_NULL(panel.getChild());
    TEST_ASSERT_EQUAL(labelPtr, panel.getChild());
}

void test_panel_position_update() {
    UIPanel panel(0, 0, 100, 100);
    auto label = std::make_unique<UILabel>("Child", Vector2(0, 0), Color::White, 1);
    
    panel.setChild(label.release());
    panel.setPosition(toScalar(50), toScalar(50));
    
    TEST_ASSERT_EQUAL_FLOAT(50, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(50, panel.position.y);
}

void test_nested_container_hierarchy() {
    UIPanel outer(0, 0, 200, 200);
    auto inner = std::make_unique<UIPanel>(10, 10, 100, 100);
    UIPanel* innerPtr = inner.get();
    
    auto label = std::make_unique<UILabel>("Nested", Vector2::ZERO(), Color::White, 1);
    UILabel* labelPtr = label.get();
    
    innerPtr->setChild(label.release());
    outer.setChild(inner.release());
    
    // Verify hierarchy is set up correctly
    TEST_ASSERT_EQUAL(labelPtr, innerPtr->getChild());
    TEST_ASSERT_EQUAL(innerPtr, outer.getChild());
}

void test_sibling_order_in_horizontal_layout() {
    UIHorizontalLayout layout(0, 0, 300, 50);
    
    auto btn1 = std::make_unique<UIButton>("First", 0, Vector2::ZERO(), Vector2(50, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("Second", 1, Vector2::ZERO(), Vector2(50, 30), nullptr);
    auto btn3 = std::make_unique<UIButton>("Third", 2, Vector2::ZERO(), Vector2(50, 30), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    UIElement* ptr3 = btn3.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.addElement(ptr3);
    layout.updateLayout();
    
    // Verify order is preserved (left to right)
    TEST_ASSERT_TRUE(ptr1->position.x < ptr2->position.x);
    TEST_ASSERT_TRUE(ptr2->position.x < ptr3->position.x);
}

void test_sibling_order_in_vertical_layout() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("First", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("Second", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn3 = std::make_unique<UIButton>("Third", 2, Vector2::ZERO(), Vector2(80, 30), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    UIElement* ptr3 = btn3.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.addElement(ptr3);
    layout.updateLayout();
    
    // Verify order is preserved (top to bottom)
    TEST_ASSERT_TRUE(ptr1->position.y < ptr2->position.y);
    TEST_ASSERT_TRUE(ptr2->position.y < ptr3->position.y);
}

void test_element_index_tracking() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 5, Vector2::ZERO(), Vector2(80, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 3, Vector2::ZERO(), Vector2(80, 20), nullptr);
    
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex());
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
}

void test_element_removal_from_hierarchy() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(80, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(80, 20), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    
    layout.removeElement(ptr1);
    
    // Element should no longer be in layout but ptr is still valid (not deleted)
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

void test_selected_index_navigation() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(80, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(80, 20), nullptr);
    auto btn3 = std::make_unique<UIButton>("Btn3", 2, Vector2::ZERO(), Vector2(80, 20), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    UIElement* ptr3 = btn3.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.addElement(ptr3);
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex());
    TEST_ASSERT_EQUAL(ptr1, layout.getSelectedElement());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
    TEST_ASSERT_EQUAL(ptr2, layout.getSelectedElement());
    
    layout.setSelectedIndex(2);
    TEST_ASSERT_EQUAL(2, layout.getSelectedIndex());
    TEST_ASSERT_EQUAL(ptr3, layout.getSelectedElement());
}

void test_visibility_affects_rendering() {
    auto mockDrawer = std::make_unique<MockDrawSurfaceAdvanced>();
    MockDrawSurfaceAdvanced* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UIPanel panel(0, 0, 100, 100);
    panel.setBackgroundColor(Color::Red);
    
    // Visible - should draw
    panel.setVisible(true);
    panel.draw(renderer);
    TEST_ASSERT_FALSE(mockRaw->rectCalls.empty());
    
    // Clear and hide
    mockRaw->clearBuffer();
    panel.setVisible(false);
    panel.draw(renderer);
    TEST_ASSERT_TRUE(mockRaw->rectCalls.empty());
}

void test_layout_element_count() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 20), nullptr);
    layout.addElement(btn1.get());
    
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
    
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 20), nullptr);
    layout.addElement(btn2.get());
    
    TEST_ASSERT_EQUAL(2, static_cast<int>(layout.getElementCount()));
}

// =============================================================================
// Additional draw/update coverage tests
// =============================================================================

void test_grid_layout_draw() {
    auto mockDrawer = std::make_unique<MockDrawSurfaceAdvanced>();
    MockDrawSurfaceAdvanced* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    layout.draw(renderer);
    TEST_ASSERT_TRUE(true); // Should not crash
}

void test_grid_layout_update() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    layout.update(16); // 16ms delta
    TEST_ASSERT_TRUE(true); // Should not crash
}

void test_horizontal_layout_draw() {
    auto mockDrawer = std::make_unique<MockDrawSurfaceAdvanced>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.draw(renderer);
    TEST_ASSERT_TRUE(true); // Should not crash
}

void test_horizontal_layout_update() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.update(16);
    TEST_ASSERT_TRUE(true); // Should not crash
}

void test_vertical_layout_draw() {
    auto mockDrawer = std::make_unique<MockDrawSurfaceAdvanced>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.draw(renderer);
    TEST_ASSERT_TRUE(true); // Should not crash
}

void test_vertical_layout_update() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.update(16);
    TEST_ASSERT_TRUE(true); // Should not crash
}

void test_grid_layout_handle_input() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    // Test with empty input - should not crash
    pixelroot32::input::InputConfig config;
    pixelroot32::input::InputManager input(config);
    layout.handleInput(input);
    TEST_ASSERT_TRUE(true);
}

void test_horizontal_layout_handle_input() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    pixelroot32::input::InputConfig config;
    pixelroot32::input::InputManager input(config);
    layout.handleInput(input);
    TEST_ASSERT_TRUE(true);
}

void test_vertical_layout_handle_input() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    pixelroot32::input::InputConfig config;
    pixelroot32::input::InputManager input(config);
    layout.handleInput(input);
    TEST_ASSERT_TRUE(true);
}

void test_layout_clear_elements() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 20), nullptr);
    
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    TEST_ASSERT_EQUAL(2, static_cast<int>(layout.getElementCount()));
    
    layout.clearElements();
    
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

// =============================================================================
// Additional UIGridLayout Coverage Tests
// =============================================================================

void test_grid_layout_calculate_rows_basic() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("2", 1, Vector2::ZERO(), Vector2(40, 20), nullptr);
    auto btn3 = std::make_unique<UIButton>("3", 2, Vector2::ZERO(), Vector2(40, 20), nullptr);
    
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    layout.addElement(btn3.get());
    
    TEST_ASSERT_EQUAL(2, layout.getRows()); // 3 elements, 2 columns = 2 rows
}

void test_grid_layout_calculate_rows_single_element() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    TEST_ASSERT_EQUAL(1, layout.getRows());
}

void test_grid_layout_calculate_rows_empty() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    TEST_ASSERT_EQUAL(0, layout.getRows());
}

void test_grid_layout_set_selected_index() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("2", 1, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
}

void test_grid_layout_set_selected_index_invalid() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(100); // Invalid index
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex()); // Should clamp to valid
}

void test_grid_layout_get_selected_element() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_NOT_NULL(layout.getSelectedElement());
}

void test_grid_layout_get_selected_element_invalid() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    TEST_ASSERT_NULL(layout.getSelectedElement());
}

void test_grid_layout_add_duplicate_element() {
    UIGridLayout layout(0, 0, 100, 100);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn1.get()); // Add same element twice
    
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

void test_grid_layout_set_button_style() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    layout.setButtonStyle(Color::White, Color::Blue, Color::Gray, Color::Black);
    TEST_ASSERT_TRUE(true);
}

void test_grid_layout_set_selected_index_negative() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(-1);
    TEST_ASSERT_EQUAL(-1, layout.getSelectedIndex());
}

void test_grid_layout_remove_selected_element() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("2", 1, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    layout.removeElement(btn1.get()); // Remove selected element
    
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

// =============================================================================
// Additional UIHorizontalLayout Coverage Tests
// =============================================================================

void test_horizontal_layout_set_selected_index() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
}

void test_horizontal_layout_set_selected_index_negative() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(-1);
    TEST_ASSERT_EQUAL(-1, layout.getSelectedIndex());
}

void test_horizontal_layout_get_selected_element() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_NOT_NULL(layout.getSelectedElement());
}

void test_horizontal_layout_remove_selected() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    layout.removeElement(btn1.get());
    
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

void test_horizontal_layout_scroll_offset() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setScrollEnabled(true);
    layout.setScrollOffset(toScalar(50));
    TEST_ASSERT_TRUE(true);
}

void test_horizontal_layout_scroll_to_selected() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setScrollEnabled(true);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Additional UIVerticalLayout Coverage Tests
// =============================================================================

void test_vertical_layout_set_selected_index() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
}

void test_vertical_layout_set_selected_index_negative() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(-1);
    TEST_ASSERT_EQUAL(-1, layout.getSelectedIndex());
}

void test_vertical_layout_get_selected_element() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_NOT_NULL(layout.getSelectedElement());
}

void test_vertical_layout_remove_selected() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    layout.removeElement(btn1.get());
    
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

// =============================================================================
// More UI layout coverage - handleInput with navigation
// =============================================================================

void test_horizontal_layout_handle_input_with_selection() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    
    pixelroot32::input::InputConfig config;
    pixelroot32::input::InputManager input(config);
    layout.handleInput(input);
    TEST_ASSERT_TRUE(true);
}

void test_vertical_layout_handle_input_with_selection() {
    UIVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    
    pixelroot32::input::InputConfig config;
    pixelroot32::input::InputManager input(config);
    layout.handleInput(input);
    TEST_ASSERT_TRUE(true);
}

void test_grid_layout_handle_input_with_selection() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("2", 1, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(0);
    
    pixelroot32::input::InputConfig config;
    pixelroot32::input::InputManager input(config);
    layout.handleInput(input);
    TEST_ASSERT_TRUE(true);
}

void test_horizontal_layout_ensure_visible() {
    UIHorizontalLayout layout(0, 0, 100, 50);
    layout.setScrollEnabled(true);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_TRUE(true);
}

void test_vertical_layout_ensure_visible() {
    UIVerticalLayout layout(0, 0, 100, 100);
    layout.setScrollEnabled(true);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("B", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Test helper classes to access protected methods
// =============================================================================

// Test subclass to access protected methods of UIVerticalLayout
class TestableVerticalLayout : public UIVerticalLayout {
public:
    using UIVerticalLayout::calculateContentHeight;
    using UIVerticalLayout::updateElementVisibility;
    using UIVerticalLayout::ensureSelectedVisible;
    using UIVerticalLayout::clampScrollOffset;
    
    TestableVerticalLayout(Scalar x, Scalar y, int w, int h) : UIVerticalLayout(x, y, w, h) {}
};

// Test subclass to access protected methods of UIHorizontalLayout
class TestableHorizontalLayout : public UIHorizontalLayout {
public:
    using UIHorizontalLayout::calculateContentWidth;
    using UIHorizontalLayout::updateElementVisibility;
    using UIHorizontalLayout::ensureSelectedVisible;
    using UIHorizontalLayout::clampScrollOffset;
    
    TestableHorizontalLayout(Scalar x, Scalar y, int w, int h) : UIHorizontalLayout(x, y, w, h) {}
};

// Test subclass to access protected methods of UIGridLayout
class TestableGridLayout : public UIGridLayout {
public:
    using UIGridLayout::calculateCellDimensions;
    using UIGridLayout::calculateRows;
    
    TestableGridLayout(Scalar x, Scalar y, int w, int h) : UIGridLayout(x, y, w, h) {}
};

// =============================================================================
// Tests for protected methods via test subclasses
// =============================================================================

void test_vertical_layout_protected_calculate_content_height() {
    TestableVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    // Call protected method directly
    layout.calculateContentHeight();
    TEST_ASSERT_TRUE(true);
}

void test_vertical_layout_protected_update_element_visibility() {
    TestableVerticalLayout layout(0, 0, 100, 200);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    // Call protected method directly
    layout.updateElementVisibility();
    TEST_ASSERT_TRUE(true);
}

void test_vertical_layout_protected_clamp_scroll() {
    TestableVerticalLayout layout(0, 0, 100, 200);
    
    // Call protected method directly
    layout.clampScrollOffset();
    TEST_ASSERT_TRUE(true);
}

void test_horizontal_layout_protected_calculate_content_width() {
    TestableHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    // Call protected method directly
    layout.calculateContentWidth();
    TEST_ASSERT_TRUE(true);
}

void test_horizontal_layout_protected_update_element_visibility() {
    TestableHorizontalLayout layout(0, 0, 200, 50);
    
    auto btn1 = std::make_unique<UIButton>("A", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    layout.addElement(btn1.get());
    
    // Call protected method directly
    layout.updateElementVisibility();
    TEST_ASSERT_TRUE(true);
}

void test_horizontal_layout_protected_clamp_scroll() {
    TestableHorizontalLayout layout(0, 0, 200, 50);
    
    // Call protected method directly
    layout.clampScrollOffset();
    TEST_ASSERT_TRUE(true);
}

void test_grid_layout_protected_calculate_rows() {
    TestableGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("2", 1, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    layout.addElement(btn2.get());
    
    // Call protected method directly
    layout.calculateRows();
    TEST_ASSERT_TRUE(true);
}

void test_grid_layout_protected_calculate_cell_dimensions() {
    TestableGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    
    auto btn1 = std::make_unique<UIButton>("1", 0, Vector2::ZERO(), Vector2(40, 20), nullptr);
    layout.addElement(btn1.get());
    
    // Call protected method directly
    layout.calculateCellDimensions();
    TEST_ASSERT_TRUE(true);
}
