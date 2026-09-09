/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/Entity.h"
#include "math/Scalar.h"
#include "platforms/PlatformDefaults.h"

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

#if PIXELROOT32_ENABLE_DEPTH_SORT

/**
 * @brief Ready-made depth comparator: orders entities by the `depthKey` the
 * game wrote on them, ascending.
 *
 * The projection-agnostic counterpart to compareByBottomY(). That comparator
 * orders by world Y, which is the correct paint order only while screen depth
 * is a monotone function of world Y — true for an axis-aligned top-down game,
 * false under any non-identity projection. An isometric game writes
 * `depthKey` from its projected anchor (e.g. `cellToScreenY()`) and uses this
 * comparator instead; the engine never interprets the value.
 *
 * Matches `core::Scene::DepthComparator`'s signature
 * (`bool (*)(Entity*, Entity*)`), so it can be assigned directly to a
 * `Scene`-derived class's `depthComparator` member. No allocation, no virtual
 * dispatch, inlinable — which matters because `Scene::sortEntities()` is an
 * insertion sort and calls this O(n^2) times in the worst case.
 *
 * Equal keys compare `false`, so entities that share a key keep their
 * insertion-relative order, exactly like the layer-only path.
 *
 * @param a First entity.
 * @param b Second entity.
 * @return true when `a`'s key is lower (`a` must draw before `b`).
 */
inline bool compareByDepthKey(core::Entity* a, core::Entity* b) {
    return a->depthKey < b->depthKey;
}

#endif  // PIXELROOT32_ENABLE_DEPTH_SORT

}  // namespace pixelroot32::gameplay
