#include <unity.h>
#include "input/TouchStateMachine.h"
#include "input/TouchEventQueue.h"
#include "input/TouchEvent.h"

using namespace pixelroot32::input;

// Helper function to create a touch state machine and test state transitions
static uint32_t current_timestamp = 0;

void setUp(void) {
    current_timestamp = 0;
}

void tearDown(void) {
}

// ============================================================================
// Test: Initial state is Idle
// ============================================================================
void test_initial_state_is_idle(void) {
    TouchStateMachine sm;
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::Idle, state);
    
    TEST_ASSERT_FALSE(sm.isActive());
}

// ============================================================================
// Test: Idle → Pressed (touch down)
// ============================================================================
void test_idle_to_pressed_on_touch_down(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Touch down at t=0
    sm.update(0, true, 100, 200, 0, queue);
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::Pressed, state);
    TEST_ASSERT_TRUE(sm.isActive());
    
    // Should have generated TouchDown event
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TouchEvent event;
    queue.dequeue(event);
    TEST_ASSERT_EQUAL(TouchEventType::TouchDown, event.getType());
    TEST_ASSERT_EQUAL(0, event.id);
    TEST_ASSERT_EQUAL(100, event.x);
    TEST_ASSERT_EQUAL(200, event.y);
}

// ============================================================================
// Test: Pressed → Idle (touch up → Click)
// ============================================================================
void test_pressed_to_idle_generates_click(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press at t=0
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Release at t=150 (within click duration)
    sm.update(0, false, 100, 200, 150, queue);
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::Idle, state);
    TEST_ASSERT_FALSE(sm.isActive());
    
    // Should have generated TouchUp and Click events
    TEST_ASSERT_EQUAL(2, queue.getCount());
    
    TouchEvent events[2];
    queue.getEvents(events, 2);
    
    TEST_ASSERT_EQUAL(TouchEventType::TouchUp, events[0].getType());
    TEST_ASSERT_EQUAL(TouchEventType::Click, events[1].getType());
}

// ============================================================================
// Test: LongPress detection (Pressed → LongPress)
// ============================================================================
void test_longpress_detection(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press at t=0
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Update at t=800 (exactly at threshold)
    sm.update(0, true, 100, 200, 800, queue);
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::LongPress, state);
    
    // Should have generated LongPress event
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TouchEvent event;
    queue.dequeue(event);
    TEST_ASSERT_EQUAL(TouchEventType::LongPress, event.getType());
}

// ============================================================================
// Test: LongPress → Idle (release after long press)
// ============================================================================
void test_longpress_to_idle_on_release(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press at t=0
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Wait for long press
    sm.update(0, true, 100, 200, 800, queue);
    queue.clear();
    
    // Release at t=1000
    sm.update(0, false, 100, 200, 1000, queue);
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::Idle, state);
    
    // Should have TouchUp only (no Click since we had LongPress)
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TouchEvent event;
    queue.dequeue(event);
    TEST_ASSERT_EQUAL(TouchEventType::TouchUp, event.getType());
}

// ============================================================================
// Test: DragStart detection (Pressed → Dragging)
// ============================================================================
void test_dragstart_detection(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press at t=0
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Move beyond drag threshold (10+ pixels)
    sm.update(0, true, 120, 220, 100, queue);
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::Dragging, state);
    
    // Should have generated DragStart event
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TouchEvent event;
    queue.dequeue(event);
    TEST_ASSERT_EQUAL(TouchEventType::DragStart, event.getType());
}

// ============================================================================
// Test: Dragging → DragEnd on release
// ============================================================================
void test_dragend_on_release(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Start drag
    sm.update(0, true, 120, 220, 100, queue);
    queue.clear();
    
    // Release while dragging
    sm.update(0, false, 150, 250, 200, queue);
    
    TouchState state = sm.getState(0);
    TEST_ASSERT_EQUAL(TouchState::Idle, state);
    
    // Should have DragEnd event
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TouchEvent event;
    queue.dequeue(event);
    TEST_ASSERT_EQUAL(TouchEventType::DragEnd, event.getType());
}

// ============================================================================
// Test: DoubleClick detection
// ============================================================================
void test_doubleclick_detection(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // First click
    sm.update(0, true, 100, 200, 0, queue);
    sm.update(0, false, 100, 200, 100, queue);
    queue.clear();
    
    // Second click within interval (t=400 < 400)
    sm.update(0, true, 100, 200, 350, queue);
    sm.update(0, false, 100, 200, 450, queue);
    
    // Should have DoubleClick instead of Click
    TEST_ASSERT_TRUE(queue.hasEvents());
    
    TouchEvent events[4];
    uint8_t count = queue.getEvents(events, 4);
    
    // Find Click or DoubleClick
    bool hasDoubleClick = false;
    for (uint8_t i = 0; i < count; i++) {
        if (events[i].getType() == TouchEventType::DoubleClick) {
            hasDoubleClick = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasDoubleClick);
}

// ============================================================================
// Test: Multiple touch IDs work independently
// ============================================================================
void test_multiple_touch_ids_independent(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Touch 0 pressed
    sm.update(0, true, 100, 100, 0, queue);
    
    // Touch 1 pressed
    sm.update(1, true, 200, 200, 50, queue);
    
    // Both should be active
    TEST_ASSERT_TRUE(sm.isActive());
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(0));
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(1));
    
    // Release touch 0
    sm.update(0, false, 100, 100, 100, queue);
    
    // Touch 0 should be idle, touch 1 still pressed
    TEST_ASSERT_EQUAL(TouchState::Idle, sm.getState(0));
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(1));
    TEST_ASSERT_TRUE(sm.isActive());  // Touch 1 still active
}

// ============================================================================
// Test: Reset single touch
// ============================================================================
void test_reset_single_touch(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press touch
    sm.update(0, true, 100, 200, 0, queue);
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(0));
    
    // Reset touch 0
    sm.reset(0);
    
    TEST_ASSERT_EQUAL(TouchState::Idle, sm.getState(0));
    TEST_ASSERT_FALSE(sm.isActive());
}

// ============================================================================
// Test: Reset all touches
// ============================================================================
void test_reset_all_touches(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press multiple touches
    sm.update(0, true, 100, 100, 0, queue);
    sm.update(1, true, 200, 200, 0, queue);
    sm.update(2, true, 300, 300, 0, queue);
    
    TEST_ASSERT_TRUE(sm.isActive());
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(0));
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(1));
    TEST_ASSERT_EQUAL(TouchState::Pressed, sm.getState(2));
    
    // Reset all
    sm.resetAll();
    
    TEST_ASSERT_FALSE(sm.isActive());
    TEST_ASSERT_EQUAL(TouchState::Idle, sm.getState(0));
    TEST_ASSERT_EQUAL(TouchState::Idle, sm.getState(1));
    TEST_ASSERT_EQUAL(TouchState::Idle, sm.getState(2));
}

// ============================================================================
// Test: Get press duration
// ============================================================================
void test_get_press_duration(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press at t=0
    sm.update(0, true, 100, 200, 0, queue);

    // Check duration at t=500
    uint32_t duration = sm.getPressDuration(0, 500);
    TEST_ASSERT_EQUAL(500, duration);

    // Release
    sm.update(0, false, 100, 200, 500, queue);

    // Duration should be 0 after release
    duration = sm.getPressDuration(0, 500);
    TEST_ASSERT_EQUAL(0, duration);
}

// ============================================================================
// Test: DragMove events while dragging
// ============================================================================
void test_dragmove_events(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Start drag
    sm.update(0, true, 120, 220, 100, queue);
    queue.clear();
    
    // Continue dragging
    sm.update(0, true, 140, 240, 200, queue);
    sm.update(0, true, 160, 260, 300, queue);
    
    // Should have DragMove events
    TEST_ASSERT_TRUE(queue.hasEvents());
    
    TouchEvent events[4];
    uint8_t count = queue.getEvents(events, 4);
    
    bool hasDragMove = false;
    for (uint8_t i = 0; i < count; i++) {
        if (events[i].getType() == TouchEventType::DragMove) {
            hasDragMove = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasDragMove);
}

// ============================================================================
// Test: Click too long (no click event)
// ============================================================================
void test_click_too_long_no_click(void) {
    TouchStateMachine sm;
    TouchEventQueue queue;
    
    // Press at t=0
    sm.update(0, true, 100, 200, 0, queue);
    queue.clear();
    
    // Release at t=500 (beyond CLICK_MAX_DURATION of 300ms)
    sm.update(0, false, 100, 200, 500, queue);
    
    // Should have TouchUp but no Click
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TouchEvent event;
    queue.dequeue(event);
    TEST_ASSERT_EQUAL(TouchEventType::TouchUp, event.getType());
    
    // State should be idle
    TEST_ASSERT_EQUAL(TouchState::Idle, sm.getState(0));
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_is_idle);
    RUN_TEST(test_idle_to_pressed_on_touch_down);
    RUN_TEST(test_pressed_to_idle_generates_click);
    RUN_TEST(test_longpress_detection);
    RUN_TEST(test_longpress_to_idle_on_release);
    RUN_TEST(test_dragstart_detection);
    RUN_TEST(test_dragend_on_release);
    RUN_TEST(test_doubleclick_detection);
    RUN_TEST(test_multiple_touch_ids_independent);
    RUN_TEST(test_reset_single_touch);
    RUN_TEST(test_reset_all_touches);
    RUN_TEST(test_get_press_duration);
    RUN_TEST(test_dragmove_events);
    RUN_TEST(test_click_too_long_no_click);
    return UNITY_END();
}