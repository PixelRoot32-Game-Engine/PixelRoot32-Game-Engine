#pragma once

#include "GameConstants.h"
#include "TileFormat.h"

/**
 * @file OverworldMap.h
 * @brief The world's tile data, as a list of drawable layers plus a collision map.
 *
 * **The world is a list of layers, and the list is data.** The scene draws
 * `OVERWORLD_LAYER_COUNT` layers in array order and asks nothing about what
 * they contain. That is what makes the 4bpp migration structural-change-free:
 * 1bpp needs three layers because a layer can only be one color, and 4bpp
 * needs one because the palette lives per cell. Collapsing three entries into
 * one is an edit to this file, not to the scene.
 *
 * All three layers index into the same OVERWORLD_TILES array, so a tile id
 * means the same thing everywhere and the collision map can be derived from
 * ids alone rather than maintained by hand.
 */
namespace zelda_overworld {

/// One drawable layer of the world plus the ink it renders in.
struct WorldLayer {
    SceneTileMap map;         ///< Full-world tilemap; index 0 means "nothing here".
    pixelroot32::graphics::Color color;  ///< 1bpp ink. Unused once the art is 4bpp.
};

/// Layers in draw order, back to front.
inline constexpr int OVERWORLD_LAYER_COUNT = 3;

/**
 * @brief The world layers. Valid only after buildOverworld() has run.
 *
 * Not const: TileMapGeneric::indices is a mutable pointer, and the index
 * buffers are filled at init from the character map rather than written out
 * as 660-entry literals.
 */
extern WorldLayer OVERWORLD_LAYERS[OVERWORLD_LAYER_COUNT];

/**
 * @brief Expands the character map into the layer index buffers and the
 *        collision map.
 *
 * Idempotent, allocation-free, and cheap enough to run from Scene::init().
 * Must be called before the first draw or the first isSolidCell() query.
 */
void buildOverworld();

/**
 * @brief Whether the world cell at (col, row) blocks the player.
 * @return true for out-of-bounds cells — the world edge is a wall.
 *
 * Derived from tile ids at build time, so it cannot drift away from the art
 * the way a hand-maintained collision table would.
 */
bool isSolidCell(int col, int row);

} // namespace zelda_overworld
