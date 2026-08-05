/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "gameplay/GameplayEventType.h"
#include "math/Scalar.h"

#include <cstdint>

namespace pixelroot32::gameplay {

/**
 * @struct GameplayEvent
 * @brief Fixed-size POD carried by the GameplayEventBus.
 *
 * One flat struct, no union, no variable-length payload. Field declaration
 * order (pointer, then Scalar, then the two ids, then the tag) is chosen for
 * alignment, not for readability — do not reorder without re-checking the
 * byte budget in design.md (16 B on ESP32-C3, 24 B on native).
 *
 * `userData` is an untyped escape hatch: the engine never interprets,
 * manages, or frees it, mirroring the existing `PhysicsActor::userData`
 * contract (include/core/PhysicsActor.h).
 */
struct GameplayEvent {
    void*             userData   = nullptr;   ///< Untyped escape hatch (never owned by the engine).
    math::Scalar      value{};                ///< Position / quantity / damage-style datum.
    uint16_t          entityIdA  = 0;         ///< "Who" — the primary actor's entity id.
    uint16_t          entityIdB  = 0;         ///< "With whom" (0 = none).
    GameplayEventType type       = GameplayEventType::None;   ///< Event tag.
};

}
