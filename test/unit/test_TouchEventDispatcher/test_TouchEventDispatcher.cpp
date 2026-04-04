#include <unity.h>
#include "input/TouchEventDispatcher.h"
#include "input/TouchPoint.h"
#include "input/TouchEvent.h"
#include "input/TouchEventTypes.h"

using namespace pixelroot32::input;

void setUp(void) {
}

void tearDown(void) {
}

// ============================================================================
// Test: Initial state - no events
// ============================================================================
void test_initial_no_events(void) {
    TouchEventDispatcher dispatcher;
    
    TEST_ASSERT_FALSE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(0, dispatcher.getEventCount());
    TEST_ASSERT_FALSE(dispatcher.isTouchActive());
}

// ============================================================================
// Test: getEvents() pull-based API
// ============================================================================
void test_getEvents_pull_api(void) {
    TouchEventDispatcher dispatcher;
    
    // A 100ms tap generates TouchDown + TouchUp + Click (within CLICK_MAX_DURATION)
    dispatcher.processTouch(0, true, 100, 200, 0);    // TouchDown
    dispatcher.processTouch(0, false, 100, 200, 100); // TouchUp + Click

    TEST_ASSERT_TRUE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(3, dispatcher.getEventCount());

    // Pull events with buffer
    TouchEvent events[5];
    uint8_t count = dispatcher.getEvents(events, 5);

    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(TouchEventType::TouchDown, events[0].type);
    TEST_ASSERT_EQUAL(TouchEventType::TouchUp, events[1].type);
    TEST_ASSERT_EQUAL(TouchEventType::Click, events[2].type);

    // Queue should be empty now
    TEST_ASSERT_FALSE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(0, dispatcher.getEventCount());
}

// ============================================================================
// Test: peekEvents() without removing
// ============================================================================
void test_peekEvents_without_removing(void) {
    TouchEventDispatcher dispatcher;
    
    // A 100ms tap generates TouchDown + TouchUp + Click
    dispatcher.processTouch(0, true, 100, 200, 0);
    dispatcher.processTouch(0, false, 100, 200, 100);

    // Peek without removing
    TouchEvent peeked[5];
    uint8_t count = dispatcher.peekEvents(peeked, 5);

    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(TouchEventType::TouchDown, peeked[0].type);

    // Count should still be the same after peek
    TEST_ASSERT_EQUAL(3, dispatcher.getEventCount());
    TEST_ASSERT_TRUE(dispatcher.hasEvents());
}

// ============================================================================
// Test: hasEvents() and getEventCount()
// ============================================================================
void test_hasEvents_and_count(void) {
    TouchEventDispatcher dispatcher;
    
    TEST_ASSERT_FALSE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(0, dispatcher.getEventCount());
    
    dispatcher.processTouch(0, true, 50, 50, 0);
    
    TEST_ASSERT_TRUE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(1, dispatcher.getEventCount());
    
    // Clear events
    dispatcher.clearEvents();
    
    TEST_ASSERT_FALSE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(0, dispatcher.getEventCount());
}

// ============================================================================
// Test: Complete touch sequence - Down → hold → LongPress → Up
// ============================================================================
void test_complete_touch_sequence_longpress(void) {
    TouchEventDispatcher dispatcher;
    
    // Touch down at t=0
    dispatcher.processTouch(0, true, 100, 200, 0);
    
    // Wait for long press (800ms)
    dispatcher.processTouch(0, true, 100, 200, 800);
    
    // Check state - should be in long press
    TouchState state = dispatcher.getTouchState(0);
    TEST_ASSERT_EQUAL(TouchState::LongPress, state);
    
    // Get events
    TouchEvent events[10];
    uint8_t count = dispatcher.getEvents(events, 10);
    
    // Should have: TouchDown, LongPress, (TouchUp later)
    bool hasLongPress = false;
    for (uint8_t i = 0; i < count; i++) {
        if (events[i].getType() == TouchEventType::LongPress) {
            hasLongPress = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasLongPress);
    
    // Release at t=1000
    dispatcher.processTouch(0, false, 100, 200, 1000);
    
    // Get remaining events (should include TouchUp)
    count = dispatcher.getEvents(events, 10);
    TEST_ASSERT_TRUE(count > 0);
    
    // State should be idle now
    state = dispatcher.getTouchState(0);
    TEST_ASSERT_EQUAL(TouchState::Idle, state);
}

// ============================================================================
// Test: Single click sequence
// ============================================================================
void test_single_click_sequence(void) {
    TouchEventDispatcher dispatcher;
    
    // Press at t=0
    dispatcher.processTouch(0, true, 100, 200, 0);
    
    // Release at t=150 (within click duration)
    dispatcher.processTouch(0, false, 100, 200, 150);
    
    // Get all events
    TouchEvent events[10];
    uint8_t count = dispatcher.getEvents(events, 10);
    
    // Should have: TouchDown, TouchUp, Click
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(TouchEventType::TouchDown, events[0].type);
    TEST_ASSERT_EQUAL(TouchEventType::TouchUp, events[1].type);
    TEST_ASSERT_EQUAL(TouchEventType::Click, events[2].type);
}

// ============================================================================
// Test: Drag sequence
// ============================================================================
void test_drag_sequence(void) {
    TouchEventDispatcher dispatcher;
    
    // Press at t=0
    dispatcher.processTouch(0, true, 100, 200, 0);
    
    // Move beyond drag threshold
    dispatcher.processTouch(0, true, 120, 220, 100);
    
    // Continue dragging
    dispatcher.processTouch(0, true, 140, 240, 200);
    dispatcher.processTouch(0, true, 160, 260, 300);
    
    // Release
    dispatcher.processTouch(0, false, 160, 260, 400);
    
    // Get events
    TouchEvent events[10];
    uint8_t count = dispatcher.getEvents(events, 10);
    
    // Should have: TouchDown, DragStart, DragMove(s), DragEnd
    bool hasDragStart = false;
    bool hasDragEnd = false;
    for (uint8_t i = 0; i < count; i++) {
        if (events[i].getType() == TouchEventType::DragStart) hasDragStart = true;
        if (events[i].getType() == TouchEventType::DragEnd) hasDragEnd = true;
    }
    TEST_ASSERT_TRUE(hasDragStart);
    TEST_ASSERT_TRUE(hasDragEnd);
}

// ============================================================================
// Test: processTouchPoints with array
// ============================================================================
void test_processTouchPoints_array(void) {
    TouchEventDispatcher dispatcher;
    
    TouchPoint points[3] = {
        TouchPoint(10, 20, true,  0, 100),
        TouchPoint(30, 40, true,  1, 100),
        TouchPoint(50, 60, false, 2, 100)
    };
    
    dispatcher.processTouchPoints(points, 3, 100);
    
    // Check active state
    TEST_ASSERT_TRUE(dispatcher.isTouchActive());
    
    // Get events
    TouchEvent events[10];
    uint8_t count = dispatcher.getEvents(events, 10);
    
    // Should have events for each touch
    TEST_ASSERT_TRUE(count >= 2);  // At least TouchDown for touch 0 and 1
}

// ============================================================================
// Test: getEvents with small buffer
// ============================================================================
void test_getEvents_small_buffer(void) {
    TouchEventDispatcher dispatcher;
    
    // tap(0) = TouchDown + TouchUp + Click; Down(1) = TouchDown  →  4 events total
    dispatcher.processTouch(0, true, 100, 200, 0);
    dispatcher.processTouch(0, false, 100, 200, 100);
    dispatcher.processTouch(1, true, 50, 50, 150);

    // Buffer only holds 1
    TouchEvent buffer[1];
    uint8_t count = dispatcher.getEvents(buffer, 1);

    TEST_ASSERT_EQUAL(1, count);

    // Should have 3 remaining (TouchUp + Click + TouchDown for id=1)
    TEST_ASSERT_EQUAL(3, dispatcher.getEventCount());
}

// ============================================================================
// Test: clearEvents
// ============================================================================
void test_clearEvents(void) {
    TouchEventDispatcher dispatcher;
    
    // Add events
    dispatcher.processTouch(0, true, 100, 200, 0);
    dispatcher.processTouch(0, false, 100, 200, 100);
    
    TEST_ASSERT_TRUE(dispatcher.hasEvents());
    
    dispatcher.clearEvents();
    
    TEST_ASSERT_FALSE(dispatcher.hasEvents());
    TEST_ASSERT_EQUAL(0, dispatcher.getEventCount());
}

// ============================================================================
// Test: reset
// ============================================================================
void test_reset(void) {
    TouchEventDispatcher dispatcher;
    
    // Add events and set state
    dispatcher.processTouch(0, true, 100, 200, 0);
    dispatcher.processTouch(1, true, 50, 50, 0);
    
    TEST_ASSERT_TRUE(dispatcher.isTouchActive());
    
    dispatcher.reset();
    
    TEST_ASSERT_FALSE(dispatcher.isTouchActive());
    TEST_ASSERT_EQUAL(TouchState::Idle, dispatcher.getTouchState(0));
    TEST_ASSERT_EQUAL(TouchState::Idle, dispatcher.getTouchState(1));
    TEST_ASSERT_FALSE(dispatcher.hasEvents());
}

// ============================================================================
// Test: getTouchState
// ============================================================================
void test_getTouchState(void) {
    TouchEventDispatcher dispatcher;
    
    // Initial state
    TEST_ASSERT_EQUAL(TouchState::Idle, dispatcher.getTouchState(0));
    
    // Press
    dispatcher.processTouch(0, true, 100, 200, 0);
    TEST_ASSERT_EQUAL(TouchState::Pressed, dispatcher.getTouchState(0));
    
    // Release
    dispatcher.processTouch(0, false, 100, 200, 100);
    TEST_ASSERT_EQUAL(TouchState::Idle, dispatcher.getTouchState(0));
}

// ============================================================================
// Test: isTouchActive
// ============================================================================
void test_isTouchActive(void) {
    TouchEventDispatcher dispatcher;
    
    TEST_ASSERT_FALSE(dispatcher.isTouchActive());
    
    // Press
    dispatcher.processTouch(0, true, 100, 200, 0);
    TEST_ASSERT_TRUE(dispatcher.isTouchActive());
    
    // Release
    dispatcher.processTouch(0, false, 100, 200, 100);
    TEST_ASSERT_FALSE(dispatcher.isTouchActive());
}

// ============================================================================
// Test: Multiple touches
// ============================================================================
void test_multiple_touches(void) {
    TouchEventDispatcher dispatcher;
    
    // Press touch 0
    dispatcher.processTouch(0, true, 100, 100, 0);
    
    // Press touch 1
    dispatcher.processTouch(1, true, 200, 200, 50);
    
    TEST_ASSERT_TRUE(dispatcher.isTouchActive());
    TEST_ASSERT_EQUAL(TouchState::Pressed, dispatcher.getTouchState(0));
    TEST_ASSERT_EQUAL(TouchState::Pressed, dispatcher.getTouchState(1));
    
    // Release touch 0
    dispatcher.processTouch(0, false, 100, 100, 100);
    
    // Touch 1 still active
    TEST_ASSERT_TRUE(dispatcher.isTouchActive());
    TEST_ASSERT_EQUAL(TouchState::Idle, dispatcher.getTouchState(0));
    TEST_ASSERT_EQUAL(TouchState::Pressed, dispatcher.getTouchState(1));
    
    // Release touch 1
    dispatcher.processTouch(1, false, 200, 200, 150);
    
    TEST_ASSERT_FALSE(dispatcher.isTouchActive());
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_no_events);
    RUN_TEST(test_getEvents_pull_api);
    RUN_TEST(test_peekEvents_without_removing);
    RUN_TEST(test_hasEvents_and_count);
    RUN_TEST(test_complete_touch_sequence_longpress);
    RUN_TEST(test_single_click_sequence);
    RUN_TEST(test_drag_sequence);
    RUN_TEST(test_processTouchPoints_array);
    RUN_TEST(test_getEvents_small_buffer);
    RUN_TEST(test_clearEvents);
    RUN_TEST(test_reset);
    RUN_TEST(test_getTouchState);
    RUN_TEST(test_isTouchActive);
    RUN_TEST(test_multiple_touches);
    return UNITY_END();
}