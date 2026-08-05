#pragma once
#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "BombermanBoard.h"
#include "BombermanBombs.h"
#include "BombermanConstants.h"

namespace bomberman {

/**
 * @class BoardRenderer
 * @brief Layer-0 entity: screen clear, status band, board frame, every
 *        tile currently on the board, active bombs, and explosion cells.
 *
 * Holds references to the scene's board/bomb/blastSteps arrays — never
 * copies — so a regenerated level or a fresh bomb pool is visible without
 * re-adding or reconstructing this entity. Power-up tiles are drawn as part
 * of the same per-cell pass as walls/exit; enemies and the player draw
 * themselves as layer-1 actors, added separately.
 */
class BoardRenderer : public pixelroot32::core::Entity {
public:
    BoardRenderer(const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs],
                  const uint8_t (&blastSteps)[kCells]);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    const TileType (&board_)[kCells];
    const Bomb (&bombs_)[kMaxBombs];
    const uint8_t (&blastSteps_)[kCells];
};

}  // namespace bomberman
