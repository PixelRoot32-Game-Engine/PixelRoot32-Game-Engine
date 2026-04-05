/**
 * @file test_uimanager.cpp
 * @brief Unit tests for graphics/ui/UIManager module
 * @version 1.0
 * @date 2026-04-05
 * 
 * Tests for UIManager - touch UI element registry and event routing.
 */

#include <unity.h>
#include "../test_config.h"
#include "graphics/ui/UIManager.h"
#include "graphics/ui/UITouchElement.h"
#include "input/TouchEvent.h"

using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::input;

class MockUITouchElement : public UITouchElement {
public:
    MockUITouchElement(int16_t x, int16_t y, uint16_t w, uint16_t h)
        : UITouchElement(x, y, w, h, UIWidgetType::Button) {}
    
    bool processEvent(const TouchEvent& event) override {
        (void)event;
        return true;
    }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_uimanager_default_constructor(void) {
    UIManager manager;
    
    TEST_ASSERT_EQUAL(0, manager.getElementCount());
    TEST_ASSERT_EQUAL(16, manager.getMaxElements());
    TEST_ASSERT_FALSE(manager.isFull());
}

void test_uimanager_add_element(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    
    bool result = manager.addElement(&element);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, manager.getElementCount());
    TEST_ASSERT_FALSE(manager.isFull());
}

void test_uimanager_add_null_element(void) {
    UIManager manager;
    
    bool result = manager.addElement(nullptr);
    
    TEST_ASSERT_FALSE(result);
}

void test_uimanager_add_duplicate_element(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    
    manager.addElement(&element);
    bool result = manager.addElement(&element);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, manager.getElementCount());
}

void test_uimanager_is_full(void) {
    UIManager manager;
    
    for (int i = 0; i < 16; i++) {
        auto* elem = new MockUITouchElement(i * 10, 0, 50, 50);
        manager.addElement(elem);
    }
    
    TEST_ASSERT_TRUE(manager.isFull());
    TEST_ASSERT_EQUAL(16, manager.getElementCount());
}

void test_uimanager_find_free_slot_empty(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    auto* elem = manager.getElementAt(0);
    TEST_ASSERT_NOT_NULL(elem);
}

void test_uimanager_get_element_invalid_id(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    auto* elem = manager.getElement(200);
    TEST_ASSERT_NULL(elem);
}

void test_uimanager_remove_by_id(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    uint8_t id = element.getWidgetData().id;
    bool result = manager.removeElement(id);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, manager.getElementCount());
}

void test_uimanager_remove_by_id_not_found(void) {
    UIManager manager;
    
    bool result = manager.removeElement(200);
    
    TEST_ASSERT_FALSE(result);
}

void test_uimanager_remove_by_widget(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    bool result = manager.removeElement(&element.getWidgetData());
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, manager.getElementCount());
}

void test_uimanager_remove_by_null_widget(void) {
    UIManager manager;
    
    bool result = manager.removeElement(static_cast<UITouchWidget*>(nullptr));
    
    TEST_ASSERT_FALSE(result);
}

void test_uimanager_clear(void) {
    UIManager manager;
    MockUITouchElement element1(0, 0, 100, 50);
    MockUITouchElement element2(100, 0, 100, 50);
    manager.addElement(&element1);
    manager.addElement(&element2);
    
    TEST_ASSERT_EQUAL(2, manager.getElementCount());
    
    manager.clear();
    
    TEST_ASSERT_EQUAL(0, manager.getElementCount());
    TEST_ASSERT_FALSE(manager.isFull());
}

void test_uimanager_process_events_empty(void) {
    UIManager manager;
    TouchEvent event;
    event.setType(TouchEventType::TouchDown);
    event.x = 10;
    event.y = 10;
    
    uint8_t count = manager.processEvents(&event, 1);
    
    TEST_ASSERT_EQUAL(0, count);
}

void test_uimanager_process_touch_down_no_hit(void) {
    UIManager manager;
    MockUITouchElement element(100, 100, 50, 50);
    manager.addElement(&element);
    
    TouchEvent event;
    event.setType(TouchEventType::TouchDown);
    event.x = 10;
    event.y = 10;
    
    uint8_t count = manager.processEvents(&event, 1);
    
    TEST_ASSERT_EQUAL(0, count);
}

void test_uimanager_process_touch_down_hit(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent event;
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 25;
    
    uint8_t count = manager.processEvents(&event, 1);
    
    TEST_ASSERT_EQUAL(1, count);
}

void test_uimanager_captured_widget_after_touch_down(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent event;
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 25;
    manager.processEvents(&event, 1);
    
    auto* captured = manager.getCapturedWidget();
    TEST_ASSERT_NOT_NULL(captured);
}

void test_uimanager_capture_cleared_on_touch_up(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent down;
    down.setType(TouchEventType::TouchDown);
    down.x = 50;
    down.y = 25;
    manager.processEvents(&down, 1);
    
    TEST_ASSERT_NOT_NULL(manager.getCapturedWidget());
    
    TouchEvent up;
    up.setType(TouchEventType::TouchUp);
    up.x = 50;
    up.y = 25;
    manager.processEvents(&up, 1);
    
    TEST_ASSERT_NULL(manager.getCapturedWidget());
}

void test_uimanager_get_hover_widget(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    manager.updateHover(50, 25);
    
    auto* hover = manager.getHoverWidget();
    TEST_ASSERT_NOT_NULL(hover);
}

void test_uimanager_update_hover_no_hit(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    manager.updateHover(500, 500);
    
    TEST_ASSERT_NULL(manager.getHoverWidget());
}

void test_uimanager_clear_consume_flags(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent event;
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 25;
    manager.processEvents(&event, 1);
    
    manager.clearConsumeFlags();
    
    TEST_ASSERT_TRUE(true);
}

void test_uimanager_release_capture(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent down;
    down.setType(TouchEventType::TouchDown);
    down.x = 50;
    down.y = 25;
    manager.processEvents(&down, 1);
    
    TEST_ASSERT_NOT_NULL(manager.getCapturedWidget());
    
    manager.releaseCapture();
    
    TEST_ASSERT_NULL(manager.getCapturedWidget());
}

void test_uimanager_remove_clears_captured_widget(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent down;
    down.setType(TouchEventType::TouchDown);
    down.x = 50;
    down.y = 25;
    manager.processEvents(&down, 1);
    
    TEST_ASSERT_NOT_NULL(manager.getCapturedWidget());
    
    uint8_t id = element.getWidgetData().id;
    manager.removeElement(id);
    
    TEST_ASSERT_NULL(manager.getCapturedWidget());
}

void test_uimanager_get_elements_array(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    auto** elements = manager.getElements();
    TEST_ASSERT_NOT_NULL(elements);
    TEST_ASSERT_EQUAL(&element, elements[0]);
}

void test_uimanager_process_event_single(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent event;
    event.setType(TouchEventType::TouchDown);
    event.x = 50;
    event.y = 25;
    
    bool result = manager.processEvent(event);
    
    TEST_ASSERT_TRUE(result);
}

void test_uimanager_drag_events_routed_to_captured(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent down;
    down.setType(TouchEventType::TouchDown);
    down.x = 50;
    down.y = 25;
    manager.processEvents(&down, 1);
    
    TouchEvent drag;
    drag.setType(TouchEventType::DragMove);
    drag.x = 60;
    drag.y = 30;
    uint8_t count = manager.processEvents(&drag, 1);
    
    TEST_ASSERT_EQUAL(1, count);
}

void test_uimanager_drag_end_clears_capture(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    TouchEvent down;
    down.setType(TouchEventType::TouchDown);
    down.x = 50;
    down.y = 25;
    manager.processEvents(&down, 1);
    
    TEST_ASSERT_NOT_NULL(manager.getCapturedWidget());
    
    TouchEvent dragEnd;
    dragEnd.setType(TouchEventType::DragEnd);
    dragEnd.x = 50;
    dragEnd.y = 25;
    manager.processEvents(&dragEnd, 1);
    
    TEST_ASSERT_NULL(manager.getCapturedWidget());
}

void test_uimanager_out_of_range_index(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    auto* elem = manager.getElementAt(99);
    TEST_ASSERT_NULL(elem);
}

void test_uimanager_get_element_at(void) {
    UIManager manager;
    MockUITouchElement element(0, 0, 100, 50);
    manager.addElement(&element);
    
    auto* elem = manager.getElementAt(0);
    TEST_ASSERT_NOT_NULL(elem);
    TEST_ASSERT_EQUAL(&element, elem);
}

void test_uimanager_id_wrapping(void) {
    UIManager manager;
    
    MockUITouchElement element1(0, 0, 50, 50);
    manager.addElement(&element1);
    uint8_t id1 = element1.getWidgetData().id;
    
    manager.clear();
    
    MockUITouchElement element2(0, 0, 50, 50);
    manager.addElement(&element2);
    uint8_t id2 = element2.getWidgetData().id;
    
    TEST_ASSERT_NOT_EQUAL(id1, id2);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_uimanager_default_constructor);
    RUN_TEST(test_uimanager_add_element);
    RUN_TEST(test_uimanager_add_null_element);
    RUN_TEST(test_uimanager_add_duplicate_element);
    RUN_TEST(test_uimanager_is_full);
    RUN_TEST(test_uimanager_find_free_slot_empty);
    RUN_TEST(test_uimanager_get_element_invalid_id);
    RUN_TEST(test_uimanager_remove_by_id);
    RUN_TEST(test_uimanager_remove_by_id_not_found);
    RUN_TEST(test_uimanager_remove_by_widget);
    RUN_TEST(test_uimanager_remove_by_null_widget);
    RUN_TEST(test_uimanager_clear);
    RUN_TEST(test_uimanager_process_events_empty);
    RUN_TEST(test_uimanager_process_touch_down_no_hit);
    RUN_TEST(test_uimanager_process_touch_down_hit);
    RUN_TEST(test_uimanager_captured_widget_after_touch_down);
    RUN_TEST(test_uimanager_capture_cleared_on_touch_up);
    RUN_TEST(test_uimanager_get_hover_widget);
    RUN_TEST(test_uimanager_update_hover_no_hit);
    RUN_TEST(test_uimanager_clear_consume_flags);
    RUN_TEST(test_uimanager_release_capture);
    RUN_TEST(test_uimanager_remove_clears_captured_widget);
    RUN_TEST(test_uimanager_get_elements_array);
    RUN_TEST(test_uimanager_process_event_single);
    RUN_TEST(test_uimanager_drag_events_routed_to_captured);
    RUN_TEST(test_uimanager_drag_end_clears_capture);
    RUN_TEST(test_uimanager_out_of_range_index);
    RUN_TEST(test_uimanager_get_element_at);
    RUN_TEST(test_uimanager_id_wrapping);
    
    return UNITY_END();
}