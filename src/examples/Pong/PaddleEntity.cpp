#include "PaddleEntity.h"
#include "PongScene.h"  
#include "core/EDGE.h"
#include <math.h>

extern EDGE engine;

#define AI_TARGET_OFFSET 6.0f
#define AI_MAX_SPEED 90.0f
#define AI_MOVEMENT_THRESHOLD 0.5f
#define PONG_PLAY_AREA_TOP 0
#define PONG_PLAY_AREA_BOTTOM engine.getRenderer().getHeight()

void PaddleEntity::update(unsigned long deltaTime) {
    float dt = deltaTime / 1000.0f;
    
    if (isAI) {
        PongScene* pongScene = static_cast<PongScene*>(engine.getCurrentScene());

        if (pongScene) {
            float ballX = pongScene->getBallX();
            float ballY = pongScene->getBallY();

            float paddleCenterY = y + height / 2.0f;
            float diff = ballY - paddleCenterY;


            static uint32_t aiOffsetSeed = 0;
            if (AI_TARGET_OFFSET > 0.0f) {
                if (aiOffsetSeed == 0) aiOffsetSeed = millis();
                float offset = ((float)((aiOffsetSeed + (uint32_t)(ballX*5) + (uint32_t)(millis()/150)) % 200) / 200.0f - 0.5f) * AI_TARGET_OFFSET;
                diff += offset;
                aiOffsetSeed++;
            }

            float proportionalSpeed = diff * 1.5f;
            if (proportionalSpeed > AI_MAX_SPEED) proportionalSpeed = AI_MAX_SPEED;
            else if (proportionalSpeed < -AI_MAX_SPEED) proportionalSpeed = -AI_MAX_SPEED;

            if (fabsf(diff) > AI_MOVEMENT_THRESHOLD) accumulator += proportionalSpeed * dt;

            if (accumulator >= 1.0f) { int m = (int)accumulator; y += m; accumulator -= m; }
            else if (accumulator <= -1.0f) { int m = (int)accumulator; y += m; accumulator -= m; }

        }

    } else {
        y += velocity * dt;
    }

    if (y < 0) { y = 0; accumulator = 0; }
    if (y + height > PONG_PLAY_AREA_BOTTOM) { y = PONG_PLAY_AREA_BOTTOM - height; accumulator = 0; }
}

void PaddleEntity::draw(Renderer& renderer) {
    renderer.drawFilledRectangle((int)x, (int)y, width, height, 0xFFFF); // blanco
}

void PaddleEntity::onCollision(Entity* other) {
    // No special collision handling needed for paddles
}
