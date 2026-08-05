#include "BoardRenderer.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace bomberman {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

BoardRenderer::BoardRenderer(const TileType (&board)[kCells])
    : core::Entity(math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, core::EntityType::GENERIC),
      board_(board) {
    setRenderLayer(0);
}

void BoardRenderer::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void BoardRenderer::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::Black);
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, kStatusBandHeight, gfx::Color::Navy);
    renderer.drawRectangle(kBoardOriginX, kBoardOriginY, kBoardWidth, kBoardHeight, gfx::Color::Gray);

    for (int y = 0; y < kRows; ++y) {
        for (int x = 0; x < kCols; ++x) {
            const TileType t = board_[cellIndex(x, y)];
            const int px = gameplay::cellToWorldX(x, kBoardGrid);
            const int py = gameplay::cellToWorldY(y, kBoardGrid);

            if (t == TileType::HardWall) {
                renderer.drawFilledRectangle(px, py, kCellSize, kCellSize, gfx::Color::Gray);
                renderer.drawRectangle(px + 1, py + 1, kCellSize - 2, kCellSize - 2, gfx::Color::White);
            } else if (isSoftWall(t)) {
                // Load-bearing: all three soft-wall variants (SoftWall,
                // SoftWallHidingExit, SoftWallHidingPowerUp) share this one
                // branch and render pixel-identical. Any visible
                // difference would leak the hidden exit/power-up location
                // before the wall is destroyed.
                renderer.drawFilledRectangle(px, py, kCellSize, kCellSize, gfx::Color::Orange);
                renderer.drawLine(px, py + kCellSize / 2, px + kCellSize - 1, py + kCellSize / 2,
                                   gfx::Color::DarkRed);
            } else if (t == TileType::Exit) {
                renderer.drawRectangle(px, py, kCellSize, kCellSize, gfx::Color::Purple);
                renderer.drawFilledRectangle(px + 4, py + 4, 8, 8, gfx::Color::Purple);
            }
            // TileType::Empty draws nothing. PowerUpFire/PowerUpBomb draw
            // passes are added in Phase 4.
        }
    }
}

}  // namespace bomberman
