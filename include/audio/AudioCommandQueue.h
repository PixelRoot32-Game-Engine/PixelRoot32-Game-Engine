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
     * @brief Multi-Producer Single-Consumer lock-free ring buffer for AudioCommands.
     * 
     * Fixed size, no allocation. Supports multiple concurrent producer threads.
     * If the queue is full, the newest command is dropped and droppedCommands is incremented.
     * Uses compare-and-swap (CAS) for atomic ring index advancement.
     */
    class AudioCommandQueue {
    public:
        // Configurable capacity: 128-1024. Default 128 for memory efficiency.
        // Can be increased for high-throughput use cases at compile time.
        #ifndef AUDIO_COMMAND_QUEUE_CAPACITY
        static constexpr size_t CAPACITY = 128;
        #else
        static_assert(AUDIO_COMMAND_QUEUE_CAPACITY >= 128 && AUDIO_COMMAND_QUEUE_CAPACITY <= 1024,
            "AUDIO_COMMAND_QUEUE_CAPACITY must be between 128 and 1024");
        static constexpr size_t CAPACITY = AUDIO_COMMAND_QUEUE_CAPACITY;
        #endif

        AudioCommandQueue() : head(0), tail(0), droppedCommands(0) {}

        /**
         * @brief Enqueues a command. Thread-safe for multiple producers.
         * @param cmd The command to enqueue.
         * @return true if successful, false if the queue is full (dropped).
         */
        bool enqueue(const AudioCommand& cmd) {
            size_t currentTail = tail.load(std::memory_order_relaxed);
            
            do {
                size_t nextTail = (currentTail + 1) % CAPACITY;
                
                if (nextTail == head.load(std::memory_order_acquire)) {
                    // Queue full - drop newest command and increment dropped counter
                    droppedCommands.fetch_add(1, std::memory_order_relaxed);
                    return false;
                }
                
                // Try to advance tail atomically using CAS
                if (tail.compare_exchange_weak(currentTail, nextTail,
                    std::memory_order_release, std::memory_order_relaxed)) {
                    // Successfully advanced tail, now write the command
                    buffer[currentTail] = cmd;
                    return true;
                }
                // CAS failed, tail was modified by another producer, retry
            } while (true);
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

        /**
         * @brief Returns the count of dropped commands due to queue full.
         * Thread-safe for concurrent reads from multiple producers.
         */
        size_t getDroppedCommands() const {
            return droppedCommands.load(std::memory_order_relaxed);
        }

    private:
        AudioCommand buffer[CAPACITY];
        std::atomic<size_t> head;
        std::atomic<size_t> tail;
        std::atomic<size_t> droppedCommands;
    };

} // namespace pixelroot32::audio
