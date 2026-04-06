#include <unity.h>
#include "input/TouchEventQueue.h"
#include <cstring>

using namespace pixelroot32::input;

// Test counter for verifying test execution
static int test_counter = 0;

void setUp(void) {
    test_counter++;
}

void tearDown(void) {}

// ============================================================================
// Test: Queue Initialization
// ============================================================================
void test_queue_initialization(void) {
    TouchEventQueue queue;
    
    TEST_ASSERT_TRUE(queue.isEmpty());
    TEST_ASSERT_FALSE(queue.isFull());
    TEST_ASSERT_EQUAL(0, queue.getCount());
    TEST_ASSERT_EQUAL(16, queue.getCapacity());
    TEST_ASSERT_FALSE(queue.hasEvents());
}

// ============================================================================
// Test: Enqueue single event
// ============================================================================
void test_enqueue_single_event(void) {
    TouchEventQueue queue;
    TouchEvent event(TouchEventType::TouchDown, 0, 100, 200, 1000);
    
    bool result = queue.enqueue(event);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(queue.isEmpty());
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TEST_ASSERT_TRUE(queue.hasEvents());
}

// ============================================================================
// Test: Dequeue single event
// ============================================================================
void test_dequeue_single_event(void) {
    TouchEventQueue queue;
    TouchEvent inEvent(TouchEventType::TouchDown, 0, 100, 200, 1000);
    TouchEvent outEvent;
    
    queue.enqueue(inEvent);
    bool result = queue.dequeue(outEvent);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(TouchEventType::TouchDown, outEvent.type);
    TEST_ASSERT_EQUAL(0, outEvent.id);
    TEST_ASSERT_EQUAL(100, outEvent.x);
    TEST_ASSERT_EQUAL(200, outEvent.y);
    TEST_ASSERT_EQUAL(1000, outEvent.timestamp);
    TEST_ASSERT_TRUE(queue.isEmpty());
}

// ============================================================================
// Test: Enqueue and Dequeue multiple events
// ============================================================================
void test_enqueue_dequeue_multiple(void) {
    TouchEventQueue queue;
    TouchEvent events[5] = {
        TouchEvent(TouchEventType::TouchDown, 0, 10, 20, 100),
        TouchEvent(TouchEventType::TouchUp, 0, 10, 20, 150),
        TouchEvent(TouchEventType::Click, 0, 10, 20, 150),
        TouchEvent(TouchEventType::LongPress, 1, 50, 60, 900),
        TouchEvent(TouchEventType::DragStart, 2, 100, 200, 500)
    };
    
    // Enqueue all events
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(queue.enqueue(events[i]));
    }
    
    TEST_ASSERT_EQUAL(5, queue.getCount());
    TEST_ASSERT_FALSE(queue.isFull());
    
    // Dequeue and verify all events
    TouchEvent outEvent;
    for (int i = 0; i < 5; i++) {
        queue.dequeue(outEvent);
        TEST_ASSERT_EQUAL(events[i].type, outEvent.type);
        TEST_ASSERT_EQUAL(events[i].id, outEvent.id);
    }
    
    TEST_ASSERT_TRUE(queue.isEmpty());
}

// ============================================================================
// Test: Queue overflow behavior - drop oldest
// ============================================================================
void test_queue_overflow_drops_oldest(void) {
    TouchEventQueue queue;
    
    // Fill the queue to capacity (16 events)
    for (int i = 0; i < 16; i++) {
        TouchEvent event(TouchEventType::TouchDown, 0, i, i, i * 100);
        TEST_ASSERT_TRUE(queue.enqueue(event));
    }
    
    TEST_ASSERT_TRUE(queue.isFull());
    TEST_ASSERT_EQUAL(16, queue.getCount());
    
    // Try to enqueue more - should fail (or drop depending on implementation)
    TouchEvent overflowEvent(TouchEventType::TouchDown, 0, 999, 999, 9999);
    bool result = queue.enqueue(overflowEvent);
    
    // Queue should still have 16 events (either reject new or drop old)
    TEST_ASSERT_EQUAL(16, queue.getCount());
}

// ============================================================================
// Test: Peek without removing
// ============================================================================
void test_peek_without_removing(void) {
    TouchEventQueue queue;
    TouchEvent inEvent(TouchEventType::Click, 1, 50, 100, 500);
    
    queue.enqueue(inEvent);
    
    TouchEvent peekedEvent;
    bool result = queue.peek(peekedEvent);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(TouchEventType::Click, peekedEvent.type);
    TEST_ASSERT_EQUAL(1, peekedEvent.id);
    TEST_ASSERT_EQUAL(50, peekedEvent.x);
    TEST_ASSERT_EQUAL(100, peekedEvent.y);
    
    // Count should not change after peek
    TEST_ASSERT_EQUAL(1, queue.getCount());
    TEST_ASSERT_FALSE(queue.isEmpty());
}

// ============================================================================
// Test: Peek on empty queue
// ============================================================================
void test_peek_empty_queue(void) {
    TouchEventQueue queue;
    TouchEvent event;
    
    bool result = queue.peek(event);
    
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Test: Peek multiple events
// ============================================================================
void test_peek_multiple(void) {
    TouchEventQueue queue;
    TouchEvent events[3] = {
        TouchEvent(TouchEventType::TouchDown, 0, 10, 20, 100),
        TouchEvent(TouchEventType::TouchUp, 0, 10, 20, 150),
        TouchEvent(TouchEventType::Click, 0, 10, 20, 150)
    };
    
    for (int i = 0; i < 3; i++) {
        queue.enqueue(events[i]);
    }
    
    TouchEvent peeked[5];
    uint8_t count = queue.peekMultiple(peeked, 5);
    
    TEST_ASSERT_EQUAL(3, count);
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL(events[i].type, peeked[i].type);
    }
    
    // Count should not change
    TEST_ASSERT_EQUAL(3, queue.getCount());
}

// ============================================================================
// Test: Clear operation
// ============================================================================
void test_clear_operation(void) {
    TouchEventQueue queue;
    
    // Add some events
    for (int i = 0; i < 5; i++) {
        TouchEvent event(TouchEventType::TouchDown, 0, i, i, i * 100);
        queue.enqueue(event);
    }
    
    TEST_ASSERT_EQUAL(5, queue.getCount());
    
    queue.clear();
    
    TEST_ASSERT_TRUE(queue.isEmpty());
    TEST_ASSERT_EQUAL(0, queue.getCount());
    TEST_ASSERT_FALSE(queue.hasEvents());
}

// ============================================================================
// Test: Clear empty queue
// ============================================================================
void test_clear_empty_queue(void) {
    TouchEventQueue queue;
    
    queue.clear();
    
    TEST_ASSERT_TRUE(queue.isEmpty());
    TEST_ASSERT_EQUAL(0, queue.getCount());
}

// ============================================================================
// Test: hasEvents() and getCount()
// ============================================================================
void test_hasEvents_and_getCount(void) {
    TouchEventQueue queue;
    
    TEST_ASSERT_FALSE(queue.hasEvents());
    TEST_ASSERT_EQUAL(0, queue.getCount());
    
    // Add one event
    TouchEvent event(TouchEventType::TouchDown, 0, 0, 0, 0);
    queue.enqueue(event);
    
    TEST_ASSERT_TRUE(queue.hasEvents());
    TEST_ASSERT_EQUAL(1, queue.getCount());
    
    // Dequeue - should be empty
    TouchEvent outEvent;
    queue.dequeue(outEvent);
    
    TEST_ASSERT_FALSE(queue.hasEvents());
    TEST_ASSERT_EQUAL(0, queue.getCount());
}

// ============================================================================
// Test: Dequeue from empty queue
// ============================================================================
void test_dequeue_empty_queue(void) {
    TouchEventQueue queue;
    TouchEvent event;
    
    bool result = queue.dequeue(event);
    
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Test: Drop events
// ============================================================================
void test_drop_events(void) {
    TouchEventQueue queue;
    
    // Add 5 events
    for (int i = 0; i < 5; i++) {
        TouchEvent event(TouchEventType::TouchDown, 0, i, i, i * 100);
        queue.enqueue(event);
    }
    
    TEST_ASSERT_EQUAL(5, queue.getCount());
    
    // Drop 2 events
    uint8_t dropped = queue.drop(2);
    
    TEST_ASSERT_EQUAL(2, dropped);
    TEST_ASSERT_EQUAL(3, queue.getCount());
}

// ============================================================================
// Test: Drop more than available
// ============================================================================
void test_drop_more_than_available(void) {
    TouchEventQueue queue;
    
    // Add 3 events
    for (int i = 0; i < 3; i++) {
        TouchEvent event(TouchEventType::TouchDown, 0, i, i, i * 100);
        queue.enqueue(event);
    }
    
    // Try to drop 10
    uint8_t dropped = queue.drop(10);
    
    TEST_ASSERT_EQUAL(3, dropped);  // Only 3 available
    TEST_ASSERT_EQUAL(0, queue.getCount());
    TEST_ASSERT_TRUE(queue.isEmpty());
}

// ============================================================================
// Test: getEvents pull-based API
// ============================================================================
void test_getEvents_pull_api(void) {
    TouchEventQueue queue;
    
    // Add 3 events
    for (int i = 0; i < 3; i++) {
        TouchEvent event(TouchEventType::TouchDown, i, i * 10, i * 20, i * 100);
        queue.enqueue(event);
    }
    
    TouchEvent buffer[5];
    uint8_t count = queue.getEvents(buffer, 5);
    
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(0, buffer[0].id);
    TEST_ASSERT_EQUAL(1, buffer[1].id);
    TEST_ASSERT_EQUAL(2, buffer[2].id);
    
    // Queue should be empty after getEvents
    TEST_ASSERT_TRUE(queue.isEmpty());
}

// ============================================================================
// Test: getEvents with small buffer
// ============================================================================
void test_getEvents_small_buffer(void) {
    TouchEventQueue queue;
    
    // Add 5 events
    for (int i = 0; i < 5; i++) {
        TouchEvent event(TouchEventType::TouchDown, i, i * 10, i * 20, i * 100);
        queue.enqueue(event);
    }
    
    TouchEvent buffer[2];
    uint8_t count = queue.getEvents(buffer, 2);
    
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL(0, buffer[0].id);
    TEST_ASSERT_EQUAL(1, buffer[1].id);
    
    // Should have 3 remaining
    TEST_ASSERT_EQUAL(3, queue.getCount());
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_queue_initialization);
    RUN_TEST(test_enqueue_single_event);
    RUN_TEST(test_dequeue_single_event);
    RUN_TEST(test_enqueue_dequeue_multiple);
    RUN_TEST(test_queue_overflow_drops_oldest);
    RUN_TEST(test_peek_without_removing);
    RUN_TEST(test_peek_empty_queue);
    RUN_TEST(test_peek_multiple);
    RUN_TEST(test_clear_operation);
    RUN_TEST(test_clear_empty_queue);
    RUN_TEST(test_hasEvents_and_getCount);
    RUN_TEST(test_dequeue_empty_queue);
    RUN_TEST(test_drop_events);
    RUN_TEST(test_drop_more_than_available);
    RUN_TEST(test_getEvents_pull_api);
    RUN_TEST(test_getEvents_small_buffer);
    return UNITY_END();
}