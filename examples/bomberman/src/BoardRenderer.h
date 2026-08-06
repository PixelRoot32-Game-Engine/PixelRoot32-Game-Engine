#pragma once
#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "BombermanBoard.h"
#include "BombermanBombs.h"
#include "BombermanConstants.h"
#include "assets/BoardTiles.h"
#include "assets/ExplosionSprites.h"

namespace bomberman {

/**
 * @class BoardRenderer
 * @brief Layer-0 entity: screen clear, status band, board frame, every
 *        tile currently on the board, active bombs, and explosion cells.
 *
 * Holds references to the scene's board/bomb/blastSteps/blastShape arrays
 * — never copies — so a regenerated level or a fresh bomb pool is visible
 * without re-adding or reconstructing this entity. Power-up tiles are drawn
 * as part of the same per-cell pass as walls/exit; enemies and the player
 * draw themselves as layer-1 actors, added separately.
 */
class BoardRenderer : public pixelroot32::core::Entity {
public:
    BoardRenderer(const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs],
                  const uint8_t (&blastSteps)[kCells],
                  const uint8_t (&blastShape)[kCells]);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    const TileType (&board_)[kCells];
    const Bomb (&bombs_)[kMaxBombs];
    const uint8_t (&blastSteps_)[kCells];
    const uint8_t (&blastShape_)[kCells];
};

/**
 * @brief Returns the explosion sprite for the given blast shape and
 *        blast steps count (flicker frame).
 *
 * @param shape     BlastShape enum value for this cell (Center/ArmH/.../TipD).
 * @param blastSteps Remaining explosion display steps for the cell.
 * @return Pointer to the correct kExplosionByShape entry, or nullptr if
 *         shape is out of range.
 *
 * Flicker frame index: (blastSteps / 7) % 2. As blastSteps counts down
 * from 25 to 0, the flicker alternates roughly every 7 steps (~3.5 flips
 * across the full 25-step lifetime of an explosion cell).
 *
 * Defined inline in the header (same pattern as softWallSpriteFor) so the
 * Phase 2 unit test can call it without linking against examples/.
 */
inline const pixelroot32::graphics::Sprite4bpp* explosionSpriteFor(uint8_t shape, uint8_t blastSteps) {
    if (shape > static_cast<uint8_t>(BlastShape::TipD)) return nullptr;
    const uint8_t flicker = (blastSteps / 7) % 2;
    return &kExplosionByShape[shape][flicker];
}

/**
 * @brief Returns the shared soft-wall sprite descriptor for any of the
 *        three soft-wall TileTypes, or nullptr for non-soft-wall types.
 *
 * Load-bearing for the soft-wall secrecy invariant (audit §8.3): the three
 * soft-wall variants (SoftWall, SoftWallHidingExit, SoftWallHidingPowerUp)
 * MUST render pixel-identical so the hidden exit/power-up locations stay
 * secret until the wall is destroyed. This helper is the single source of
 * truth — all three variants return the same `&kSoftWallSprite` pointer.
 *
 * Defined inline in the header so the Phase 1 unit test (which lives in
 * test/unit/ and does not link against examples/bomberman/src/) can call it
 * without a separate translation unit. The function is one line and the
 * inline cost is zero; the alternative (a separate .cpp) would require
 * PlatformIO's `test_build_src` to pull in examples/bomberman/src/, which
 * its `build_src_filter` cannot do.
 */
inline const pixelroot32::graphics::Sprite4bpp* softWallSpriteFor(TileType t) {
    return isSoftWall(t) ? &kSoftWallSprite : nullptr;
}

}  // namespace bomberman
