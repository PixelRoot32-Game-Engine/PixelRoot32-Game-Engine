#pragma once

#include "graphics/Renderer.h"

namespace iso_dungeon {

/**
 * @brief Draws a sprite so that its `footY` row lands on a cell's centre.
 *
 * The single anchoring rule for this example. A 16px floor diamond, a 40px
 * wall, a 48px pillar and the 24px hero all go through this one function, so
 * none of them can drift a pixel out of alignment with the others.
 *
 * Each sprite ships its own `*_FOOT_Y` next to its descriptor (see
 * DungeonTiles.h and HeroSprites.h) because the correct row differs by kind:
 * for a block it is the centre of its base diamond, which covers the whole
 * tile footprint; for an actor it is the bottom of the sprite, because the
 * hero stands ON the cell rather than filling it. Encoding that per sprite is
 * what keeps the call sites free of ad-hoc Y offsets -- the way isometric art
 * quietly goes crooked.
 *
 * @param footY   Sprite row that must coincide with the cell centre.
 * @param centreX Screen X of the target cell's diamond centre.
 * @param centreY Screen Y of the target cell's diamond centre.
 */
inline void drawAtCell(pixelroot32::graphics::Renderer& renderer,
                       const pixelroot32::graphics::Sprite4bpp& sprite,
                       int footY, int centreX, int centreY, bool flipX = false) {
    renderer.drawSprite(sprite, centreX - sprite.width / 2, centreY - footY, 0, flipX);
}

}  // namespace iso_dungeon
