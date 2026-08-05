#pragma once
#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "BombermanBoard.h"
#include "BombermanConstants.h"

namespace bomberman {

/**
 * @class BoardRenderer
 * @brief Layer-0 entity: screen clear, status band, board frame, and every
 *        tile currently on the board.
 *
 * Holds a reference to the scene's board array — never a copy — so a
 * regenerated level is visible without re-adding or reconstructing this
 * entity. Bomb, explosion, and item draw passes are added in later phases.
 */
class BoardRenderer : public pixelroot32::core::Entity {
public:
    explicit BoardRenderer(const TileType (&board)[kCells]);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    const TileType (&board_)[kCells];
};

}  // namespace bomberman
