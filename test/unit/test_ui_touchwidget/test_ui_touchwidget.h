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
