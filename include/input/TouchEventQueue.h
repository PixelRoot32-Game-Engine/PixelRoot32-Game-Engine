/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchEventQueue.h - Touch event ring buffer
 * Fixed-size circular buffer for touch events
 */
#pragma once

#include <cstdint>
#include <input/TouchEvent.h>

namespace pixelroot32::input {

/**
 * @class TouchEventQueue
 * @brief Ring buffer for touch events (192 bytes total)
 * 
 * Fixed-size circular buffer with O(1) enqueue/dequeue operations.
 * Uses a static array - no dynamic memory allocation.
 * 
 * Memory layout:
 * - events[16]: 16 * 12 = 192 bytes
 * - head: 1 byte
 * - tail: 1 byte
 * - count: 1 byte
 * Total: ~195 bytes (with padding)
 */
class TouchEventQueue {
public:
    /**
     * @brief Default constructor - initializes empty queue
     */
    TouchEventQueue();
    
    /**
     * @brief Check if queue is empty
     * @return true if no events in queue
     */
    bool isEmpty() const;
    
    /**
     * @brief Check if queue is full
     * @return true if queue cannot accept more events
     */
    bool isFull() const;
    
    /**
     * @brief Get number of events in queue
     * @return Number of events currently queued
     */
    uint8_t getCount() const;
    
    /**
     * @brief Get capacity of queue
     * @return Maximum number of events (16)
     */
    constexpr uint8_t getCapacity() const;
    
    /**
     * @brief Enqueue an event (add to tail)
     * @param event Event to add
     * @return true if event was enqueued, false if queue was full
     */
    bool enqueue(const TouchEvent& event);
    
    /**
     * @brief Dequeue an event (remove from head)
     * @param event Output parameter for dequeued event
     * @return true if event was dequeued, false if queue was empty
     */
    bool dequeue(TouchEvent& event);
    
    /**
     * @brief Peek at head event without removing
     * @param event Output parameter for peeked event
     * @return true if event exists, false if queue empty
     */
    bool peek(TouchEvent& event) const;
    
    /**
     * @brief Peek at multiple events from head
     * @param events Output buffer for events
     * @param maxCount Maximum number of events to peek
     * @return Number of events peeked
     */
    uint8_t peekMultiple(TouchEvent* events, uint8_t maxCount) const;
    
    /**
     * @brief Clear all events from queue
     */
    void clear();
    
    /**
     * @brief Remove and discard n events from head
     * @param count Number of events to drop
     * @return Number of events actually dropped
     */
    uint8_t drop(uint8_t count);
    
    /**
     * @brief Get events by providing caller-owned buffer
     * @param events Caller-provided buffer for events
     * @param maxCount Maximum events to retrieve
     * @return Number of events retrieved
     * 
     * This is the pull-based API: consumer provides the buffer.
     */
    uint8_t getEvents(TouchEvent* events, uint8_t maxCount);
    
    /**
     * @brief Check if events are available (hasEvents)
     * @return true if queue has events
     */
    bool hasEvents() const;
    
private:
    /// Ring buffer storage
    TouchEvent events[TOUCH_EVENT_QUEUE_SIZE];
    
    /// Head index (next to dequeue)
    uint8_t head;
    
    /// Tail index (next to enqueue)
    uint8_t tail;
    
    /// Current event count
    uint8_t count;
};

// Inline implementations

inline TouchEventQueue::TouchEventQueue()
    : head(0)
    , tail(0)
    , count(0) {}

inline bool TouchEventQueue::isEmpty() const {
    return count == 0;
}

inline bool TouchEventQueue::isFull() const {
    return count >= TOUCH_EVENT_QUEUE_SIZE;
}

inline uint8_t TouchEventQueue::getCount() const {
    return count;
}

inline constexpr uint8_t TouchEventQueue::getCapacity() const {
    return TOUCH_EVENT_QUEUE_SIZE;
}

inline bool TouchEventQueue::hasEvents() const {
    return count > 0;
}

inline void TouchEventQueue::clear() {
    head = 0;
    tail = 0;
    count = 0;
}

} // namespace pixelroot32::input
