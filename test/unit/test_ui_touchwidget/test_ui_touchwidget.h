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
#include "graphics/ui/UIHitTest.h"
#include "input/TouchEvent.h"

using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::input;
using namespace pixelroot32::graphics;

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
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    TEST_ASSERT_EQUAL(10, element.getX());
    TEST_ASSERT_EQUAL(20, element.getY());
    TEST_ASSERT_EQUAL(100, element.getWidgetWidth());
    TEST_ASSERT_EQUAL(40, element.getWidgetHeight());
}

void test_uitouch_element_is_enabled() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    
    TEST_ASSERT_TRUE(element.isEnabled());
}

void test_uitouch_element_is_disabled() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(false);
    
    TEST_ASSERT_FALSE(element.isEnabled());
}

void test_uitouch_element_is_visible() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetVisible(true);
    
    TEST_ASSERT_TRUE(element.isVisible());
}

void test_uitouch_element_is_not_visible() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
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
    event.type = TouchEventType::TouchDown;
    event.x = 50;
    event.y = 30;
    
    bool result = button.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_button_process_event_outside_bounds() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    
    TouchEvent event{};
    event.type = TouchEventType::TouchDown;
    event.x = 200;
    event.y = 30;
    
    bool result = button.processEvent(event);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_button_process_event_inside_bounds() {
    UITouchButton button("T", 10, 20, 100, 40);
    button.setWidgetEnabled(true);
    
    TouchEvent event{};
    event.type = TouchEventType::TouchDown;
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
// UIHitTest Tests
// =============================================================================

void test_uitouch_element_hit_test_enabled_visible() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(true);
    
    bool result = UIHitTest::hitTest(element, 50, 30);
    
    TEST_ASSERT_TRUE(result);
}

void test_uitouch_element_hit_test_disabled() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(false);
    element.setWidgetVisible(true);
    
    bool result = UIHitTest::hitTest(element, 50, 30);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_element_hit_test_not_visible() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(false);
    
    bool result = UIHitTest::hitTest(element, 50, 30);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_element_hit_test_outside_bounds() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(true);
    
    bool result = UIHitTest::hitTest(element, 200, 200);
    
    TEST_ASSERT_FALSE(result);
}

void test_uitouch_hit_test_find_hit_array() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    element.setWidgetEnabled(true);
    element.setWidgetVisible(true);
    
    UITouchElement* elements[] = { &element };
    
    UITouchElement* result = UIHitTest::findHit(elements, 1, 50, 30);
    
    TEST_ASSERT_EQUAL_PTR(&element, result);
}

void test_uitouch_hit_test_find_hit_no_match() {
    UITouchElement element(10, 20, 100, 40, UIWidgetType::Button);
    
    UITouchElement* elements[] = { &element };
    
    UITouchElement* result = UIHitTest::findHit(elements, 1, 200, 200);
    
    TEST_ASSERT_NULL(result);
}
