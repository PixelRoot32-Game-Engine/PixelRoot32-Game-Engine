/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchEventQueue.cpp - Ring buffer implementation
 */
#include <input/TouchEventQueue.h>

namespace pixelroot32::input {

bool TouchEventQueue::enqueue(const TouchEvent& event) {
    if (isFull()) {
        return false;
    }
    
    events[tail] = event;
    tail = (tail + 1) % TOUCH_EVENT_QUEUE_SIZE;
    count++;
    return true;
}

bool TouchEventQueue::dequeue(TouchEvent& event) {
    if (isEmpty()) {
        return false;
    }
    
    event = events[head];
    head = (head + 1) % TOUCH_EVENT_QUEUE_SIZE;
    count--;
    return true;
}

bool TouchEventQueue::peek(TouchEvent& event) const {
    if (isEmpty()) {
        return false;
    }
    
    event = events[head];
    return true;
}

uint8_t TouchEventQueue::peekMultiple(TouchEvent* output, uint8_t maxCount) const {
    if (isEmpty() || output == nullptr || maxCount == 0) {
        return 0;
    }
    
    uint8_t copied = 0;
    uint8_t index = head;
    
    while (copied < count && copied < maxCount) {
        output[copied] = events[index];
        index = (index + 1) % TOUCH_EVENT_QUEUE_SIZE;
        copied++;
    }
    
    return copied;
}

uint8_t TouchEventQueue::drop(uint8_t numToDrop) {
    if (isEmpty() || numToDrop == 0) {
        return 0;
    }
    
    uint8_t actualDrop = (numToDrop > count) ? count : numToDrop;
    head = (head + actualDrop) % TOUCH_EVENT_QUEUE_SIZE;
    count -= actualDrop;
    return actualDrop;
}

uint8_t TouchEventQueue::getEvents(TouchEvent* output, uint8_t maxCount) {
    if (output == nullptr || maxCount == 0) {
        return 0;
    }

    uint8_t toRetrieve = (count < maxCount) ? count : maxCount;
    uint8_t copied = 0;

    while (copied < toRetrieve) {
        output[copied] = events[head];
        head = (head + 1) % TOUCH_EVENT_QUEUE_SIZE;
        count--;
        copied++;
    }

    return copied;
}

} // namespace pixelroot32::input
