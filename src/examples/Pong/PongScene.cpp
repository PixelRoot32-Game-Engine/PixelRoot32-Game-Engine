#include "PongScene.h"
#include "Engine.h"
#include "PhysicsActor.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;
namespace pong {



using Color = pr32::graphics::Color;

void PongScene::init() {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    leftScore = 0;
    rightScore = 0;
    gameOver = false;

    int16_t scoreY = PONG_PLAY_AREA_TOP / 2 - 8; // Center vertically in top border

    lblScore = new pr32::graphics::ui::UILabel("0 : 0", 0, scoreY, Color::Black, 2);
    lblScore->centerX(screenWidth);
    lblScore->setVisible(true);

    lblStartMessage = new pr32::graphics::ui::UILabel("PRESS A TO START", 0, 150, Color::White, 1);
    lblStartMessage->centerX(screenWidth);
    lblStartMessage->setVisible(false);

    lblGameOver = new pr32::graphics::ui::UILabel("GAME OVER", 0, 120, Color::White, 2);
    lblGameOver->centerX(screenWidth);
    lblGameOver->setVisible(false);

    leftPaddle = new PaddleActor(0, screenHeight/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, false);
    leftPaddle->setTopLimit(PONG_PLAY_AREA_TOP);
    leftPaddle->setBottomLimit(PONG_PLAY_AREA_BOTTOM);
    
    rightPaddle = new PaddleActor(screenWidth - PADDLE_WIDTH, screenHeight/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, true);
    rightPaddle->setTopLimit(PONG_PLAY_AREA_TOP);
    rightPaddle->setBottomLimit(PONG_PLAY_AREA_BOTTOM);  
    
    ball = new BallActor(screenWidth/2, screenHeight/2, BALL_SPEED, BALL_RADIUS);
    ball->setWorldSize(screenWidth, screenHeight);
    ball->setLimits(pr32::core::LimitRect(-1, PONG_PLAY_AREA_TOP, -1, PONG_PLAY_AREA_BOTTOM));
    ball->reset();

    addEntity(lblScore);
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
        if (ball->isActive && ball->getWorldCollisionInfo().left) {
            rightScore++;
            printf("Right Score: %d\n", rightScore);
            ball->reset();
        }
        if (ball->isActive && ball->getWorldCollisionInfo().right) {
            leftScore++;
            printf("Left Score: %d\n", leftScore);
            ball->reset();
        }

        // --- Update score label ---
        char scoreStr[16];
        snprintf(scoreStr, sizeof(scoreStr), "%d : %d", leftScore, rightScore);
        lblScore->setText(scoreStr);

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

    // === BORDERS ===
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, PONG_PLAY_AREA_TOP, Color::White);
    renderer.drawFilledRectangle(0, PONG_PLAY_AREA_BOTTOM, DISPLAY_WIDTH, 
                    DISPLAY_HEIGHT - PONG_PLAY_AREA_BOTTOM, Color::White);

    // === CENTER LINE ===
    int16_t centerX = screenWidth / 2;
    int16_t dashHeight = 10;
    int16_t dashGap = 5;

    for (int16_t y = 0; y < screenHeight; y += (dashHeight + dashGap)) {
        int16_t dashEnd = y + dashHeight;
        if (dashEnd > screenHeight) dashEnd = screenHeight;
        renderer.drawLine(centerX, y, centerX, dashEnd, Color::White);
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

}