#pragma once

#include "graphics/Renderer.h"

/**
 * @file TileFormat.h
 * @brief The single seam between this example and the sprite bit depth it uses.
 *
 * Iteration 1 ships 1bpp art. Iteration 2 is meant to ship 4bpp art without
 * touching the scene, the player, the map data or the room graph. Everything
 * those files know about pixels lives behind the four declarations below:
 *
 *   - SceneSprite / SceneTileMap — the descriptor types.
 *   - drawSceneSprite / drawSceneTileMap — the draw calls.
 *
 * The engine's `drawSprite` and `drawTileMap` overloads do not share a
 * signature across bit depths: the 1bpp pair takes a `Color` (the whole map or
 * sprite renders in it) while the 2bpp/4bpp pair takes a palette that lives
 * inside the descriptor. The wrappers keep the *caller's* signature stable
 * across that difference — after the migration `color` is simply unused, and
 * the call sites do not change.
 *
 * See README.md, "Migrating the art to 4bpp", for the full checklist.
 */
namespace zelda_overworld {

namespace gfx = pixelroot32::graphics;

// ---------------------------------------------------------------------------
// Descriptor types
// ---------------------------------------------------------------------------
// 4bpp migration: swap for gfx::Sprite4bpp / gfx::TileMap4bpp.

using SceneSprite  = gfx::Sprite;
using SceneTileMap = gfx::TileMap;

// ---------------------------------------------------------------------------
// Draw calls
// ---------------------------------------------------------------------------

/**
 * @brief Draws one sprite in this example's tile format.
 * @param color Ink color. 1bpp only — a 4bpp sprite carries its own palette
 *              and ignores this argument.
 */
inline void drawSceneSprite(gfx::Renderer& renderer,
                            const SceneSprite& sprite,
                            int x,
                            int y,
                            gfx::Color color,
                            bool flipX = false) {
    // 4bpp migration: renderer.drawSprite(sprite, x, y, flipX);
    renderer.drawSprite(sprite, x, y, color, flipX);
}

/**
 * @brief Draws one tilemap layer in this example's tile format.
 * @param color Ink color for the whole layer. 1bpp only — see drawSceneSprite.
 * @param layerType Static skips per-cell dirty marking; the world scrolls, so
 *                  the scene passes Dynamic.
 */
inline void drawSceneTileMap(gfx::Renderer& renderer,
                             const SceneTileMap& map,
                             int originX,
                             int originY,
                             gfx::Color color,
                             gfx::LayerType layerType = gfx::LayerType::Dynamic) {
    // 4bpp migration: renderer.drawTileMap(map, originX, originY, layerType);
    renderer.drawTileMap(map, originX, originY, color, layerType);
}

} // namespace zelda_overworld
