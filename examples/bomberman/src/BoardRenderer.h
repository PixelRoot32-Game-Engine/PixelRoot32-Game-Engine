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
                  const uint8_t (&blastShape)[kCells],
                  const uint8_t (&blastDist)[kCells],
                  const uint8_t (&blastRange)[kCells]);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    const TileType (&board_)[kCells];
    const Bomb (&bombs_)[kMaxBombs];
    const uint8_t (&blastSteps_)[kCells];
    const uint8_t (&blastShape_)[kCells];
    const uint8_t (&blastDist_)[kCells];
    const uint8_t (&blastRange_)[kCells];
};

/**
 * @brief Returns the explosion sprite for the given blast cell.
 *
 * Range-aware chain (Phase 2+): given the cell's shape, distance from
 * the bomb cell (`dist`), the bomb's range (`range`), and the
 * animation frame derived from `blastSteps`, picks one of the named
 * groups in ExplosionSprites.h:
 *   - dist==0 (or shape==Center): kCENTER[frame]
 *   - dir=ArmHR + dist==1:        kARMBASE_HR[frame]
 *   - dir=ArmHR + dist<range:     kARNEXT_HR[frame]
 *   - dir=ArmHR + dist==range:    kTIPR[frame]
 * (analogous for HL, VU, VD)
 *
 * Returns nullptr if `shape` is out of range or the Tip* fallback
 * branches are taken with an unrecognized value.
 *
 * Animation frame index: 3 - (blastSteps / 7), clamped to 0..3. The NES
 * cross grows from thin (frame 0) at the start of the explosion to
 * thick (frame 3) at the end, replacing the old 2-frame flicker. As
 * blastSteps counts down from 25 to 0, this yields roughly 7 steps per
 * frame, with frame 3 (thickest) covering the last ~5 steps so the
 * cross "lingers" before disappearing.
 *
 * Defined inline in the header (same pattern as softWallSpriteFor) so the
 * Phase 2 unit test can call it without linking against examples/.
 */
inline const pixelroot32::graphics::Sprite4bpp* explosionSpriteFor(uint8_t shape, uint8_t dist,
                                                                   uint8_t range, uint8_t blastSteps) {
    if (shape > static_cast<uint8_t>(BlastShape::TipD)) return nullptr;
    int frame = 3 - (blastSteps / 7);
    if (frame < 0) frame = 0;
    if (frame > 3) frame = 3;

    // Center cell: always kCENTER.
    if (shape == static_cast<uint8_t>(BlastShape::Center) || dist == 0) {
        return &kCENTER[frame];
    }

    const bool isTip = (dist == range);
    switch (static_cast<BlastShape>(shape)) {
        case BlastShape::ArmHL:
            return isTip ? &kTIPL[frame] : (dist == 1 ? &kARMBASE_HL[frame] : &kARNEXT_HL[frame]);
        case BlastShape::ArmHR:
            return isTip ? &kTIPR[frame] : (dist == 1 ? &kARMBASE_HR[frame] : &kARNEXT_HR[frame]);
        case BlastShape::ArmVU:
            return isTip ? &kTIPU[frame] : (dist == 1 ? &kARMBASE_VU[frame] : &kARNEXT_VU[frame]);
        case BlastShape::ArmVD:
            return isTip ? &kTIPD[frame] : (dist == 1 ? &kARMBASE_VD[frame] : &kARNEXT_VD[frame]);
        case BlastShape::TipL: return &kTIPL[frame];
        case BlastShape::TipR: return &kTIPR[frame];
        case BlastShape::TipU: return &kTIPU[frame];
        case BlastShape::TipD: return &kTIPD[frame];
        default: return nullptr;
    }
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
