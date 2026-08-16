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

// Game-state table for pixelroot32::gameplay::StateMachine. Only onEnter
// carries real work: WAITING resets the score/position (the old
// resetGame() body) and RUNNING fires the first jump and reveals the pipes
// (the old WAITING-branch transition side effect). GAME_OVER has no
// entry/exit work of its own — the "GAME OVER"/"RESTART" text is a
// per-frame draw() query, not a one-time entry action — so its row leaves
// every callback null. Defined class-static (not namespace-scope) so its
// rows can take the address of the private callback member functions
// above — still `static const`, so it lands in flash/.rodata per
// StateMachine.h's documented convention.
const pixelroot32::gameplay::StateMachine::State FlappyBirdScene::kFlappyStates[3] = {
    { &FlappyBirdScene::onEnterWaiting, nullptr, nullptr, static_cast<pixelroot32::gameplay::StateId>(GameState::WAITING) },
    { &FlappyBirdScene::onEnterRunning, nullptr, nullptr, static_cast<pixelroot32::gameplay::StateId>(GameState::RUNNING) },
    { nullptr,                          nullptr, nullptr, static_cast<pixelroot32::gameplay::StateId>(GameState::GAME_OVER) },
};

FlappyBirdScene::FlappyBirdScene() = default;
FlappyBirdScene::~FlappyBirdScene() = default;

void FlappyBirdScene::init() {
    screenWidth = DISPLAY_WIDTH;
    screenHeight = DISPLAY_HEIGHT;

    bird = std::make_unique<BirdActor>(Vector2(toScalar(BIRD_START_X), toScalar(screenHeight / 2.0f)));
    bird->isVisible = true; // Always visible
    addEntity(bird.get());

    createPipes();

    fsm.configure(this, kFlappyStates, sizeof(kFlappyStates) / sizeof(kFlappyStates[0]));
    fsm.start(static_cast<pixelroot32::gameplay::StateId>(GameState::WAITING));
}

void FlappyBirdScene::createPipes() {
    // Spawn pipes completely off-screen (x = screenWidth + PIPE_WIDTH)
    // to prevent collision before they scroll into view
    topPipe = std::make_unique<PipeActor>(Vector2(toScalar(screenWidth + PIPE_WIDTH), toScalar(0)), PIPE_WIDTH, 20, true);
    bottomPipe = std::make_unique<PipeActor>(Vector2(toScalar(screenWidth + PIPE_WIDTH), toScalar(40)), PIPE_WIDTH, 20, false);
    
    topPipe->isVisible = false;
    bottomPipe->isVisible = false;
    
    addEntity(topPipe.get());
    addEntity(bottomPipe.get());
}

GameState FlappyBirdScene::currentState() const {
    return static_cast<GameState>(fsm.getCurrentState());
}

void FlappyBirdScene::onEnterWaiting(void* owner, pixelroot32::gameplay::StateId fromState) {
    (void)fromState;
    auto* self = static_cast<FlappyBirdScene*>(owner);

    self->bird->reset(Vector2(toScalar(BIRD_START_X), toScalar(self->screenHeight / 2.0f)));

    // Calculate pipe gap position
    int maxGapY = self->screenHeight - PIPE_GAP - 10;
    int minGapY = 10;
    if (maxGapY <= minGapY) maxGapY = minGapY + 1;
    int pipeGapY = random(minGapY, maxGapY);

    self->topPipe->position = Vector2(toScalar(self->screenWidth), toScalar(0));
    self->topPipe->setSize(PIPE_WIDTH, pipeGapY);
    self->topPipe->markPassed();
    self->topPipe->markPassed();

    self->bottomPipe->position = Vector2(toScalar(self->screenWidth), toScalar(pipeGapY + PIPE_GAP));
    self->bottomPipe->setSize(PIPE_WIDTH, self->screenHeight - (pipeGapY + PIPE_GAP));

    self->score = 0;
    std::snprintf(self->scoreStr, sizeof(self->scoreStr), "0");
}

void FlappyBirdScene::onEnterRunning(void* owner, pixelroot32::gameplay::StateId fromState) {
    (void)fromState;
    auto* self = static_cast<FlappyBirdScene*>(owner);
    self->bird->jump();
    self->topPipe->isVisible = true;
    self->bottomPipe->isVisible = true;
}

// Physics (Scene::update) and the pipe scroll/scoring/bounds-check block
// below both gate on fsm.getCurrentState() directly rather than on
// fsm.update()/onUpdate, and requestState() is called directly from here
// rather than from inside an onUpdate callback. This mirrors the original
// code's two-snapshot shape on purpose: the physics gate reads the
// PRE-input state (skipping physics on the very frame WAITING transitions
// to RUNNING, same as before), while the gameplay-logic gate below reads
// the POST-input state, so it still runs on that same transition frame.
// StateMachine::update() deliberately does not cascade onUpdate into a
// newly-entered state within the same call (see StateMachine.h's ordering
// warning) — routing this through onUpdate would delay the pipe
// scroll/scoring/bounds-check by one frame on the WAITING->RUNNING
// transition frame, a behavior change this migration must avoid. Per that
// same warning, getTimeInState() is never read here either.
void FlappyBirdScene::update(unsigned long deltaTime) {
    if (currentState() == GameState::RUNNING) {
        pixelroot32::core::Scene::update(deltaTime);
    }

    auto& input = engine.getInputManager();

    if (input.isButtonPressed(BTN_JUMP)) {
        if (currentState() == GameState::WAITING) {
            fsm.requestState(static_cast<pixelroot32::gameplay::StateId>(GameState::RUNNING));
        } else if (currentState() == GameState::RUNNING) {
            bird->jump();
        } else if (currentState() == GameState::GAME_OVER) {
            fsm.requestState(static_cast<pixelroot32::gameplay::StateId>(GameState::WAITING));
        }
    }

    if (currentState() == GameState::RUNNING) {
        if (bird->isDead()) {
            fsm.requestState(static_cast<pixelroot32::gameplay::StateId>(GameState::GAME_OVER));
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
            fsm.requestState(static_cast<pixelroot32::gameplay::StateId>(GameState::GAME_OVER));
        }
    }
}

void FlappyBirdScene::draw(pixelroot32::graphics::Renderer& renderer) {
    using Color = pixelroot32::graphics::Color;

    pixelroot32::core::Scene::draw(renderer);
    renderer.drawText(scoreStr, screenWidth - SCORE_X_OFFSET, SCORE_Y_OFFSET, Color::White, 1);

    if (currentState() == GameState::WAITING) {
        renderer.drawTextCentered("START", 25, Color::White, 1);
    } else if (currentState() == GameState::GAME_OVER) {
        renderer.drawTextCentered("GAME OVER", 20, Color::White, 1);
        renderer.drawTextCentered("RESTART", 45, Color::White, 1);
    }
}
}