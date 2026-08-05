/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

namespace pixelroot32::gameplay {

/**
 * @enum GameplayEventType
 * @brief Tag identifying the meaning of a GameplayEvent.
 *
 * `None` is the default/uninitialized tag. `TriggerEnter`/`TriggerExit` are
 * published by the actor-interaction-triggers capability. `Interact` is
 * reserved for manual interact dispatch. Game-defined event codes MUST start
 * at `UserBase` (64) or above so they never collide with engine-reserved
 * values added in future phases.
 */
enum class GameplayEventType : uint8_t {
    None = 0,       ///< Default/uninitialized tag.
    TriggerEnter,   ///< A tracked interaction pair began contact.
    TriggerExit,    ///< A tracked interaction pair ended contact.
    Interact,       ///< Reserved for manual interact dispatch.
    UserBase = 64   ///< Game-defined event codes start here.
};

}
