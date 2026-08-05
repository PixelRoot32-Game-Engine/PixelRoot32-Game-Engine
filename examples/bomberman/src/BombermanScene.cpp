#include "BombermanScene.h"
#include "core/Engine.h"
#include <cassert>
#include <ctime>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace bomberman {

namespace gfx = pr32::graphics;
namespace core = pr32::core;

BombermanScene::BombermanScene()
    : board_{},
      renderer_(board_),
      player_(kPlayerStartCellX, kPlayerStartCellY),
      state_(LevelState::Loading),
      accumulatorMs_(0),
      seed_(0) {
    addEntity(&renderer_);
    addEntity(&player_);
}

void BombermanScene::init() {
    gfx::setPalette(gfx::PaletteType::PR32);
    startLevel();
}

void BombermanScene::startLevel() {
    state_ = LevelState::Loading;

    seed_ = static_cast<uint32_t>(std::time(nullptr));
    const LevelLayout layout = generateLevel(seed_, kEnemyCount, kSoftWallPercent);
    for (int i = 0; i < kCells; ++i) {
        board_[i] = layout.board[i];
    }
    // layout.enemyCells / layout.enemyCount are already computed here, but
    // this slice has no EnemyActor yet to place them in — wired in Phase 4.

    player_.resetTo(kPlayerStartCellX, kPlayerStartCellY);
    accumulatorMs_ = 0;

    state_ = LevelState::Playing;

    // Guardrail: catches an entity that was dropped or double-added.
    // Compiles out under NDEBUG (esp32dev release builds); the
    // static_assert in BombermanConstants.h is the half that always holds.
    assert(entityCount == 2 && "An entity was dropped or double-added.");
}

void BombermanScene::update(unsigned long deltaTime) {
    accumulatorMs_ += deltaTime;
    int steps = 0;
    while (accumulatorMs_ >= static_cast<unsigned long>(kLogicStepMs) && steps < kMaxLogicStepsPerFrame) {
        accumulatorMs_ -= static_cast<unsigned long>(kLogicStepMs);
        ++steps;
        logicStep();
    }
    if (steps == kMaxLogicStepsPerFrame) {
        // Drop the backlog rather than let a long frame (SD flush, serial
        // log) burst dozens of steps and teleport the player through a
        // wall. deltaTime never reaches a rule function below this point.
        accumulatorMs_ = 0;
    }
}

void BombermanScene::logicStep() {
    auto& input = engine.getInputManager();

    // Stages 1 (input) + 2 (player movement): PlayerActor reads held input
    // directly, since there is nothing else in this slice to arbitrate
    // against. The full nine-stage pipeline (enemies, bombs, explosions,
    // blocks, power-ups, collisions, victory) is assembled incrementally
    // in later phases.
    player_.logicStep(board_, input);
}

void BombermanScene::draw(gfx::Renderer& renderer) {
    // BoardRenderer (layer 0) then PlayerActor (layer 1), in that order —
    // Scene::draw() sorts by render layer. HUD text and overlays are added
    // in Phase 5.
    Scene::draw(renderer);
}

}  // namespace bomberman
