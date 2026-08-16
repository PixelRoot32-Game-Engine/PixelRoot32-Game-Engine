#pragma once

#include "platforms/PlatformDefaults.h"

/**
 * @file Scenes.h
 * @brief The game's scenes, as objects the platform headers can reach.
 *
 * One stage, one scene. This file exists anyway so the two platform entry
 * points name the same object instead of each defining their own, and so a
 * second stage can be added without either of them changing.
 */

#ifndef PIXELROOT32_ENABLE_4BPP_SPRITES
#error "midway_clone needs -D PIXELROOT32_ENABLE_4BPP_SPRITES. \
The engine gates its 4bpp draw paths with if constexpr, so without the flag \
this example builds clean and renders nothing — a black screen with no \
diagnostic. Failing here instead is the whole point of this check."
#endif

#include "MidwayScene.h"

namespace midway_clone {

extern MidwayScene midwayScene;

} // namespace midway_clone
