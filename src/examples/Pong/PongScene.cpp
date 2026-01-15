#include "PongScene.h"
#include "Engine.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 50
#define BALL_RADIUS 6
#define BALL_SPEED 120.0f
#define SCORE_TO_WIN 1

void PongScene::init() {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    leftScore = 0;
    rightScore = 0;
    gameOver = false;

    lblLeftScore = new pr32::graphics::ui::UILabel("0", 20, 8, COLOR_WHITE, 2);
    lblLeftScore->setVisible(true);

    lblRightScore = new pr32::graphics::ui::UILabel("0", screenWidth - 20, 8, COLOR_WHITE, 2);
    lblRightScore->setVisible(true);

    lblStartMessage = new pr32::graphics::ui::UILabel("PRESS A TO START", 0, 150, COLOR_WHITE, 1);
    lblStartMessage->centerX(screenWidth);
    lblStartMessage->setVisible(false);

    lblGameOver = new pr32::graphics::ui::UILabel("GAME OVER", 0, 120, COLOR_WHITE, 2);
    lblGameOver->centerX(screenWidth);
    lblGameOver->setVisible(false);

    leftPaddle = new PaddleActor(0, screenHeight/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, false);
    rightPaddle = new PaddleActor(screenWidth - PADDLE_WIDTH, screenHeight/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, true);
    ball = new BallActor(screenWidth/2, screenHeight/2, BALL_SPEED, BALL_RADIUS);
    ball->reset();

    addEntity(lblLeftScore);
    addEntity(lblRightScore);
    addEntity(lblStartMessage);
    addEntity(lblGameOver);

    addEntity(leftPaddle);
    addEntity(rightPaddle);
    addEntity(ball);
}

void PongScene::update(unsigned long deltaTime) {
    if (!gameOver) {
        // --- Input Player ---
        leftPaddle->velocity = 0;
        if (engine.getInputManager().isButtonDown(0)) leftPaddle->velocity = -100.0f;
        if (engine.getInputManager().isButtonDown(1)) leftPaddle->velocity = 100.0f;

        // --- Update all entities via base Scene ---
        Scene::update(deltaTime);

        // --- Check if ball is out of bounds ---
        if (ball->x < 0) {
            rightScore++;

            char rightScoreStr[16];
            snprintf(rightScoreStr, sizeof(rightScoreStr), "%d", rightScore);
            lblRightScore->setText(rightScoreStr);

            ball->reset();
        }
        if (ball->x > engine.getRenderer().getWidth()) {
            leftScore++;

            char leftScoreStr[16];
            snprintf(leftScoreStr, sizeof(leftScoreStr), "%d", leftScore);
            lblLeftScore->setText(leftScoreStr);

            ball->reset();
        }

        // --- Game Over ---
        if (leftScore >= SCORE_TO_WIN || rightScore >= SCORE_TO_WIN) {
            gameOver = true;
        }

    } else {
        // --- Game Over ---
        lblStartMessage->setVisible(true);
        lblGameOver->setVisible(true);
        if (engine.getInputManager().isButtonPressed(0)) resetGame();
    }
}

void PongScene::draw(pr32::graphics::Renderer& renderer) {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    // === CENTER LINE ===
    int16_t centerX = screenWidth / 2;
    int16_t dashHeight = 10;
    int16_t dashGap = 5;

    for (int16_t y = 0; y < screenHeight; y += (dashHeight + dashGap)) {
        int16_t dashEnd = y + dashHeight;
        if (dashEnd > screenHeight) dashEnd = screenHeight;
        renderer.drawLine(centerX, y, centerX, dashEnd, COLOR_WHITE);
    }

    Scene::draw(renderer);
}

void PongScene::resetGame() {
    leftScore = 0;
    rightScore = 0;
    gameOver = false;
    lblStartMessage->setVisible(false);
    lblGameOver->setVisible(false);
    ball->reset();
}
