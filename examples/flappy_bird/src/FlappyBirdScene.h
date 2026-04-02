#pragma once
#include <core/Scene.h>
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
        GameState currentState;

        std::unique_ptr<BirdActor> bird;
        std::unique_ptr<PipeActor> topPipe;
        std::unique_ptr<PipeActor> bottomPipe;

        int score;              ///< Current score
        int screenWidth;        ///< Display width
        int screenHeight;       ///< Display height
        char scoreStr[8];       ///< Score string buffer

        void resetGame();
        void createPipes();
    };

} // namespace flappy
