#include "PaddleActor.h"
#include <core/Engine.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace brickbreaker {

void PaddleActor::update(unsigned long dt) {
    float deltaTimeSec = dt / 1000.0f;

    if (engine.getInputManager().isButtonDown(2)) {
        x -= speed * deltaTimeSec;
    }

    if (engine.getInputManager().isButtonDown(1)) {
        x += speed * deltaTimeSec;
    }

    if (x < 0) {
        x = 0;
    }
    
    if (x + width > screenWidth) {
        x = screenWidth - width;
    }
}

void PaddleActor::draw(pixelroot32::graphics::Renderer& renderer) {
    renderer.drawFilledRectangle((int)x, (int)y, width, height, color);
}

void PaddleActor::onCollision(pixelroot32::core::Actor* other) {
    (void)other;
}

}
