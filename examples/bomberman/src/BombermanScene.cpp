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
      bombs_{},
      blastSteps_{},
      hiddenPowerUp_(TileType::PowerUpFire),
      renderer_(board_, bombs_, blastSteps_),
      player_(kPlayerStartCellX, kPlayerStartCellY),
      state_(LevelState::Loading),
      accumulatorMs_(0),
      seed_(0),
      lives_(kStartingLives) {
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
        blastSteps_[i] = 0;
    }
    hiddenPowerUp_ = layout.hiddenPowerUp;
    for (int i = 0; i < kMaxBombs; ++i) {
        bombs_[i] = Bomb{};
    }
    // layout.enemyCells / layout.enemyCount are already computed here, but
    // this slice has no EnemyActor yet to place them in — wired in later
    // work.

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
    if (lives_ <= 0) {
        // Every life is spent. A full game-over state with its own overlay
        // is later work; this slice only needs "dying at zero lives does
        // not restart", which freezing the simulation here satisfies
        // without inventing that state early.
        return;
    }

    auto& input = engine.getInputManager();

    // Input + player movement, including bomb placement (the player's own
    // action, read inside PlayerActor::logicStep) and bomb-aware blocking
    // (the own-bomb pass-through exemption).
    player_.logicStep(board_, bombs_, input);

    // Bomb fuses tick down, seeding this step's detonation queue with any
    // bomb whose fuse just reached zero.
    uint8_t detonationQueue[kMaxBombs] = {};
    int queueTail = 0;
    tickFuses(bombs_, detonationQueue, queueTail);

    // The queue — including any bomb a blast arm chain-triggers along the
    // way — drains to a fixed point within this same call. See
    // resolveDetonations()'s termination comment for why that is
    // guaranteed to happen in at most kMaxBombs iterations.
    resolveDetonations(detonationQueue, queueTail, bombs_, board_, blastSteps_, hiddenPowerUp_);

    // Explosion cells count down toward reverting to their
    // post-destruction tile.
    tickExplosions(blastSteps_);

    // Instant elimination: the player's LOGICAL cell — never the
    // interpolated visual position — is what every rule reads.
    if (blastSteps_[cellIndex(player_.cellX(), player_.cellY())] > 0) {
        handlePlayerDeath();
    }

    // The full nine-stage pipeline (enemies, power-ups, victory) is
    // assembled incrementally in later work.
}

void BombermanScene::handlePlayerDeath() {
    --lives_;
    if (lives_ > 0) {
        // lives_ lives outside startLevel() entirely, so it carries the
        // decremented value over instead of resetting.
        startLevel();
    }
    // else: lives_ reached zero. logicStep()'s guard above freezes the
    // simulation from the next call onward.
}

void BombermanScene::draw(gfx::Renderer& renderer) {
    // BoardRenderer (layer 0) then PlayerActor (layer 1), in that order —
    // Scene::draw() sorts by render layer. HUD text and overlays are
    // later work.
    Scene::draw(renderer);
}

}  // namespace bomberman
