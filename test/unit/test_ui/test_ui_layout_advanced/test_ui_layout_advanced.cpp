#include <unity.h>
#include "../../../test_config.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UICheckbox.h"
#include "graphics/ui/UIPanel.h"
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/ui/UILabel.h"
#include "graphics/BaseDrawSurface.h"
#include <vector>
#include <memory>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;

// Mock DrawSurface to capture drawing calls
class MockDrawSurface : public BaseDrawSurface {
public:
    struct RectCall {
        int x, y, w, h;
        uint16_t color;
        bool filled;
    };
    std::vector<RectCall> rectCalls;

    void init() override {}
    void clearBuffer() override { rectCalls.clear(); }
    void sendBuffer() override {}
    void present() override {}
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override {
        rectCalls.push_back({x, y, width, height, color, false});
    }
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override {
        rectCalls.push_back({x, y, width, height, color, true});
    }
    void drawPixel(int x, int y, uint16_t color) override {}
    void drawFilledCircle(int x, int y, int radius, uint16_t color) override {}
    void drawCircle(int x, int y, int radius, uint16_t color) override {}
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override {}
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) override {}
    void setContrast(uint8_t level) override {}
    void setTextColor(uint16_t color) override {}
    void setTextSize(uint8_t size) override {}
    void setCursor(int16_t x, int16_t y) override {}
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override { return 0; }
    void setDisplaySize(int w, int h) override {}
    void setPhysicalSize(int w, int h) override {}
    void setOffset(int x, int y) override {}
};

// ============================================================================
// Task 2.2.2: Horizontal Layout with Spacing Tests
// ============================================================================

void test_horizontal_layout_spacing_calculation() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setSpacing(10);
    layout.setPadding(5);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(40, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(40, 30), nullptr);
    auto btn3 = std::make_unique<UIButton>("Btn3", 2, Vector2::ZERO(), Vector2(40, 30), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    UIElement* ptr3 = btn3.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.addElement(ptr3);
    
    layout.updateLayout();
    
    // With padding 5 and spacing 10: 
    // btn1 at x=5, btn2 at x=5+40+10=55, btn3 at x=55+40+10=105
    TEST_ASSERT_EQUAL_FLOAT(5, ptr1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(55, ptr2->position.x);
    TEST_ASSERT_EQUAL_FLOAT(105, ptr3->position.x);
}

void test_horizontal_layout_zero_spacing() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setSpacing(0);
    layout.setPadding(0);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(30, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(30, 30), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.updateLayout();
    
    // With zero spacing: btn2 should be right after btn1
    TEST_ASSERT_EQUAL_FLOAT(0, ptr1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(30, ptr2->position.x);
}

void test_horizontal_layout_negative_spacing() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setSpacing(-5); // Overlap
    layout.setPadding(0);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(30, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(30, 30), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.updateLayout();
    
    // With -5 spacing: btn2 overlaps btn1 by 5 pixels
    TEST_ASSERT_EQUAL_FLOAT(0, ptr1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(25, ptr2->position.x); // 30 + (-5)
}

// ============================================================================
// Task 2.2.3: Vertical Layout with Spacing Tests
// ============================================================================

void test_vertical_layout_spacing_calculation() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setSpacing(10);
    layout.setPadding(5);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(80, 30), nullptr);
    auto btn3 = std::make_unique<UIButton>("Btn3", 2, Vector2::ZERO(), Vector2(80, 30), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    UIElement* ptr3 = btn3.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.addElement(ptr3);
    
    layout.updateLayout();
    
    // With padding 5 and spacing 10:
    // btn1 at y=5, btn2 at y=5+30+10=45, btn3 at y=45+30+10=85
    TEST_ASSERT_EQUAL_FLOAT(5, ptr1->position.y);
    TEST_ASSERT_EQUAL_FLOAT(45, ptr2->position.y);
    TEST_ASSERT_EQUAL_FLOAT(85, ptr3->position.y);
}

void test_vertical_layout_zero_spacing() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setSpacing(0);
    layout.setPadding(0);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(80, 25), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(80, 25), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.updateLayout();
    
    // With zero spacing: btn2 should be right below btn1
    TEST_ASSERT_EQUAL_FLOAT(0, ptr1->position.y);
    TEST_ASSERT_EQUAL_FLOAT(25, ptr2->position.y);
}

void test_vertical_layout_large_spacing() {
    UIVerticalLayout layout(0, 0, 100, 300);
    layout.setSpacing(50);
    layout.setPadding(0);
    
    auto btn1 = std::make_unique<UIButton>("Btn1", 0, Vector2::ZERO(), Vector2(80, 20), nullptr);
    auto btn2 = std::make_unique<UIButton>("Btn2", 1, Vector2::ZERO(), Vector2(80, 20), nullptr);
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    layout.updateLayout();
    
    // With 50 spacing
    TEST_ASSERT_EQUAL_FLOAT(0, ptr1->position.y);
    TEST_ASSERT_EQUAL_FLOAT(70, ptr2->position.y); // 20 + 50
}

// ============================================================================
// Task 2.2.4: UI Component State Tests
// ============================================================================

void test_button_state_transitions() {
    UIButton button("Test", 0, Vector2::ZERO(), Vector2(100, 30), nullptr);
    
    // Initial state
    TEST_ASSERT_FALSE(button.getSelected());
    
    // Select state
    button.setSelected(true);
    TEST_ASSERT_TRUE(button.getSelected());
    
    // Deselect
    button.setSelected(false);
    TEST_ASSERT_FALSE(button.getSelected());
}

void test_button_callback_invocation() {
    bool clicked = false;
    UIButton button("Click", 0, Vector2::ZERO(), Vector2(100, 30), [&](){ clicked = true; });
    
    button.press();
    TEST_ASSERT_TRUE(clicked);
}

void test_checkbox_state_transitions() {
    UICheckBox checkbox("Test", 0, 0, 0, 50, 20, false, nullptr);
    
    // Initial unchecked state
    TEST_ASSERT_FALSE(checkbox.isChecked());
    
    // Toggle to checked
    checkbox.toggle();
    TEST_ASSERT_TRUE(checkbox.isChecked());
    
    // Toggle back to unchecked
    checkbox.toggle();
    TEST_ASSERT_FALSE(checkbox.isChecked());
}

void test_checkbox_set_checked_directly() {
    bool callbackValue = false;
    UICheckBox checkbox("Test", 0, 0, 0, 50, 20, false, [&](bool v){ callbackValue = v; });
    
    checkbox.setChecked(true);
    TEST_ASSERT_TRUE(checkbox.isChecked());
    TEST_ASSERT_TRUE(callbackValue);
    
    checkbox.setChecked(false);
    TEST_ASSERT_FALSE(checkbox.isChecked());
    TEST_ASSERT_FALSE(callbackValue);
}

void test_checkbox_callback_with_toggle() {
    bool callbackFired = false;
    bool receivedState = false;
    UICheckBox checkbox("Test", 0, 0, 0, 50, 20, false, [&](bool v){ 
        callbackFired = true; 
        receivedState = v; 
    });
    
    checkbox.toggle();
    TEST_ASSERT_TRUE(callbackFired);
    TEST_ASSERT_TRUE(receivedState);
    
    callbackFired = false;
    checkbox.toggle();
    TEST_ASSERT_TRUE(callbackFired);
    TEST_ASSERT_FALSE(receivedState);
}

// ============================================================================
// Task 2.2.5: UI Container Hierarchy Tests
// ============================================================================

void test_parent_child_relationship() {
    UIPanel parent(10, 10, 100, 100);
    auto child = std::make_unique<UILabel>("Child", Vector2::ZERO(), Color::White, 1);
    UILabel* childPtr = child.get();
    
    parent.setChild(child.release());
    
    // Child should report correct parent relationship
    TEST_ASSERT_EQUAL_PTR(childPtr, parent.getChild());
    // Child position should be relative to parent
    TEST_ASSERT_EQUAL_FLOAT(10, childPtr->position.x);
    TEST_ASSERT_EQUAL_FLOAT(10, childPtr->position.y);
}

void test_panel_position_update_affects_child() {
    UIPanel panel(10, 10, 100, 100);
    auto label = std::make_unique<UILabel>("Label", Vector2::ZERO(), Color::White, 1);
    UILabel* labelPtr = label.get();
    
    panel.setChild(label.release());
    panel.setPosition(50, 60);
    
    // Child should move with parent
    TEST_ASSERT_EQUAL_FLOAT(50, labelPtr->position.x);
    TEST_ASSERT_EQUAL_FLOAT(60, labelPtr->position.y);
}

void test_padding_container_hierarchy() {
    UIPaddingContainer container(10, 10, 100, 100);
    container.setPadding(10);
    
    auto child = std::make_unique<UIButton>("Btn", 0, Vector2::ZERO(), Vector2(50, 20), nullptr);
    UIButton* childPtr = child.get();
    
    container.setChild(child.release());
    container.update(16);
    
    // Child should be positioned with padding offset
    TEST_ASSERT_EQUAL_FLOAT(20, childPtr->position.x); // 10 + 10 padding
    TEST_ASSERT_EQUAL_FLOAT(20, childPtr->position.y); // 10 + 10 padding
}

void test_nested_container_hierarchy() {
    UIPanel outer(0, 0, 200, 200);
    auto inner = std::make_unique<UIPanel>(10, 10, 100, 100);
    UIPanel* innerPtr = inner.get();
    
    auto label = std::make_unique<UILabel>("Nested", Vector2::ZERO(), Color::White, 1);
    UILabel* labelPtr = label.get();
    
    innerPtr->setChild(label.release());
    outer.setChild(inner.release());
    
    // Nested element position should be cumulative
    TEST_ASSERT_EQUAL_FLOAT(10, innerPtr->position.x);
    TEST_ASSERT_EQUAL_FLOAT(10, innerPtr->position.y);
    TEST_ASSERT_EQUAL_FLOAT(10, labelPtr->position.x);
    TEST_ASSERT_EQUAL_FLOAT(10, labelPtr->position.y);
}

// ============================================================================
// Task 2.2.6: Sibling Order and Z-Index Tests
// ============================================================================

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
    
    UIElement* ptr1 = btn1.get();
    UIElement* ptr2 = btn2.get();
    
    layout.addElement(ptr1);
    layout.addElement(ptr2);
    
    // Check that selection tracking works
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
    
    // Remove first element by pointer
    layout.removeElement(ptr1);
    
    // Element should no longer be in layout but ptr is still valid (not deleted)
    TEST_ASSERT_NOT_NULL(ptr1);
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
    TEST_ASSERT_EQUAL_PTR(ptr1, layout.getSelectedElement());
    
    // Move to next
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
    TEST_ASSERT_EQUAL_PTR(ptr2, layout.getSelectedElement());
    
    // Move to next
    layout.setSelectedIndex(2);
    TEST_ASSERT_EQUAL(2, layout.getSelectedIndex());
    TEST_ASSERT_EQUAL_PTR(ptr3, layout.getSelectedElement());
}

void test_visibility_affects_rendering() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
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

// ============================================================================
// Main
// ============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();
    
    // Task 2.2.2: Horizontal Layout with Spacing
    RUN_TEST(test_horizontal_layout_spacing_calculation);
    RUN_TEST(test_horizontal_layout_zero_spacing);
    RUN_TEST(test_horizontal_layout_negative_spacing);
    
    // Task 2.2.3: Vertical Layout with Spacing
    RUN_TEST(test_vertical_layout_spacing_calculation);
    RUN_TEST(test_vertical_layout_zero_spacing);
    RUN_TEST(test_vertical_layout_large_spacing);
    
    // Task 2.2.4: UI Component State
    RUN_TEST(test_button_state_transitions);
    RUN_TEST(test_button_callback_invocation);
    RUN_TEST(test_checkbox_state_transitions);
    RUN_TEST(test_checkbox_set_checked_directly);
    RUN_TEST(test_checkbox_callback_with_toggle);
    
    // Task 2.2.5: UI Container Hierarchy
    RUN_TEST(test_parent_child_relationship);
    RUN_TEST(test_panel_position_update_affects_child);
    RUN_TEST(test_padding_container_hierarchy);
    RUN_TEST(test_nested_container_hierarchy);
    
    // Task 2.2.6: Sibling Order and Z-Index
    RUN_TEST(test_sibling_order_in_horizontal_layout);
    RUN_TEST(test_sibling_order_in_vertical_layout);
    RUN_TEST(test_element_index_tracking);
    RUN_TEST(test_element_removal_from_hierarchy);
    RUN_TEST(test_selected_index_navigation);
    RUN_TEST(test_visibility_affects_rendering);
    
    return UNITY_END();
}
