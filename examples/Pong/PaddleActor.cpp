#include "PaddleActor.h"
#include "PongScene.h"  
#include "core/Engine.h"
#include <math.h>
#include "GameLayers.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace pong {

using Color = pr32::graphics::Color;

// Constantes AI
#define AI_TARGET_OFFSET 6.0f
#define AI_MAX_SPEED 90.0f
#define AI_MOVEMENT_THRESHOLD 0.5f

void PaddleActor::update(unsigned long deltaTime) {
    float dt = deltaTime / 1000.0f;
    
    if (isAI) {
        // AI movement
        PongScene* pongScene = static_cast<PongScene*>(engine.getCurrentScene());

        if (pongScene) {
            float ballX = pongScene->getBallX();
            float ballY = pongScene->getBallY();

            float paddleCenterY = y + height / 2.0f;
            float diff = ballY - paddleCenterY;

            // Small offset for AI
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
        // Player movement
        y += velocity * dt;
    }

    // Keep inside play area
    if (y < topLimit) { y = topLimit; accumulator = 0; }
    if (y + height > bottomLimit) { y = bottomLimit - height; accumulator = 0; }
}

void PaddleActor::draw(pr32::graphics::Renderer& renderer) {
    renderer.drawFilledRectangle((int)x, (int)y, width, height, Color::White);
}

void PaddleActor::onCollision(pr32::core::Actor* other) {
    (void)other;
    // No special collision handling needed for paddles
}

}
