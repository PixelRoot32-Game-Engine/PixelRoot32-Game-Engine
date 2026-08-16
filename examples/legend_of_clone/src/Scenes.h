#pragma once

#include "platforms/PlatformDefaults.h"

/**
 * @file Scenes.h
 * @brief The game's scenes, as objects each can reach.
 *
 * The two scenes name each other — the overworld starts a transition to the
 * dungeon and the dungeon starts one back — so neither can own the other and
 * both are defined once in Scenes.cpp. The platform headers only pick which one
 * the engine starts on.
 */

#ifndef PIXELROOT32_ENABLE_4BPP_SPRITES
#error "legend_of_clone needs -D PIXELROOT32_ENABLE_4BPP_SPRITES. \
The engine gates its 4bpp draw paths with if constexpr, so without the flag \
this example builds clean and renders nothing — a black screen with no \
diagnostic. Failing here instead is the whole point of this check."
#endif

#include "OverworldScene.h"
#include "DungeonScene.h"

namespace legend_of_clone {

extern OverworldScene overworldScene;
extern DungeonScene dungeonScene;

} // namespace legend_of_clone
