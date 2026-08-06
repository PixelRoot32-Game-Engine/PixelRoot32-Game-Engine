#include "BoardRenderer.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace bomberman {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

BoardRenderer::BoardRenderer(const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs],
                              const uint8_t (&blastSteps)[kCells])
    : core::Entity(math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, core::EntityType::GENERIC),
      board_(board), bombs_(bombs), blastSteps_(blastSteps) {
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
            } else if (t == TileType::PowerUpFire) {
                renderer.drawFilledCircle(px + kCellSize / 2, py + kCellSize / 2, 5, gfx::Color::LightRed);
                renderer.drawCircle(px + kCellSize / 2, py + kCellSize / 2, 6, gfx::Color::Yellow);
            } else if (t == TileType::PowerUpBomb) {
                renderer.drawFilledCircle(px + kCellSize / 2, py + kCellSize / 2, 5, gfx::Color::Blue);
                renderer.drawCircle(px + kCellSize / 2, py + kCellSize / 2, 6, gfx::Color::Cyan);
            }
            // TileType::Empty draws nothing.
        }
    }

    // Explosion cells, drawn before the bomb/actor layers so a player (or,
    // later, an enemy) dying in a blasted cell shows the explosion for at
    // least one frame instead of hiding whatever was just eliminated
    // there.
    for (int i = 0; i < kCells; ++i) {
        if (blastSteps_[i] == 0) {
            continue;
        }
        const int x = i % kCols;
        const int y = i / kCols;
        const int px = gameplay::cellToWorldX(x, kBoardGrid);
        const int py = gameplay::cellToWorldY(y, kBoardGrid);
        renderer.drawFilledRectangle(px + 2, py + 2, kCellSize - 4, kCellSize - 4, gfx::Color::Yellow);
        renderer.drawFilledCircle(px + kCellSize / 2, py + kCellSize / 2, 4, gfx::Color::Orange);
    }

    // Bombs: a body that alternates colour every 5 steps during the final
    // flash window, plus a short fuse line.
    for (int i = 0; i < kMaxBombs; ++i) {
        const Bomb& b = bombs_[i];
        if (!b.active) {
            continue;
        }
        const int px = gameplay::cellToWorldX(b.cellX, kBoardGrid);
        const int py = gameplay::cellToWorldY(b.cellY, kBoardGrid);
        const int cx = px + kCellSize / 2;
        const int cy = py + kCellSize / 2;
        bool flash = false;
        if (b.fuseSteps <= kBombFlashSteps) {
            flash = ((b.fuseSteps / 5) % 2) == 0;
        }
        renderer.drawFilledCircle(cx, cy, 6, flash ? gfx::Color::LightRed : gfx::Color::Black);
        renderer.drawLine(cx, cy - 6, cx + 3, cy - 10, gfx::Color::White);
    }
}

}  // namespace bomberman
