#include <core/Engine.h>
#include <physics/CollisionSystem.h>

#include "FlappyBirdScene.h"
#include "BirdActor.h"
#include "PipeActor.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdlib>
#define random(min, max) (min + (std::rand() % (max - min)))
#endif
#include <cstdio>

extern pixelroot32::core::Engine engine;

namespace flappy {
    
using pixelroot32::math::toScalar;
using pixelroot32::math::Scalar;
using pixelroot32::math::Vector2;

FlappyBirdScene::FlappyBirdScene() = default;
FlappyBirdScene::~FlappyBirdScene() = default;

void FlappyBirdScene::init() {
    screenWidth = DISPLAY_WIDTH;
    screenHeight = DISPLAY_HEIGHT;

    bird = std::make_unique<BirdActor>(Vector2(toScalar(BIRD_START_X), toScalar(screenHeight / 2.0f)));
    bird->isVisible = true; // Always visible
    addEntity(bird.get());
    
    createPipes();
    
    resetGame();
}

void FlappyBirdScene::createPipes() {
    topPipe = std::make_unique<PipeActor>(Vector2(toScalar(screenWidth), toScalar(0)), PIPE_WIDTH, 20, true);
    bottomPipe = std::make_unique<PipeActor>(Vector2(toScalar(screenWidth), toScalar(40)), PIPE_WIDTH, 20, false);
    
    topPipe->isVisible = false;
    bottomPipe->isVisible = false;
    
    addEntity(topPipe.get());
    addEntity(bottomPipe.get());
}

void FlappyBirdScene::resetGame() {
    bird->reset(Vector2(toScalar(BIRD_START_X), toScalar(screenHeight / 2.0f)));

    int maxGapY = screenHeight - PIPE_GAP - 10;
    int minGapY = 10;
    if (maxGapY <= minGapY) maxGapY = minGapY + 1;
    int pipeGapY = random(minGapY, maxGapY);
    
    topPipe->position = Vector2(toScalar(screenWidth), toScalar(0));
    topPipe->setSize(PIPE_WIDTH, pipeGapY);
    topPipe->markPassed();
    topPipe->markPassed();

    bottomPipe->position = Vector2(toScalar(screenWidth), toScalar(pipeGapY + PIPE_GAP));
    bottomPipe->setSize(PIPE_WIDTH, screenHeight - (pipeGapY + PIPE_GAP));

    currentState = flappy::GameState::WAITING;
    score = 0;
    std::snprintf(scoreStr, sizeof(scoreStr), "0");
}

void FlappyBirdScene::update(unsigned long deltaTime) {
    if (currentState == flappy::GameState::RUNNING) {
        pixelroot32::core::Scene::update(deltaTime);
    }
    
    auto& input = engine.getInputManager();

    if (input.isButtonPressed(BTN_JUMP)) {
        if (currentState == flappy::GameState::WAITING) {
            currentState = flappy::GameState::RUNNING;
            bird->jump();
            topPipe->isVisible = true;
            bottomPipe->isVisible = true;
        } else if (currentState == flappy::GameState::RUNNING) {
            bird->jump();
        } else if (currentState == flappy::GameState::GAME_OVER) {
            resetGame();
        }
    }

    if (currentState == flappy::GameState::RUNNING) {
        if (bird->isDead()) {
            currentState = flappy::GameState::GAME_OVER;
        }

        int maxGapY = screenHeight - PIPE_GAP - 10;
        int minGapY = 10;
        if (maxGapY <= minGapY) maxGapY = minGapY + 1;
        
        int newGapY = random(minGapY, maxGapY);
        if (topPipe->resetIfOffScreen(screenWidth, screenHeight, newGapY)) {
            bottomPipe->position.x = topPipe->position.x;
            bottomPipe->position.y = toScalar(newGapY + PIPE_GAP);
            bottomPipe->setSize(PIPE_WIDTH, screenHeight - (newGapY + PIPE_GAP));
        }

        if (!topPipe->hasPassed() && topPipe->isPassed(bird->position)) {
            topPipe->markPassed();
            score++;
            std::snprintf(scoreStr, sizeof(scoreStr), "%d", score);
        }

        if (static_cast<float>(bird->position.y) < 0 || 
            static_cast<float>(bird->position.y) + bird->height > screenHeight) {
            currentState = flappy::GameState::GAME_OVER;
        }
    }
}

void FlappyBirdScene::draw(pixelroot32::graphics::Renderer& renderer) {
    using Color = pixelroot32::graphics::Color;

    pixelroot32::core::Scene::draw(renderer);
    renderer.drawText(scoreStr, screenWidth - SCORE_X_OFFSET, SCORE_Y_OFFSET, Color::White, 1);

    if (currentState == flappy::GameState::WAITING) {
        renderer.drawTextCentered("START", 25, Color::White, 1);
    } else if (currentState == flappy::GameState::GAME_OVER) {
        renderer.drawTextCentered("GAME OVER", 20, Color::White, 1);
        renderer.drawTextCentered("RESTART", 45, Color::White, 1);
    }
}
}