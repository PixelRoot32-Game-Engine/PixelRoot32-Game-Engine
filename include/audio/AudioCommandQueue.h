/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioTypes.h"
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace pixelroot32::audio {

    /**
     * @class AudioCommandQueue
     * @brief Single-Producer Single-Consumer lock-free ring buffer for AudioCommands.
     * 
     * Fixed size, no allocation. If the queue is full, the newest command is dropped.
     */
    class AudioCommandQueue {
    public:
        static constexpr size_t CAPACITY = 128;

        AudioCommandQueue() : head(0), tail(0) {}

        /**
         * @brief Enqueues a command. Called from the producer (Game Thread).
         * @param cmd The command to enqueue.
         * @return true if successful, false if the queue is full.
         */
        bool enqueue(const AudioCommand& cmd) {
            size_t currentTail = tail.load(std::memory_order_relaxed);
            size_t nextTail = (currentTail + 1) % CAPACITY;

            if (nextTail == head.load(std::memory_order_acquire)) {
                // Queue full - drop newest command
                return false;
            }

            buffer[currentTail] = cmd;
            tail.store(nextTail, std::memory_order_release);
            return true;
        }

        /**
         * @brief Dequeues a command. Called from the consumer (Audio Thread).
         * @param outCmd Reference to store the dequeued command.
         * @return true if a command was dequeued, false if the queue is empty.
         */
        bool dequeue(AudioCommand& outCmd) {
            size_t currentHead = head.load(std::memory_order_relaxed);

            if (currentHead == tail.load(std::memory_order_acquire)) {
                // Queue empty
                return false;
            }

            outCmd = buffer[currentHead];
            head.store((currentHead + 1) % CAPACITY, std::memory_order_release);
            return true;
        }

        /**
         * @brief Checks if the queue is empty.
         */
        bool isEmpty() const {
            return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
        }

    private:
        AudioCommand buffer[CAPACITY];
        std::atomic<size_t> head;
        std::atomic<size_t> tail;
    };

} // namespace pixelroot32::audio
