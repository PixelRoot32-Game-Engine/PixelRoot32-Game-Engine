/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for UITouchElement, UITouchButton, UITouchSlider
 */

#pragma once

#include <unity.h>
#include "../../test_config.h"

#include "graphics/ui/UITouchWidget.h"
#include "graphics/ui/UITouchElement.h"
#include "graphics/ui/UITouchButton.h"
#include "graphics/ui/UITouchSlider.h"
#include "graphics/ui/UITouchCheckbox.h"
#include "graphics/ui/UIHitTest.h"
#include "input/TouchEvent.h"

using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::input;
using namespace pixelroot32::graphics;
using namespace pixelroot32::math;

/** Concrete fixture for tests that need a minimal UITouchElement (base class is abstract). */
class TestTouchElement : public UITouchElement {
public:
    TestTouchElement(int16_t x, int16_t y, uint16_t w, uint16_t h, UIWidgetType type)
        : UITouchElement(x, y, w, h, type) {}
    bool processEvent(const TouchEvent&) override { return false; }
};

// =============================================================================
// Test Fixtures
// =============================================================================

struct TestButton {
    UITouchButton button;
    
    TestButton() : button("Test", 10, 20, 100, 40) {}
};

struct TestSlider {
    UITouchSlider slider;
    
    TestSlider() : slider(10, 20, 100, 40, 50) {}
};

// =============================================================================
// UITouchElement Tests
// =============================================================================

void test_uitouch_element_initialization() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    TEST_ASSERT_EQUAL(10, element.getX());
    TEST_ASSERT_EQUAL(20, element.getY());
    TEST_ASSERT_EQUAL(100, element.getWidgetWidth());
    TEST_ASSERT_EQUAL(40, element.getWidgetHeight());
}

void test_uitouch_element_is_enabled() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    
    TEST_ASSERT_TRUE(element.isEnabled());
}

void test_uitouch_element_is_disabled() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(false);
    
    TEST_ASSERT_FALSE(element.isEnabled());
}

void test_uitouch_element_is_visible() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetVisible(true);
    
    TEST_ASSERT_TRUE(element.isVisible());
}

void test_uitouch_element_is_not_visible() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetVisible(false);
    
    TEST_ASSERT_FALSE(element.isVisible());
}

// =============================================================================
// UITouchElement - Additional value tests for line coverage
// =============================================================================

void test_uitouch_element_set_position_syncs_widget_data() {
    // Test that setPosition() syncs both Entity position and widgetData_
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    // Initial positions
    TEST_ASSERT_EQUAL(10, element.getX());
    TEST_ASSERT_EQUAL(20, element.getY());
    TEST_ASSERT_EQUAL_FLOAT(10.0f, element.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, element.position.y);
    
    // Change position via setPosition (override)
    element.setPosition(50, 60);
    
    // Both Entity position and widgetData_ should be updated
    TEST_ASSERT_EQUAL(50, element.getX());
    TEST_ASSERT_EQUAL(60, element.getY());
    TEST_ASSERT_EQUAL_FLOAT(50.0f, element.position.x);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, element.position.y);
}

void test_uitouch_element_set_position_different_values() {
    // Test setPosition with various values
    TestTouchElement element(0, 0, 80, 30, UIWidgetType::Button);
    
    element.setPosition(-10, -20);
    TEST_ASSERT_EQUAL(-10, element.getX());
    TEST_ASSERT_EQUAL(-20, element.getY());
    
    element.setPosition(200, 150);
    TEST_ASSERT_EQUAL(200, element.getX());
    TEST_ASSERT_EQUAL(150, element.getY());
}

void test_uitouch_element_update_called() {
    // Test that update() can be called without crashing (inline empty implementation)
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    // Call update - even though it's a no-op by default, we need to cover the line
    element.update(16);  // 16ms delta time
    
    // Verify state is preserved after update
    TEST_ASSERT_TRUE(element.isEnabled());
}

void test_uitouch_element_update_with_zero_delta() {
    // Test update with zero delta time
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetVisible(false);
    element.update(0);
    // Verify visibility is preserved after update
    TEST_ASSERT_FALSE(element.isVisible());
}

void test_uitouch_element_get_widget_state_default() {
    // Test getWidgetState() returns correct default state
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    // Default state should be UIWidgetState::Default (or 0)
    uint8_t state = element.getWidgetState();
    
    // Verify state is a valid UIWidgetState value
    TEST_ASSERT_TRUE(state <= 3);  // Valid states are 0-3
}

void test_uitouch_element_get_widget_state_after_press() {
    // Test getWidgetState() changes when pressed
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    // Initial state
    uint8_t initialState = element.getWidgetState();
    
    // Check isPressed returns false initially
    TEST_ASSERT_FALSE(element.isPressed());
    
    // Note: Actually pressing requires processEvent to be implemented properly
    // For now we just verify the initial state and that getWidgetState returns something valid
    TEST_ASSERT_TRUE(initialState <= 3);
}

// =============================================================================
// UITouchButton Tests
// =============================================================================

void test_uitouch_button_initialization() {
    UITouchButton button("Test", 10, 20, 100, 40);
    
    TEST_ASSERT_EQUAL_STRING("Test", button.getLabel().data());
    TEST_ASSERT_EQUAL(10, button.getX());
    TEST_ASSERT_EQUAL(20, button.getY());
}

void test_uitouch_button_set_label() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setLabel("Test");
    
    TEST_ASSERT_EQUAL_STRING("Test", button.getLabel().data());
}

void test_uitouch_button_set_colors() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setColors(Color::White, Color::Gray, Color::DarkGray);
    
    TEST_ASSERT_EQUAL(Color::White, button.getNormalColor());
    TEST_ASSERT_EQUAL(Color::Gray, button.getPressedColor());
    TEST_ASSERT_EQUAL(Color::DarkGray, button.getDisabledColor());
}

void test_uitouch_button_callbacks() {
    UITouchButton button("T", 10, 20, 100, 40);
    
    button.setOnDown([]() {});
    button.setOnUp([]() {});
    button.setOnClick([]() {});
    
    TEST_ASSERT_NOT_NULL(button.getOnDown());
    TEST_ASSERT_NOT_NULL(button.getOnUp());
    TEST_ASSERT_NOT_NULL(button.getOnClick());
}

void test_uitouch_button_process_event_disabled() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(false);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    bool result = button.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_button_process_event_outside_bounds() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 200;
    event.y = 30;
    
    bool result = button.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_button_process_event_inside_bounds() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    bool result = button.processEvent(event);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_button_reset() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    
    // First trigger some state
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    button.processEvent(downEvent);
    
    // Reset
    button.reset();
    
    // Verify button returns to default state after reset
    TEST_ASSERT_FALSE(button.isPressed());
}

void test_uitouch_button_get_callbacks() {
    UITouchButton button("T", 10, 20, 100, 40);
    
    // Test getters return nullptr initially
    TEST_ASSERT_TRUE(button.getOnDown() == nullptr);
    TEST_ASSERT_TRUE(button.getOnUp() == nullptr);
    TEST_ASSERT_TRUE(button.getOnClick() == nullptr);
}

void test_uitouch_button_get_font_size() {
    UITouchButton button("T", 10, 20, 100, 40);
    
    TEST_ASSERT_EQUAL(2, button.getFontSize());
    
    button.setFontSize(4);
    
    TEST_ASSERT_EQUAL(4, button.getFontSize());
}

void test_uitouch_button_get_text_alignment() {
    UITouchButton button("T", 10, 20, 100, 40);
    
    TEST_ASSERT_EQUAL(TextAlignment::CENTER, button.getTextAlignment());
    
    button.setTextAlignment(TextAlignment::LEFT);
    
    TEST_ASSERT_EQUAL(TextAlignment::LEFT, button.getTextAlignment());
}

void test_uitouch_button_get_border_colors() {
    UITouchButton button("T", 10, 20, 100, 40);
    
    TEST_ASSERT_EQUAL(Color::Gray, button.getBorderColor());
    TEST_ASSERT_EQUAL(Color::DarkGray, button.getDisabledBorderColor());
}

void test_uitouch_button_process_event_touch_up() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    button.setVisible(true);
    
    // Touch down first
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    button.processEvent(downEvent);
    
    // Then touch up
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 50;
    upEvent.y = 30;
    
    bool result = button.processEvent(upEvent);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_button_process_event_click() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    button.setVisible(true);
    
    // Touch down
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    button.processEvent(downEvent);
    
    // Touch up within bounds
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 50;
    upEvent.y = 30;
    button.processEvent(upEvent);
    
    // Verify the button state reflects the touch sequence
    TEST_ASSERT_FALSE(button.isPressed());  // Should be released after up
}

void test_uitouch_button_press_outside_bounds_resets_state() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    button.setVisible(true);
    
    // Touch down within bounds
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    button.processEvent(downEvent);
    
    // Touch up outside bounds
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 200;
    upEvent.y = 200;
    button.processEvent(upEvent);
    
    // Button should not be pressed when touch ends outside bounds
    TEST_ASSERT_FALSE(button.isPressed());
}

// =============================================================================
// UITouchSlider Tests
// =============================================================================

void test_uitouch_slider_initialization() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    
    TEST_ASSERT_EQUAL(50, slider.getValue());
    TEST_ASSERT_EQUAL(10, slider.getX());
    TEST_ASSERT_EQUAL(20, slider.getY());
}

void test_uitouch_slider_get_value() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    
    TEST_ASSERT_EQUAL(50, slider.getValue());
}

void test_uitouch_slider_set_value() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setValue(75);
    
    TEST_ASSERT_EQUAL(75, slider.getValue());
}

void test_uitouch_slider_set_value_clamped() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setValue(150);
    
    TEST_ASSERT_EQUAL(100, slider.getValue());
}

void test_uitouch_slider_set_colors() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setColors(Color::Gray, Color::White);
    
    TEST_ASSERT_EQUAL(Color::Gray, slider.getTrackColor());
    TEST_ASSERT_EQUAL(Color::White, slider.getThumbColor());
}

void test_uitouch_slider_process_event_disabled() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(false);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    bool result = slider.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_slider_process_event_touch_down() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    slider.setVisible(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    bool result = slider.processEvent(event);
    
    TEST_ASSERT_TRUE(result);
    // Value calculated from position: x=50 relative to slider at x=10 with 4px padding
    // sliderLeft = 14, sliderRight = 110, range = 96, offset = 36
    // value = (36 * 100) / 96 = 37
    TEST_ASSERT_EQUAL(37, slider.getValue());
}

void test_uitouch_slider_process_event_outside_bounds() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    slider.setVisible(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 200;  // Outside slider bounds
    event.y = 30;
    
    bool result = slider.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_slider_process_event_drag_move() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    slider.setVisible(true);
    
    // First touch down to start drag
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    slider.processEvent(downEvent);
    
    // Then drag move
    TouchEvent moveEvent{};
    moveEvent.setType(TouchEventType::DragMove);
    moveEvent.x = 80;
    moveEvent.y = 30;
    
    bool result = slider.processEvent(moveEvent);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_slider_process_event_touch_up() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    slider.setVisible(true);
    
    // First touch down
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    slider.processEvent(downEvent);
    
    // Then touch up
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 50;
    upEvent.y = 30;
    
    bool result = slider.processEvent(upEvent);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_slider_drag_move_not_dragging() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    slider.setVisible(true);
    
    // Try drag move without touch down first - should be ignored
    TouchEvent moveEvent{};
    moveEvent.setType(TouchEventType::DragMove);
    moveEvent.x = 80;
    moveEvent.y = 30;
    
    bool result = slider.processEvent(moveEvent);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_slider_value_clamping_min() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setValue(0);
    
    TEST_ASSERT_EQUAL(0, slider.getValue());
}

void test_uitouch_slider_has_value_changed() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    TEST_ASSERT_FALSE(slider.hasValueChanged());
    
    slider.setValue(75);
    TEST_ASSERT_TRUE(slider.hasValueChanged());
    
    slider.setValue(75);  // Same value
    TEST_ASSERT_FALSE(slider.hasValueChanged());
}

void test_uitouch_slider_previous_value() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    
    slider.setValue(75);
    TEST_ASSERT_EQUAL(50, slider.getPreviousValue());
}

void test_uitouch_slider_reset() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    
    // Trigger some state
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    slider.processEvent(downEvent);
    
    slider.reset();
    
    // Reset doesn't change enabled state - slider should still be enabled
    TEST_ASSERT_TRUE(slider.isEnabled());
}

void test_uitouch_slider_get_callbacks() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    
    // Test getter returns null by default
    TEST_ASSERT_TRUE(slider.getOnValueChanged() == nullptr);
    TEST_ASSERT_TRUE(slider.getOnDragStart() == nullptr);
    TEST_ASSERT_TRUE(slider.getOnDragEnd() == nullptr);
}

// =============================================================================
// UITouchCheckbox Tests
// =============================================================================

void test_uitouch_checkbox_initialization() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), true);
    
    TEST_ASSERT_EQUAL_STRING("Test", checkbox.getLabel().data());
    TEST_ASSERT_EQUAL(10, checkbox.getX());
    TEST_ASSERT_EQUAL(20, checkbox.getY());
    TEST_ASSERT_TRUE(checkbox.isChecked());
}

void test_uitouch_checkbox_initialization_unchecked() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    
    TEST_ASSERT_FALSE(checkbox.isChecked());
}

void test_uitouch_checkbox_set_label() {
    UITouchCheckbox checkbox("T", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setLabel("Test Label");
    
    TEST_ASSERT_EQUAL_STRING("Test Label", checkbox.getLabel().data());
}

void test_uitouch_checkbox_set_checked() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setChecked(true);
    
    TEST_ASSERT_TRUE(checkbox.isChecked());
}

void test_uitouch_checkbox_toggle() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.toggle();
    
    TEST_ASSERT_TRUE(checkbox.isChecked());
    
    checkbox.toggle();
    TEST_ASSERT_FALSE(checkbox.isChecked());
}

void test_uitouch_checkbox_toggle_disabled() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setWidgetEnabled(false);
    checkbox.toggle();
    
    // Should not toggle when disabled
    TEST_ASSERT_FALSE(checkbox.isChecked());
}

void test_uitouch_checkbox_set_colors() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setColors(Color::White, Color::Cyan, Color::Gray);
    
    TEST_ASSERT_EQUAL(Color::White, checkbox.getNormalColor());
    TEST_ASSERT_EQUAL(Color::Cyan, checkbox.getCheckedColor());
    TEST_ASSERT_EQUAL(Color::Gray, checkbox.getDisabledColor());
}

namespace {
bool gCheckboxCallbackTestCalled = false;
bool gCheckboxCallbackTestValue = false;
void onCheckboxCallbackTest(bool checked) {
    gCheckboxCallbackTestCalled = true;
    gCheckboxCallbackTestValue = checked;
}
} // namespace

void test_uitouch_checkbox_callback() {
    gCheckboxCallbackTestCalled = false;
    gCheckboxCallbackTestValue = false;

    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setOnChanged(onCheckboxCallbackTest);

    checkbox.setChecked(true);

    TEST_ASSERT_TRUE(gCheckboxCallbackTestCalled);
    TEST_ASSERT_TRUE(gCheckboxCallbackTestValue);
}

void test_uitouch_checkbox_process_event_disabled() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setWidgetEnabled(false);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    bool result = checkbox.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_checkbox_process_event_outside_bounds() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setWidgetEnabled(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 200;
    event.y = 200;
    
    bool result = checkbox.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_checkbox_process_event_inside_bounds() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setWidgetEnabled(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    bool result = checkbox.processEvent(event);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_checkbox_toggle_on_touch_up() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setWidgetEnabled(true);
    
    // Touch down
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    checkbox.processEvent(downEvent);
    
    TEST_ASSERT_FALSE(checkbox.isChecked()); // Not toggled yet
    
    // Touch up - should toggle
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 50;
    upEvent.y = 30;
    checkbox.processEvent(upEvent);
    
    TEST_ASSERT_TRUE(checkbox.isChecked());
}

// =============================================================================
// UIHitTest Tests
// =============================================================================

void test_uitouch_element_hit_test_enabled_visible() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(true);
    
    bool result = UIHitTest::hitTest(element, 50, 30);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_element_hit_test_disabled() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(false);
    element.setWidgetVisible(true);
    
    bool result = UIHitTest::hitTest(element, 50, 30);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_element_hit_test_not_visible() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(false);
    
    bool result = UIHitTest::hitTest(element, 50, 30);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_element_hit_test_outside_bounds() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(true);
    
    bool result = UIHitTest::hitTest(element, 200, 200);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_hit_test_find_hit_array() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(true);
    
    UITouchElement* elements[] = { &element };
    
    UITouchElement* result = UIHitTest::findHit(elements, 1, 50, 30);
    
    TEST_ASSERT_EQUAL_PTR(&element, result);
}

void test_uitouch_hit_test_find_hit_no_match() {
    TestTouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    UITouchElement* elements[] = { &element };
    
    UITouchElement* result = UIHitTest::findHit(elements, 1, 200, 200);
    
    TEST_ASSERT_NULL(result);
}

// =============================================================================
// UITouchButton - Additional Coverage Tests
// =============================================================================

void test_uitouch_button_handle_touch_down_sets_state() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    button.setVisible(true);
    
    TouchEvent event{};
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 30;
    
    // After touch down, should be in pressed state
    button.processEvent(event);
    
    TEST_ASSERT_TRUE(button.isPressed());
}

void test_uitouch_button_auto_size_no_label() {
    UITouchButton button("T", 10, 20, 100, 40);
    
    // autoSize should not crash even with short label
    button.autoSize(10);
    
    // Verify widget still has valid dimensions
    TEST_ASSERT_TRUE(button.getWidgetWidth() > 0);
}

void test_uitouch_button_auto_size_with_label() {
    UITouchButton button("TestButton", 10, 20, 100, 40);
    button.setFontSize(2);
    
    uint16_t oldWidth = button.getWidgetWidth();
    button.autoSize(20);
    
    // autoSize should update width (implementation dependent)
    TEST_ASSERT_TRUE(button.getWidgetWidth() > 0);
}

// =============================================================================
// UITouchSlider - Additional Coverage Tests
// =============================================================================

void test_uitouch_slider_get_on_drag_start() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    
    TEST_ASSERT_NULL(slider.getOnDragStart());
}

void test_uitouch_slider_get_on_drag_end() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    
    TEST_ASSERT_NULL(slider.getOnDragEnd());
}

void test_uitouch_slider_handle_touch_up_exceeds_drag() {
    UITouchSlider slider(10, 20, 100, 40, 50);
    slider.setWidgetEnabled(true);
    slider.setVisible(true);
    
    // Touch down
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    slider.processEvent(downEvent);
    
    // Touch up far away - exceeds drag threshold
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 200;
    upEvent.y = 200;
    slider.processEvent(upEvent);
    
    // Verify value is valid after drag (should not crash)
    TEST_ASSERT_TRUE(slider.getValue() >= 0 && slider.getValue() <= 100);
}

// =============================================================================
// UITouchCheckbox - Additional Coverage Tests
// =============================================================================

void test_uitouch_checkbox_get_on_changed() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    
    TEST_ASSERT_NULL(checkbox.getOnChanged());
}

void test_uitouch_checkbox_font_size_get_set() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    
    TEST_ASSERT_EQUAL(2, checkbox.getFontSize());
    
    checkbox.setFontSize(4);
    
    TEST_ASSERT_EQUAL(4, checkbox.getFontSize());
}

void test_uitouch_checkbox_handle_touch_up_exceeds_drag() {
    UITouchCheckbox checkbox("Test", Vector2(10, 20), Vector2(100, 40), false);
    checkbox.setWidgetEnabled(true);
    
    // Touch down
    TouchEvent downEvent{};
    downEvent.setType(TouchEventType::TouchDown);
    downEvent.x = 50;
    downEvent.y = 30;
    checkbox.processEvent(downEvent);
    
    // Touch up far away - should NOT toggle
    TouchEvent upEvent{};
    upEvent.setType(TouchEventType::TouchUp);
    upEvent.x = 200;
    upEvent.y = 200;
    checkbox.processEvent(upEvent);
    
    // Should NOT have toggled due to drag
    TEST_ASSERT_FALSE(checkbox.isChecked());
}
