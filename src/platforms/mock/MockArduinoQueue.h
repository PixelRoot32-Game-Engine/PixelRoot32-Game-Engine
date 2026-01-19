/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#ifndef MOCK_ARDUINO_QUEUE_H
#define MOCK_ARDUINO_QUEUE_H

#ifdef PLATFORM_NATIVE

#include <queue>
#include <cstddef>

/**
 * @class ArduinoQueue
 * @brief Mocks the ArduinoQueue library using std::queue for native platforms.
 * @tparam T The type of elements stored in the queue.
 */
template<typename T>
class ArduinoQueue {
public:
    ArduinoQueue(size_t capacity = 0) {
        (void)capacity; // capacity ignored in mock, std::queue is dynamic
    }

    bool enqueue(const T& item) {
        q.push(item);
        return true;
    }

    T dequeue() {
        if (q.empty()) {
            return T{};
        }
        T v = q.front();
        q.pop();
        return v;
    }

    bool isEmpty() const {
        return q.empty();
    }

    bool isFull() const {
        return false; // std::queue is unbounded
    }

    size_t itemCount() const {
        return q.size();
    }

    void clear() {
        while (!q.empty()) {
            q.pop();
        }
    }

private:
    std::queue<T> q;
};
 // namespace pixelroot32::platforms::mock

#endif // PLATFORM_NATIVE

#endif // MOCK_ARDUINO_QUEUE_H
