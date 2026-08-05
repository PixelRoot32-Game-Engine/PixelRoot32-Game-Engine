/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS

#include "gameplay/GameplayEvent.h"
#include "platforms/EngineConfig.h"

#include <cstdint>

namespace pixelroot32::gameplay {

/**
 * @class GameplayEventBus
 * @brief Fixed-capacity FIFO ring buffer for GameplayEvent, single-instance and Engine-owned.
 *
 * Sized by `GAMEPLAY_EVENT_QUEUE_CAPACITY` (mirror:
 * `pixelroot32::platforms::config::GameplayEventQueueCapacity`). Overflow
 * policy is drop-newest: when the buffer is full, `publish()` discards the
 * incoming event and increments a monotonic drop counter rather than evicting
 * an already-buffered event — this preserves paired TriggerEnter/TriggerExit
 * ordering (see design.md D2).
 *
 * **Non-atomic by design.** Unlike AudioCommandQueue (which bridges the game
 * thread and the audio task with std::atomic), this bus is produced and
 * consumed entirely inside the single-threaded Scene::update()/Scene::draw()
 * loop driven from SceneManager::update(). Plain, non-atomic `uint16_t`
 * head/tail/count indices. Publishing from an ISR or the audio task is
 * explicitly unsupported.
 *
 * The bus is drained (cleared) by SceneManager on every SceneSwap. It is
 * NOT drained on pushScene()/popScene().
 */
class GameplayEventBus {
public:
    /** @brief Fixed capacity of the ring buffer (power of two). */
    static constexpr uint16_t kCapacity =
        static_cast<uint16_t>(pixelroot32::platforms::config::GameplayEventQueueCapacity);

    GameplayEventBus() = default;

    /**
     * @brief Publishes an event to the bus.
     * @param event The event to enqueue (copied into the ring).
     * @return true if the event was stored, false if the buffer was full
     *         (the event is dropped and the drop counter is incremented).
     *
     * Never blocks, never allocates.
     */
    bool publish(const GameplayEvent& event);

    /**
     * @brief Consumes (pops) the oldest pending event, FIFO order.
     * @param outEvent Reference to receive the dequeued event.
     * @return true if an event was retrieved, false if the buffer was empty.
     */
    bool consume(GameplayEvent& outEvent);

    /**
     * @brief Clears all pending events without changing the drop counter.
     *
     * Called by SceneManager on every SceneSwap. Safe to call on an empty
     * buffer (no-op).
     */
    void clear();

    /**
     * @brief Returns the number of events dropped due to a full buffer.
     *
     * Monotonic for the lifetime of this instance; never reset by clear().
     */
    uint32_t getDroppedCount() const { return droppedCount_; }

    /** @brief Number of events currently pending. */
    uint16_t size() const { return count_; }

    /** @brief True if no events are pending. */
    bool isEmpty() const { return count_ == 0; }

    /** @brief True if the buffer is at capacity. */
    bool isFull() const { return count_ == kCapacity; }

private:
    GameplayEvent events_[kCapacity]{};
    uint16_t head_ = 0;          ///< Index of the oldest pending event.
    uint16_t tail_ = 0;          ///< Index where the next published event will be stored.
    uint16_t count_ = 0;         ///< Number of pending events.
    uint32_t droppedCount_ = 0;  ///< Monotonic count of drop-newest overflow events.
};

}

#endif // PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
