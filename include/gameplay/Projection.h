/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "math/Projection.h"

#if PIXELROOT32_ENABLE_PROJECTION

#include <type_traits>

/**
 * @file Projection.h
 * @brief `pixelroot32::gameplay` alias of `pixelroot32::math::ProjectionSpec`
 *        and its free functions.
 *
 * The canonical implementation lives in `math/Projection.h` under
 * `pixelroot32::math` (Layer 2). This header forwards it into `gameplay` via
 * `using`-declarations — the same entities, not copies — so existing
 * `gameplay::`-qualified callers (`GridMotion.h`, `IsoDungeonConstants.h`, and
 * the test suites) keep compiling unchanged.
 *
 * A `using`-declaration carries **all** overloads of a name, so the four
 * deleted `double` overloads come across automatically and must not be
 * redeclared here.
 *
 * `gameplay::ProjectionSpec` MUST be an alias, never a duplicate. The
 * `using`-declaration is what guarantees that today -- an alias cannot be a
 * copy, so nothing at this commit can make the two types differ.
 *
 * The `static_assert` below is therefore tautological as written, and that is
 * deliberate rather than an oversight: it is a tripwire against a LATER edit,
 * not a proof about this one. Replace the alias with a `struct
 * ProjectionSpec { ... }` -- the obvious way someone "fixes" a forwarding
 * header they do not want to depend on -- and the assert fires in every
 * translation unit, in every environment, before any test runs. Without it
 * that edit compiles, passes the whole suite, and silently forks the type the
 * Tilemap Editor generates against.
 */

namespace pixelroot32::gameplay {

using ProjectionSpec = pixelroot32::math::ProjectionSpec;

static_assert(std::is_same_v<ProjectionSpec, pixelroot32::math::ProjectionSpec>,
              "gameplay::ProjectionSpec must ALIAS math::ProjectionSpec, never duplicate it.");

using pixelroot32::math::projectionDet;
using pixelroot32::math::cellToScreenX;   // carries the deleted double overload
using pixelroot32::math::cellToScreenY;   // carries the deleted double overload
using pixelroot32::math::screenToCellX;   // carries the deleted double overload
using pixelroot32::math::screenToCellY;   // carries the deleted double overload
using pixelroot32::math::projectionSpecIsValid;
using pixelroot32::math::rowMajorIsPainterOrder;

namespace detail {
using pixelroot32::math::detail::projectionFloorDiv;
}  // namespace detail

}  // namespace pixelroot32::gameplay

#endif  // PIXELROOT32_ENABLE_PROJECTION
