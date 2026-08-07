#include "BomberbotScene.h"
#include "core/Engine.h"
#include "audio/AudioTypes.h"
#include "assets/BomberbotPalette.h"
#include "graphics/Color.h"
#include <cassert>
#include <cstdio>
#include <ctime>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace bomberbot {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace audio = pr32::audio;

BomberbotScene::BomberbotScene()
    : board_{},
      bombs_{},
      blastSteps_{},
      blastShape_{},
      blastDist_{},
      blastRange_{},
      hiddenPowerUp_(TileType::PowerUpFire),
      renderer_(board_, bombs_, blastSteps_, blastShape_, blastDist_, blastRange_),
      player_(kPlayerStartCellX, kPlayerStartCellY),
      enemyCount_(0),
      enemiesAlive_(0),
      state_(LevelState::Loading),
      accumulatorMs_(0),
      seed_(0),
      lives_(kStartingLives),
      countdownSeconds_(kInitialCountdownSeconds),
      countdownSubSteps_(0),
      bombPressLatched_(false),
      restartPressLatched_(false) {
    // enemies_ is default-initialized above (omitted here): every slot
    // starts dead at (0,0) via EnemyActor's default constructor and is not
    // registered with the scene until startLevel() decides how many of
    // this level's slots actually play.
    addEntity(&renderer_);
    addEntity(&player_);
}

void BomberbotScene::init() {
    // Sprite palette must be registered BEFORE setPalette and BEFORE the
    // first draw. Phase 1 of the asset migration; see audit §6 + §8.3.
    // The palette invariants (16 entries, index 0 transparent) are asserted
    // at runtime by test_bomberbot_phase1_wiring / test_bomberbot_phase0_assets.
    // A compile-time static_assert was attempted but the palette array is
    // static const (not constexpr), so it cannot be evaluated at compile time.
    gfx::setDualCustomPalette(BOMBERBOT_SPRITE_PALETTE_RGB565, BOMBERBOT_SPRITE_PALETTE_RGB565);
    startLevel();
}

void BomberbotScene::startLevel() {
    state_ = LevelState::Loading;

    // Enemy count varies per generated level (BomberbotBoard.cpp may spawn
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
        blastShape_[i] = 0;
        blastDist_[i] = 0;
        blastRange_[i] = 0;
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

    countdownSeconds_ = kInitialCountdownSeconds;
    countdownSubSteps_ = 0;

    accumulatorMs_ = 0;
    // A press held across the death that triggered this restart must not
    // drop a bomb on the fresh board's first step.
    bombPressLatched_ = false;

    state_ = LevelState::Playing;

    // Guardrail: catches an entity that was dropped or double-added.
    // Compiles out under NDEBUG (esp32dev release builds); the
    // static_assert in BomberbotConstants.h is the half that always holds.
    assert(entityCount == 2 + enemyCount_ && "An entity was dropped or double-added.");
}

void BomberbotScene::update(unsigned long deltaTime) {
    // Latch here, at frame scope, while the press edge is still live. The
    // loop below may run no logic step at all this frame. Both button
    // presses this example ever reads (bomb, restart) are latched exactly
    // this way -- isButtonPressed() is never called from inside logicStep(),
    // which runs on the fixed 20 ms clock instead of the frame clock.
    auto& input = engine.getInputManager();
    if (input.isButtonPressed(BTN_BOMB)) {
        bombPressLatched_ = true;
    }
    if (input.isButtonPressed(BTN_RESTART)) {
        restartPressLatched_ = true;
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

void BomberbotScene::logicStep() {
    // The restart press is consumed here, once per logic step, in EVERY
    // state -- not only in the terminal branch below. Latching in update()
    // is what keeps the read on the frame clock; clearing it unconditionally
    // is what keeps it from going stale. A press made during normal play is
    // meaningless, so it is discarded rather than left armed: leaving it set
    // would mean one stray press at any point in a session sits pending and
    // then fires on the very first step after the game ends, skipping the
    // game-over and stage-clear states entirely. The bomb latch below is
    // safe for the same reason -- it is consumed every step, never only in
    // the state that happens to act on it.
    const bool restartPressed = restartPressLatched_;
    restartPressLatched_ = false;

    if (state_ == LevelState::StageClear || state_ == LevelState::GameOver) {
        // Terminal state: freeze the simulation, except for a restart
        // request.
        if (restartPressed) {
            lives_ = kStartingLives;
            startLevel();
        }
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
    if (player_.logicStep(board_, bombs_, input, bombPressed)) {
        audio::AudioEvent bombPlacedSound;
        bombPlacedSound.type = audio::WaveType::PULSE;
        bombPlacedSound.frequency = 220.0f;
        bombPlacedSound.duration = 0.08f;
        bombPlacedSound.volume = 0.5f;
        bombPlacedSound.duty = 0.5f;
        engine.getAudioEngine().playEvent(bombPlacedSound);
    }

    // Stage 3: enemy movement. Each alive enemy re-decides its own
    // direction independently, straight-else-PRNG; see EnemyActor.cpp.
    for (int i = 0; i < enemyCount_; ++i) {
        if (enemies_[i].isAlive()) {
            enemies_[i].logicStep(board_, bombs_);
        }
    }

    // Stage 4: bomb fuses tick down, seeding this step's detonation queue
    // with any bomb whose fuse just reached zero. The level countdown ticks
    // in this same stage, on the identical fixed 20 ms clock -- never from
    // update()'s deltaTime -- so it is exactly as reproducible as the fuses
    // it shares a pipeline stage with.
    uint8_t detonationQueue[kMaxBombs] = {};
    int queueTail = 0;
    tickFuses(bombs_, detonationQueue, queueTail);

    if (countdownSeconds_ > 0) {
        ++countdownSubSteps_;
        if (countdownSubSteps_ >= kStepsPerSecond) {
            countdownSubSteps_ = 0;
            --countdownSeconds_;
            if (countdownSeconds_ == 0) {
                // Expiry costs a life via the exact same funnel as an
                // explosion or an enemy contact. handlePlayerDeath() may
                // call startLevel(), which resets board_/bombs_/blastSteps_
                // -- so this returns immediately rather than letting stages
                // 5+ below run resolveDetonations()/tickExplosions() against
                // a detonation queue seeded for a board that no longer
                // exists.
                handlePlayerDeath();
                return;
            }
        }
    }

    // Stage 5: the queue -- including any bomb a blast arm chain-triggers
    // along the way -- drains to a fixed point within this same call. See
    // resolveDetonations()'s termination comment for why that is
    // guaranteed to happen in at most kMaxBombs iterations.
    const int detonatedCount =
        resolveDetonations(detonationQueue, queueTail, bombs_, board_,
                           blastSteps_, blastShape_, blastDist_, blastRange_,
                           hiddenPowerUp_);
    if (detonatedCount > 0) {
        audio::AudioEvent explosionSound;
        explosionSound.type = audio::WaveType::NOISE;
        explosionSound.frequency = 90.0f;
        explosionSound.duration = 0.3f;
        explosionSound.volume = 0.7f;
        explosionSound.duty = 0.5f;
        engine.getAudioEngine().playEvent(explosionSound);
    }

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
        audio::AudioEvent stageClearSound;
        stageClearSound.type = audio::WaveType::TRIANGLE;
        stageClearSound.frequency = 660.0f;
        stageClearSound.duration = 0.6f;
        stageClearSound.volume = 0.7f;
        stageClearSound.duty = 0.5f;
        engine.getAudioEngine().playEvent(stageClearSound);
    }
}

void BomberbotScene::killEnemy(int index) {
    enemies_[index].kill();
    removeEntity(&enemies_[index]);
    --enemiesAlive_;
}

void BomberbotScene::handlePlayerDeath() {
    // The single funnel for every death cause (explosion contact, enemy
    // contact, countdown expiry) -- one audio event fires here regardless
    // of which one triggered it.
    audio::AudioEvent deathSound;
    deathSound.type = audio::WaveType::SAW;
    deathSound.frequency = 300.0f;
    deathSound.duration = 0.4f;
    deathSound.volume = 0.7f;
    deathSound.duty = 0.5f;
    engine.getAudioEngine().playEvent(deathSound);

    --lives_;
    if (lives_ > 0) {
        // lives_ lives outside startLevel() entirely, so it carries the
        // decremented value over instead of resetting.
        startLevel();
    } else {
        state_ = LevelState::GameOver;
    }
}

void BomberbotScene::draw(gfx::Renderer& renderer) {
    // BoardRenderer (layer 0), then PlayerActor + live EnemyActors
    // (layer 1) -- Scene::draw() sorts by render layer. HUD text and the
    // game-over/stage-clear overlay are drawn on top, here, same idiom as
    // the board's status band (BoardRenderer draws the band's background;
    // this draws the text that sits on it).
    Scene::draw(renderer);

    char buffer[24];

    std::snprintf(buffer, sizeof(buffer), "LIVES %d  ENEMIES %d", lives_, enemiesAlive_);
    renderer.drawText(buffer, 4, 6, gfx::Color::White, 1);

    std::snprintf(buffer, sizeof(buffer), "F%d B%d  TIME %03u", player_.firePower(), player_.maxBombs(),
                  static_cast<unsigned>(countdownSeconds_));
    renderer.drawText(buffer, 4, 22, gfx::Color::White, 1);

    // Text overlay, not a separate screen or Scene: both terminal states
    // draw a dark band across the middle of the board with centered text
    // over whatever the board looks like at the moment of the transition.
    if (state_ == LevelState::GameOver) {
        renderer.drawFilledRectangle(0, 104, DISPLAY_WIDTH, 32, gfx::Color::Black);
        renderer.drawTextCentered("GAME OVER", 112, gfx::Color::Red, 1);
        renderer.drawTextCentered("PRESS RESTART", 124, gfx::Color::Red, 1);
    } else if (state_ == LevelState::StageClear) {
        renderer.drawFilledRectangle(0, 104, DISPLAY_WIDTH, 32, gfx::Color::Black);
        renderer.drawTextCentered("STAGE CLEAR", 112, gfx::Color::Cyan, 1);
        renderer.drawTextCentered("PRESS RESTART", 124, gfx::Color::Cyan, 1);
    }
}

}  // namespace bomberbot
