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
      enemyCount_(0),
      enemiesAlive_(0),
      state_(LevelState::Loading),
      accumulatorMs_(0),
      seed_(0),
      lives_(kStartingLives),
      bombPressLatched_(false) {
    // enemies_ is default-initialized above (omitted here): every slot
    // starts dead at (0,0) via EnemyActor's default constructor and is not
    // registered with the scene until startLevel() decides how many of
    // this level's slots actually play.
    addEntity(&renderer_);
    addEntity(&player_);
}

void BombermanScene::init() {
    gfx::setPalette(gfx::PaletteType::PR32);
    startLevel();
}

void BombermanScene::startLevel() {
    state_ = LevelState::Loading;

    // Enemy count varies per generated level (BombermanBoard.cpp may spawn
    // fewer than kEnemyCount if the map has too few eligible cells), so
    // unlike the renderer/player, enemies are removed and re-added here
    // rather than added once in the constructor. On the very first call
    // (from init()), enemyCount_ is still 0 and this loop is a no-op.
    for (int i = 0; i < enemyCount_; ++i) {
        removeEntity(&enemies_[i]);
    }

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

    player_.resetTo(kPlayerStartCellX, kPlayerStartCellY);

    enemyCount_ = layout.enemyCount;
    enemiesAlive_ = enemyCount_;
    for (int i = 0; i < enemyCount_; ++i) {
        const int cell = layout.enemyCells[i];
        const int x = cell % kCols;
        const int y = cell / kCols;
        enemies_[i].resetTo(x, y);
        addEntity(&enemies_[i]);
    }

    accumulatorMs_ = 0;
    // A press held across the death that triggered this restart must not
    // drop a bomb on the fresh board's first step.
    bombPressLatched_ = false;

    state_ = LevelState::Playing;

    // Guardrail: catches an entity that was dropped or double-added.
    // Compiles out under NDEBUG (esp32dev release builds); the
    // static_assert in BombermanConstants.h is the half that always holds.
    assert(entityCount == 2 + enemyCount_ && "An entity was dropped or double-added.");
}

void BombermanScene::update(unsigned long deltaTime) {
    // Latch here, at frame scope, while the press edge is still live. The
    // loop below may run no logic step at all this frame.
    if (engine.getInputManager().isButtonPressed(BTN_BOMB)) {
        bombPressLatched_ = true;
    }

    accumulatorMs_ += deltaTime;
    int steps = 0;
    while (accumulatorMs_ >= static_cast<unsigned long>(kLogicStepMs) && steps < kMaxLogicStepsPerFrame) {
        accumulatorMs_ -= static_cast<unsigned long>(kLogicStepMs);
        ++steps;
        logicStep();
    }
    if (steps == kMaxLogicStepsPerFrame) {
        // Drop the backlog rather than let a long frame (SD flush, serial
        // log) burst dozens of steps and teleport actors through a wall.
        // deltaTime never reaches a rule function below this point.
        accumulatorMs_ = 0;
    }
}

void BombermanScene::logicStep() {
    if (state_ == LevelState::StageClear || state_ == LevelState::GameOver) {
        // Terminal state: freeze the simulation. Both end states stop the
        // pipeline the same way; only how they were reached differs.
        return;
    }

    auto& input = engine.getInputManager();

    // Stage 1+2: input + player movement, including bomb placement and
    // bomb-aware blocking (the own-bomb pass-through exemption). Movement
    // reads held buttons straight from the InputManager -- a level state is
    // correct to sample per logic step. The bomb press is the latched
    // frame edge instead, consumed exactly once here.
    const bool bombPressed = bombPressLatched_;
    bombPressLatched_ = false;
    player_.logicStep(board_, bombs_, input, bombPressed);

    // Stage 3: enemy movement. Each alive enemy re-decides its own
    // direction independently, straight-else-PRNG; see EnemyActor.cpp.
    for (int i = 0; i < enemyCount_; ++i) {
        if (enemies_[i].isAlive()) {
            enemies_[i].logicStep(board_, bombs_);
        }
    }

    // Stage 4: bomb fuses tick down, seeding this step's detonation queue
    // with any bomb whose fuse just reached zero.
    uint8_t detonationQueue[kMaxBombs] = {};
    int queueTail = 0;
    tickFuses(bombs_, detonationQueue, queueTail);

    // Stage 5: the queue -- including any bomb a blast arm chain-triggers
    // along the way -- drains to a fixed point within this same call. See
    // resolveDetonations()'s termination comment for why that is
    // guaranteed to happen in at most kMaxBombs iterations.
    resolveDetonations(detonationQueue, queueTail, bombs_, board_, blastSteps_, hiddenPowerUp_);

    // Stage 6: explosion cells count down toward reverting to their
    // post-destruction tile.
    tickExplosions(blastSteps_);

    // Stage 7: power-up pickup. Reads the player's LOGICAL cell every step;
    // idempotent once the tile is cleared to Empty on the same step it is
    // applied, so re-checking a cell the player is simply standing on (no
    // longer a power-up) is a correctly-shaped no-op.
    const int playerCell = cellIndex(player_.cellX(), player_.cellY());
    if (board_[playerCell] == TileType::PowerUpFire) {
        player_.applyFirePowerUp();
        board_[playerCell] = TileType::Empty;
    } else if (board_[playerCell] == TileType::PowerUpBomb) {
        player_.applyBombPowerUp();
        board_[playerCell] = TileType::Empty;
    }

    // Stage 8a: explosion contact against enemies. Checked before the
    // player's own explosion contact so an enemy killed by the same blast
    // that also kills the player is already gone by the time anything else
    // looks at enemiesAlive_ this step.
    for (int i = 0; i < enemyCount_; ++i) {
        if (enemies_[i].isAlive() &&
            blastSteps_[cellIndex(enemies_[i].cellX(), enemies_[i].cellY())] > 0) {
            killEnemy(i);
        }
    }

    // Stage 8b: explosion contact against the player. The player's own
    // death always takes priority over anything stage 9 might otherwise
    // conclude this same step -- returning here skips both the
    // player-enemy contact check below and the victory check, since
    // startLevel()/GameOver have already redefined what "this step" means.
    if (blastSteps_[playerCell] > 0) {
        handlePlayerDeath();
        return;
    }

    // Stage 8c: player-enemy contact. Compares only the LOGICAL cells after
    // BOTH movements have resolved this step (never the interpolated path
    // between them), matching the update order's literal wording. A same-
    // step SWAP -- the player and an enemy exchanging cells in one step --
    // is therefore pass-through, not a collision: their post-movement cells
    // differ, so this check never fires for it. This is a deliberate
    // choice, not an oversight: a swept/segment check would need continuous
    // collision math this example otherwise has no reason to carry.
    for (int i = 0; i < enemyCount_; ++i) {
        if (enemies_[i].isAlive() && enemies_[i].cellX() == player_.cellX() &&
            enemies_[i].cellY() == player_.cellY()) {
            handlePlayerDeath();
            return;
        }
    }

    // Stage 9: victory check. Both conditions are plain `if`s, not
    // `else if` -- so the case where the LAST enemy dies (stage 8a, this
    // same call) while the player is ALREADY standing on the revealed exit
    // cell resolves straight through to StageClear in this very call,
    // instead of requiring the player to step off and back onto the exit.
    if (state_ == LevelState::Playing && enemiesAlive_ == 0) {
        state_ = LevelState::ExitUnlocked;
    }
    if (state_ == LevelState::ExitUnlocked && board_[playerCell] == TileType::Exit) {
        state_ = LevelState::StageClear;
    }
}

void BombermanScene::killEnemy(int index) {
    enemies_[index].kill();
    removeEntity(&enemies_[index]);
    --enemiesAlive_;
}

void BombermanScene::handlePlayerDeath() {
    --lives_;
    if (lives_ > 0) {
        // lives_ lives outside startLevel() entirely, so it carries the
        // decremented value over instead of resetting.
        startLevel();
    } else {
        state_ = LevelState::GameOver;
    }
}

void BombermanScene::draw(gfx::Renderer& renderer) {
    // BoardRenderer (layer 0), then PlayerActor + live EnemyActors
    // (layer 1) -- Scene::draw() sorts by render layer. HUD text and
    // overlays are later work.
    Scene::draw(renderer);
}

}  // namespace bomberman
