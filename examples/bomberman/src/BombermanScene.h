#pragma once
#include <cstdint>
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"
#include "BoardRenderer.h"
#include "BombermanBoard.h"
#include "BombermanBombs.h"
#include "BombermanConstants.h"
#include "EnemyActor.h"
#include "PlayerActor.h"

namespace bomberman {

/**
 * @class BombermanScene
 * @brief Scene shell: fixed-step drain, the full nine-stage logic pipeline,
 *        board/bomb/enemy state, and the level lifecycle.
 */
class BombermanScene : public pixelroot32::core::Scene {
public:
    BombermanScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    /// Level lifecycle, five states. This is a PLAIN enum, not the engine's
    /// generic per-class state-machine primitive, and that is deliberate:
    /// every transition below is a condition checked in a fixed order once
    /// per logic step (see logicStep()), not a per-state callback dispatched
    /// by some external driver. A generic state-machine type would only be
    /// buying a dispatch table this game barely uses -- exactly one state
    /// (Loading) does any real one-time entry work, and that work is
    /// already a single named function (startLevel()) called from exactly
    /// two call sites. Before reaching for a state-machine primitive here,
    /// re-derive that tradeoff from scratch; it does not hold for this
    /// shape of update loop.
    enum class LevelState : uint8_t { Loading, Playing, ExitUnlocked, StageClear, GameOver };

    TileType board_[kCells];
    Bomb bombs_[kMaxBombs];
    uint8_t blastSteps_[kCells];
    TileType hiddenPowerUp_;
    BoardRenderer renderer_;
    PlayerActor player_;

    /// Fixed pool of enemy slots, sized to kMaxEnemies (BombermanConstants.h)
    /// and constructed once, here, never destroyed. Only the first
    /// enemyCount_ slots are ever added to the scene or touched by
    /// logicStep()/draw() for the CURRENT level; the rest sit unused and
    /// unregistered, exactly like the entity guardrail's static_assert
    /// assumes.
    EnemyActor enemies_[kMaxEnemies];
    int enemyCount_;    ///< Total enemies spawned this level (may be < kEnemyCount; see BombermanBoard.cpp step 6).
    int enemiesAlive_;  ///< Live count; ExitUnlocked is entered the step this reaches 0.

    LevelState state_;
    unsigned long accumulatorMs_;
    uint32_t seed_;
    int lives_;

    /// Frame-scoped bomb press, latched for the fixed-step loop to consume.
    /// InputManager recomputes its press edge once per engine frame, but the
    /// accumulator below runs zero, one, or two logic steps in that same
    /// frame. Reading the edge from inside a logic step therefore drops any
    /// press that lands on a zero-step frame -- at a 20 ms step and a frame
    /// faster than 50 Hz, that is a routine occurrence, not a rare one. The
    /// latch is set where the edge is live (update(), frame scope) and
    /// cleared by the first logic step that consumes it, so a press is
    /// neither lost on a zero-step frame nor acted on twice on a two-step
    /// one.
    bool bombPressLatched_;

    /// (Re)generates the board, resets the bomb pool/explosion mask, resets
    /// the player, and redeploys enemies to match the freshly generated
    /// layout's (possibly different) enemy count. Deliberately does NOT
    /// call the base Scene::init(), since Scene::resetState() clears every
    /// entity and the renderer/player are added once, in the constructor,
    /// and never re-added. Enemies ARE removed and re-added here, because
    /// their count varies per generated level, unlike the renderer/player.
    /// Deliberately does NOT touch `lives_`: that field lives outside this
    /// function entirely so a death-with-lives-remaining restart carries
    /// the decremented value over instead of resetting it.
    void startLevel();

    /// The fixed nine-stage logic pipeline, driven by update()'s
    /// accumulator: (1) input, (2) player movement, (3) enemy movement,
    /// (4) bomb fuse tick, (5) chain-reaction drain, (6) explosion decay,
    /// (7) power-up pickup, (8) collision detection (explosion contact for
    /// both enemies and the player, then player-enemy contact), (9) victory
    /// check. A "cycle" in this codebase always means one fixed 20 ms logic
    /// step (BombermanConstants.h's kLogicStepMs) -- never a rendered frame,
    /// which runs at a different, variable rate.
    void logicStep();

    /// Removes an enemy from play: marks it dead, unregisters it from the
    /// scene's entity list (so it stops drawing and stops being iterated by
    /// future logic steps), and decrements the live count that
    /// exit-gating/victory reads.
    void killEnemy(int index);

    /// One life lost to an explosion or an enemy. Restarts the level if
    /// lives remain (decremented value carried over); otherwise enters
    /// GameOver, which logicStep()'s own top-of-function guard freezes.
    void handlePlayerDeath();
};

}  // namespace bomberman
