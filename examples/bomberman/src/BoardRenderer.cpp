#include "BoardRenderer.h"
#include "math/Vector2.h"
#include "assets/BombSprites.h"

namespace pr32 = pixelroot32;

namespace bomberman {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

BoardRenderer::BoardRenderer(const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs],
                              const uint8_t (&blastSteps)[kCells],
                              const uint8_t (&blastShape)[kCells],
                              const uint8_t (&blastDist)[kCells],
                              const uint8_t (&blastRange)[kCells])
    : core::Entity(math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, core::EntityType::GENERIC),
      board_(board), bombs_(bombs),
      blastSteps_(blastSteps), blastShape_(blastShape),
      blastDist_(blastDist), blastRange_(blastRange) {
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

            const Sprite4bpp* sprite = nullptr;
            switch (t) {
                case TileType::HardWall:    // sheet cell (3,3)
                    sprite = &kHardWallSprite;
                    break;
                case TileType::SoftWall:
                case TileType::SoftWallHidingExit:
                case TileType::SoftWallHidingPowerUp:
                    // Load-bearing: all three variants resolve to the same
                    // descriptor via softWallSpriteFor() (audit §8.3).
                    sprite = softWallSpriteFor(t);
                    break;
                case TileType::Exit:        // sheet cell (11,3)
                    sprite = &kExitSprite;
                    break;
                case TileType::PowerUpFire: // sheet cell (1,14)
                    sprite = &kPowerUpFireSprite;
                    break;
                case TileType::PowerUpBomb: // sheet cell (10,14)
                    sprite = &kPowerUpBombSprite;
                    break;
                case TileType::Empty:
                    break;
            }
            if (sprite != nullptr) {
                renderer.drawSprite(*sprite, px, py, 0, false);
            }
        }
    }

    // Explosion cells — range-aware 16x16 sprites from kCENTER / kARMBASE_<dir>
    // / kARNEXT_<dir> / kTIP<dir>, drawn before the bomb/actor layers so a
    // player (or enemy) dying in a blasted cell shows the explosion for at
    // least one frame instead of hiding whatever was just eliminated there.
    for (int i = 0; i < kCells; ++i) {
        if (blastSteps_[i] == 0) {
            continue;
        }
        const int x = i % kCols;
        const int y = i / kCols;
        const int px = gameplay::cellToWorldX(x, kBoardGrid);
        const int py = gameplay::cellToWorldY(y, kBoardGrid);
        const Sprite4bpp* sprite = explosionSpriteFor(
            blastShape_[i], blastDist_[i], blastRange_[i], blastSteps_[i]);
        if (sprite) {
            renderer.drawSprite(*sprite, px, py, 0, false);
        }
    }

    // Bombs: 3-frame pulse cycle via kBombSprites. During the final
    // kBombFlashSteps, the pulse cadence speeds up to a 4× faster cycle
    // (same 3 frames, divisor 2 instead of 8) to convey urgency.
    for (int i = 0; i < kMaxBombs; ++i) {
        const Bomb& b = bombs_[i];
        if (!b.active) {
            continue;
        }
        const int px = gameplay::cellToWorldX(b.cellX, kBoardGrid);
        const int py = gameplay::cellToWorldY(b.cellY, kBoardGrid);
        const bool inFlash = (b.fuseSteps <= kBombFlashSteps);
        const uint8_t frame = inFlash
            ? static_cast<uint8_t>((b.fuseSteps / 2) % 3)
            : static_cast<uint8_t>((b.fuseSteps / 8) % 3);
        renderer.drawSprite(kBombSprites[frame], px, py, 0, false);
    }
}

}  // namespace bomberman
