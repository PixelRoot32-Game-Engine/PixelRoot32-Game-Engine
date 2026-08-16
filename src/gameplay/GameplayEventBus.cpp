/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "gameplay/GameplayEventBus.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS

namespace pixelroot32::gameplay {

namespace {
    // kCapacity is guaranteed a power of two (design.md D2), so wraparound is
    // a mask rather than a modulo — cheaper on the RISC-V ESP32-C3, which has
    // no hardware integer divide.
    constexpr uint16_t wrapIndex(uint16_t index) {
        return static_cast<uint16_t>(index & (GameplayEventBus::kCapacity - 1));
    }
}

bool GameplayEventBus::publish(const GameplayEvent& event) {
    if (count_ == kCapacity) {
        // Drop-newest: keep already-buffered events untouched, discard the
        // incoming one, and record the loss for diagnostics.
        ++droppedCount_;
        return false;
    }

    events_[tail_] = event;
    tail_ = wrapIndex(static_cast<uint16_t>(tail_ + 1));
    ++count_;
    return true;
}

bool GameplayEventBus::consume(GameplayEvent& outEvent) {
    if (count_ == 0) {
        return false;
    }

    outEvent = events_[head_];
    head_ = wrapIndex(static_cast<uint16_t>(head_ + 1));
    --count_;
    return true;
}

void GameplayEventBus::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

}

#endif // PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
