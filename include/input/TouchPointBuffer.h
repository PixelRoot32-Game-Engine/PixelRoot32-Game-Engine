/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchPointBuffer - Ring buffer for touch points
 * Fixed-size circular buffer with no dynamic allocation
 */
#pragma once

#include "input/TouchPoint.h"
#include <cstddef>

namespace pixelroot32::input {

/**
 * @class TouchPointBuffer
 * @brief Ring buffer for storing touch points
 * 
 * Implements a fixed-size circular buffer with:
 * - O(1) push and pop operations
 * - No dynamic memory allocation
 * - Thread-safe (single consumer assumed)
 * 
 * Invariants:
 * - count <= TOUCH_MAX_POINTS
 * - head always points to next write position
 * - oldest point is at (head + 1) % TOUCH_MAX_POINTS when count > 0
 */
class TouchPointBuffer {
public:
    static constexpr uint8_t CAPACITY = TOUCH_MAX_POINTS;
    
    /**
     * @brief Construct empty buffer
     */
    TouchPointBuffer();
    
    /**
     * @brief Push a touch point to the buffer
     * @param point Touch point to add
     * @return true if added successfully
     */
    bool push(const TouchPoint& point);
    
    /**
     * @brief Pop the oldest touch point
     * @param outPoint Output for popped point
     * @return true if point was available
     */
    bool pop(TouchPoint& outPoint);
    
    /**
     * @brief Peek at oldest point without removing
     * @return Pointer to oldest point, nullptr if empty
     */
    const TouchPoint* peekOldest() const;
    
    /**
     * @brief Peek at newest point without removing
     * @return Pointer to newest point, nullptr if empty
     */
    const TouchPoint* peekNewest() const;
    
    /**
     * @brief Get point at specific index
     * @param index Index (0 = oldest, count-1 = newest)
     * @return Pointer to point, nullptr if index out of range
     */
    const TouchPoint* at(uint8_t index) const;
    
    /**
     * @brief Clear all points from buffer
     */
    void clear();
    
    /**
     * @brief Get current count of points in buffer
     * @return Number of points
     */
    uint8_t count() const;
    
    /**
     * @brief Check if buffer is empty
     * @return true if no points
     */
    bool isEmpty() const;
    
    /**
     * @brief Check if buffer is full
     * @return true if no more points can be added
     */
    bool isFull() const;
    
    /**
     * @brief Get capacity of buffer
     * @return Maximum number of points
     */
    constexpr uint8_t capacity() const;
    
private:
    TouchPoint points[CAPACITY];
    uint8_t head;     // Next write position
    uint8_t count_;   // Current number of points
};

/**
 * @brief Type alias for backward compatibility
 */
using TouchPointRing = TouchPointBuffer;

// =============================================================================
// Inline implementations (following STYLE_GUIDE.md)
// =============================================================================

inline TouchPointBuffer::TouchPointBuffer()
    : head(0), count_(0) {
    // Initialize all points to empty
    for (uint8_t i = 0; i < CAPACITY; i++) {
        points[i] = TouchPoint();
    }
}

inline bool TouchPointBuffer::push(const TouchPoint& point) {
    if (isFull()) {
        // Buffer full - overwrite oldest (advance head)
        head = (head + 1) % CAPACITY;
        count_ = CAPACITY;
    } else {
        count_++;
    }
    
    points[head] = point;
    return true;
}

inline bool TouchPointBuffer::pop(TouchPoint& outPoint) {
    if (isEmpty()) {
        return false;
    }
    
    // Oldest point is at (head + 1 - count) % CAPACITY
    uint8_t tail = (head + CAPACITY + 1 - count_) % CAPACITY;
    outPoint = points[tail];
    count_--;
    
    return true;
}

inline const TouchPoint* TouchPointBuffer::peekOldest() const {
    if (isEmpty()) {
        return nullptr;
    }
    uint8_t tail = (head + CAPACITY + 1 - count_) % CAPACITY;
    return &points[tail];
}

inline const TouchPoint* TouchPointBuffer::peekNewest() const {
    if (isEmpty()) {
        return nullptr;
    }
    return &points[head];
}

inline const TouchPoint* TouchPointBuffer::at(uint8_t index) const {
    if (index >= count_) {
        return nullptr;
    }
    uint8_t tail = (head + CAPACITY + 1 - count_) % CAPACITY;
    uint8_t actualIndex = (tail + index) % CAPACITY;
    return &points[actualIndex];
}

inline void TouchPointBuffer::clear() {
    head = 0;
    count_ = 0;
    for (uint8_t i = 0; i < CAPACITY; i++) {
        points[i] = TouchPoint();
    }
}

inline uint8_t TouchPointBuffer::count() const {
    return count_;
}

inline bool TouchPointBuffer::isEmpty() const {
    return count_ == 0;
}

inline bool TouchPointBuffer::isFull() const {
    return count_ == CAPACITY;
}

inline constexpr uint8_t TouchPointBuffer::capacity() const {
    return CAPACITY;
}

} // namespace pixelroot32::input