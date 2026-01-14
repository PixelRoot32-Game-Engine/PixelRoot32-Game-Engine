#include "BallActor.h"
#include "EDGE.h"
#include <stdlib.h>
#include <math.h>
#include <Config.h>

extern EDGE engine;

void BallActor::reset() {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    x = screenWidth / 2.0f;
    y = screenHeight / 2.0f;

    vx = 0;
    vy = 0;
    respawnTimer = 500;
    isActive = false;
    isEnabled = true;
}

void BallActor::update(unsigned long deltaTime) {
    float dt = deltaTime / 1000.0f;

    if (!isActive) {
        if (respawnTimer > deltaTime) {
            respawnTimer -= deltaTime;
            return;
        } else {
   
            respawnTimer = 0;
            isActive = true;

            vx = (rand() % 2 == 0 ? 1 : -1) * speed;
            vy = ((rand() % 100) / 100.0f - 0.5f) * speed;
        }
    }

    x += vx * dt;
    y += vy * dt;

    int screenHeight = engine.getRenderer().getHeight();

    if (y - radius < 0) { y = radius; vy = -vy; }
    if (y + radius > screenHeight) { y = screenHeight - radius; vy = -vy; }
}

void BallActor::draw(Renderer& renderer) {
    if(isEnabled) renderer.drawCircle((int)x, (int)y, radius, COLOR_WHITE);
}

void BallActor::onCollision(Actor* other) {
    // The ball only interacts with PADDLE; if it interacts with another layer, use isInLayer 
    vx = -vx;

    if (vx > 0) {
        x = other->x + other->width + radius;
    } else {
        x = other->x - radius;
    }

    float impactPos = (y - other->y) / other->height - 0.5f;
    vy += impactPos * 50.0f;
}
