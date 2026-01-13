#include "PongScene.h"
#include "Config.h"
#include "core/EDGE.h"

extern EDGE engine;

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 50
#define BALL_RADIUS 4
#define BALL_SPEED 120.0f
#define SCORE_TO_WIN 5
#define BORDER_THICKNESS 30 

void PongScene::init() {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    // Ajustamos la posición inicial para que no atraviesen los nuevos bordes
    leftPaddle = new PaddleEntity(0, screenHeight/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, false);
    rightPaddle = new PaddleEntity(screenWidth - PADDLE_WIDTH, screenHeight/2 - PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT, true);
    ball = new BallEntity(screenWidth/2, screenHeight/2, BALL_RADIUS, BALL_SPEED);
    ball->reset();

    addEntity(leftPaddle);
    addEntity(rightPaddle);
    addEntity(ball);

    leftScore = 0;
    rightScore = 0;
    gameOver = false;
}

void PongScene::update(unsigned long deltaTime) {
    if (!gameOver) {
        // --- Input Player ---
        leftPaddle->velocity = 0;
        if (engine.getInputManager().isButtonDown(0)) leftPaddle->velocity = -100.0f;
        if (engine.getInputManager().isButtonDown(1)) leftPaddle->velocity = 100.0f;

        // --- Update actors ---
        leftPaddle->update(deltaTime);
        rightPaddle->update(deltaTime);
        ball->update(deltaTime);

        int screenHeight = engine.getRenderer().getHeight();

        // --- Restringir Paletas a los bordes ---
        // Evita que las paletas dibujen sobre el borde blanco
        if (leftPaddle->y < BORDER_THICKNESS) leftPaddle->y = BORDER_THICKNESS;
        if (leftPaddle->y + PADDLE_HEIGHT > screenHeight - BORDER_THICKNESS)
            leftPaddle->y = screenHeight - BORDER_THICKNESS - PADDLE_HEIGHT;

        if (rightPaddle->y < BORDER_THICKNESS) rightPaddle->y = BORDER_THICKNESS;
        if (rightPaddle->y + PADDLE_HEIGHT > screenHeight - BORDER_THICKNESS)
            rightPaddle->y = screenHeight - BORDER_THICKNESS - PADDLE_HEIGHT;

        // --- 4. Limitar y Rebotar Bola (Lógica de los bordes blancos) ---
        // Rebote Superior
        if (ball->y - BALL_RADIUS < BORDER_THICKNESS) {
            ball->y = BORDER_THICKNESS + BALL_RADIUS;
            ball->vy *= -1; // Invertimos su velocidad vertical
        }
        // Rebote Inferior
        if (ball->y + BALL_RADIUS > screenHeight - BORDER_THICKNESS) {
            ball->y = screenHeight - BORDER_THICKNESS - BALL_RADIUS;
            ball->vy *= -1; // Invertimos su velocidad vertical
        }

        // --- Collisions between entities ---
        collisionSystem.update();

        // --- Check if ball is out of bounds (Puntos) ---
        if (ball->x < 0) {
            rightScore++;
            ball->reset();
        }
        if (ball->x > engine.getRenderer().getWidth()) {
            leftScore++;
            ball->reset();
        }

        // --- Game Over ---
        if (leftScore >= SCORE_TO_WIN || rightScore >= SCORE_TO_WIN) {
            gameOver = true;
        }

    } else {
        if (engine.getInputManager().isButtonPressed(0)) resetGame();
    }
}

void PongScene::draw(Renderer& renderer) {
    int sw = engine.getRenderer().getWidth();
    int sh = engine.getRenderer().getHeight();

    // 1. Dibujar línea divisoria punteada
    int dashHeight = 10;
    int gapHeight = 10;
    for (int i = BORDER_THICKNESS; i < sh - BORDER_THICKNESS; i += (dashHeight + gapHeight)) {
        // Dibujamos un pequeño rectángulo blanco en el centro x
        renderer.drawFilledRectangle(sw / 2 - 2, i, 2, dashHeight, COLOR_WHITE);
    }

    // 2. Dibujar bordes superior e inferior
    renderer.drawFilledRectangle(0, 0, sw, BORDER_THICKNESS, COLOR_WHITE); // Arriba
    renderer.drawFilledRectangle(0, sh - BORDER_THICKNESS, sw, BORDER_THICKNESS, COLOR_WHITE); // Abajo

    // 3. Dibujar puntajes (ajustados para no chocar con el borde)

    char leftScoreStr[16];
    char rightScoreStr[16];

    snprintf(leftScoreStr, sizeof(rightScoreStr), "%d", leftScore);
    snprintf(rightScoreStr, sizeof(rightScoreStr), "%d", rightScore);

    renderer.drawText(leftScoreStr, sw / 4,BORDER_THICKNESS - 12, COLOR_BLACK, 2);
    renderer.drawText(rightScoreStr, (sw * 3) / 4, BORDER_THICKNESS - 12, COLOR_BLACK, 2);

    if (gameOver) {
        renderer.drawTextCentered("GAME OVER!", sh/2, COLOR_RED, 2);
    }

    // 4. Dibujar entidades (paletas y bola)
    Scene::draw(renderer);
}

void PongScene::resetGame() {
    leftScore = 0;
    rightScore = 0;
    gameOver = false;
    ball->reset();
}