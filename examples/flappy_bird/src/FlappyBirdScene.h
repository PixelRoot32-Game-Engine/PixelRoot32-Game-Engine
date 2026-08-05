#pragma once
#include <core/Scene.h>
#include <gameplay/StateMachine.h>
#include <graphics/Renderer.h>
#include <math/Vector2.h>
#include <platforms/EngineConfig.h>

#include <memory>
#include <vector>

#include "FlappyBirdConstants.h"

namespace flappy {

    class BirdActor;
    class PipeActor;

    /**
     * @class FlappyBirdScene
     * @brief Flappy Bird clone with gravity-based bird and scrolling pipes.
     *
     * Uses RigidActor for the bird (gravity, jump) and KinematicActor for pipes.
     * Pipes are recycled when off-screen for object pooling.
     */
    class FlappyBirdScene : public pixelroot32::core::Scene {
    public:
        FlappyBirdScene();
        ~FlappyBirdScene();
        void init() override;
        void update(unsigned long deltaTime) override;
        void draw(pixelroot32::graphics::Renderer& renderer) override;

    private:
        /// Bound to a caller-owned, static const table (see .cpp). Only
        /// onEnter carries real work here (score/position reset entering
        /// WAITING; first jump + pipe reveal entering RUNNING) — the
        /// WAITING->RUNNING, RUNNING->GAME_OVER, and GAME_OVER->WAITING
        /// transition conditions stay as direct fsm.requestState() calls
        /// from update(), not from onUpdate. See update()'s implementation
        /// comment in the .cpp for why.
        pixelroot32::gameplay::StateMachine fsm;

        std::unique_ptr<BirdActor> bird;
        std::unique_ptr<PipeActor> topPipe;
        std::unique_ptr<PipeActor> bottomPipe;

        int score;              ///< Current score
        int screenWidth;        ///< Display width
        int screenHeight;       ///< Display height
        char scoreStr[8];       ///< Score string buffer

        void createPipes();

        /** @brief Current game state, read back from the state machine. */
        GameState currentState() const;

        // StateMachine::State callbacks (see kFlappyStates in the .cpp).
        // `owner` is always `this` — configure() binds it once in init().
        // Static so they match StateMachine::EnterFn's C function pointer
        // signature; each recovers the instance via
        // `static_cast<FlappyBirdScene*>(owner)`.

        /** @brief WAITING entry: resets the bird position, regenerates the
         *  pipe gap, repositions both pipes, and zeroes the score — the old
         *  resetGame() body, now the state's real entry side effect. */
        static void onEnterWaiting(void* owner, pixelroot32::gameplay::StateId fromState);

        /** @brief RUNNING entry: fires the first jump and reveals the pipes
         *  — the old WAITING-branch transition side effect. */
        static void onEnterRunning(void* owner, pixelroot32::gameplay::StateId fromState);

        /// State table bound in init(). Defined out-of-line in the .cpp
        /// (class-static, not namespace-scope, so its rows can take the
        /// address of the private callbacks above) — still `static const`,
        /// so it lands in flash/.rodata per StateMachine.h's documented
        /// convention.
        static const pixelroot32::gameplay::StateMachine::State kFlappyStates[3];
    };

} // namespace flappy
