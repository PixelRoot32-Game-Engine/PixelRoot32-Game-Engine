#pragma once
#include <cstdint>
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"
#include "BoardRenderer.h"
#include "BomberbotBoard.h"
#include "BomberbotBombs.h"
#include "BomberbotConstants.h"
#include "EnemyActor.h"
#include "PlayerActor.h"

namespace bomberbot {

/**
 * @class BomberbotScene
 * @brief Scene shell: fixed-step drain, the full nine-stage logic pipeline,
 *        board/bomb/enemy state, and the level lifecycle.
 */
class BomberbotScene : public pixelroot32::core::Scene {
public:
    BomberbotScene();

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
    uint8_t blastShape_[kCells] = {};  ///< Observation-only per audit §8.5; written by resolveDetonations alongside blastSteps_.
    uint8_t blastDist_[kCells]  = {};  ///< Distance from center for each blast cell (0=center, 1..range for arm cells). Observation-only.
    uint8_t blastRange_[kCells] = {};  ///< Range of the bomb that owns each blast cell. Used with blastDist_ to pick the tip sprite. Observation-only.
    TileType hiddenPowerUp_;
    BoardRenderer renderer_;
    PlayerActor player_;

    /// Fixed pool of enemy slots, sized to kMaxEnemies (BomberbotConstants.h)
    /// and constructed once, here, never destroyed. Only the first
    /// enemyCount_ slots are ever added to the scene or touched by
    /// logicStep()/draw() for the CURRENT level; the rest sit unused and
    /// unregistered, exactly like the entity guardrail's static_assert
    /// assumes.
    EnemyActor enemies_[kMaxEnemies];
    int enemyCount_;    ///< Total enemies spawned this level (may be < kEnemyCount; see BomberbotBoard.cpp step 6).
    int enemiesAlive_;  ///< Live count; ExitUnlocked is entered the step this reaches 0.

    LevelState state_;
    unsigned long accumulatorMs_;
    uint32_t seed_;
    int lives_;

    /// Level countdown: seconds remaining plus how many logic steps have
    /// elapsed into the current second. Both tick inside logicStep()'s
    /// bomb/timer stage, on the fixed 20 ms clock -- never from
    /// update()'s deltaTime -- so the countdown is exactly as reproducible
    /// as every other timer in this game (fuses, explosion decay). Reaching
    /// zero costs the player one life through handlePlayerDeath(), the same
    /// funnel explosion and enemy contact already use.
    uint16_t countdownSeconds_;
    uint8_t countdownSubSteps_;

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

    /// Same latch pattern as bombPressLatched_ above, for the restart button
    /// acted on during GameOver/StageClear. Both isButtonPressed() reads in
    /// this codebase live in update(), at frame scope -- logicStep() only
    /// ever consumes an already-latched bool, never the raw edge, so the
    /// fixed-step clock never observes a press directly.
    ///
    /// Consumed on EVERY logic step, not only in the states that act on it.
    /// A latch cleared only by the state that uses it is a latch that goes
    /// stale: a press during normal play would stay armed for the rest of
    /// the session and then fire on the first step after the game ended,
    /// skipping the very state it was meant to be read in.
    bool restartPressLatched_;

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
    /// (4) bomb fuse tick (the level countdown ticks in this same stage),
    /// (5) chain-reaction drain, (6) explosion decay, (7) power-up pickup,
    /// (8) collision detection (explosion contact for both enemies and the
    /// player, then player-enemy contact), (9) victory check. A "cycle" in
    /// this codebase always means one fixed 20 ms logic step
    /// (BomberbotConstants.h's kLogicStepMs) -- never a rendered frame,
    /// which runs at a different, variable rate. While the level is in a
    /// terminal state (StageClear/GameOver) this function only checks for a
    /// latched restart press and otherwise returns immediately -- the nine
    /// stages above never run against a frozen level.
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

}  // namespace bomberbot
