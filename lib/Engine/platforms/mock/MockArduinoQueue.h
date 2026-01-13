#ifndef MOCK_ARDUINO_QUEUE_H
#define MOCK_ARDUINO_QUEUE_H

#ifdef PLATFORM_NATIVE

#include <queue>
#include <cstddef>

// Mock ArduinoQueue using std::queue
template<typename T>
class ArduinoQueue {
public:
    ArduinoQueue(size_t capacity = 0) {
        (void)capacity; // capacity ignored in mock
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

#endif // PLATFORM_NATIVE

#endif // MOCK_ARDUINO_QUEUE_H
