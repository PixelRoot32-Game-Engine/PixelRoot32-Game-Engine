/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/Entity.h"
#include "math/Scalar.h"

namespace pixelroot32::gameplay {

/**
 * @brief Ready-made depth comparator: orders entities by their "bottom" Y
 * coordinate (`position.y + height`), ascending.
 *
 * Matches `core::Scene::DepthComparator`'s signature
 * (`bool (*)(Entity*, Entity*)`) so it can be assigned directly to a
 * `Scene`-derived class's `depthComparator` member. Convenience helper for
 * the common top-down/isometric case where entities lower on screen should
 * draw after (in front of) entities higher on screen that share the same
 * `renderLayer`. Not required — games are free to supply their own
 * comparator with the same signature.
 *
 * @param a First entity.
 * @param b Second entity.
 * @return true when `a`'s bottom edge is above `b`'s (`a` must draw before `b`).
 */
inline bool compareByBottomY(core::Entity* a, core::Entity* b) {
    const math::Scalar bottomA = a->position.y + math::toScalar(a->height);
    const math::Scalar bottomB = b->position.y + math::toScalar(b->height);
    return bottomA < bottomB;
}

}  // namespace pixelroot32::gameplay
