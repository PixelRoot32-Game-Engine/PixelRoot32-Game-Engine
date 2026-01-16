// BrickActor.cpp
#include "BrickActor.h"
#include "Config.h"

namespace pr32 = pixelroot32;

using Color = pr32::graphics::Color;

namespace brickbreaker {

BrickActor::BrickActor(int x, int y, int hp) 
    : pr32::core::Actor(x, y, 30, 12), hp(hp), active(true) {
    this->setCollisionLayer(Layers::BRICK);
    this->setCollisionMask(Layers::BALL);
}
pr32::graphics::Color BrickActor::getColor() {
    switch (this->hp) {
        case 4: return Color::DarkGray;
        case 3: return Color::Red;
        case 2: return Color::Orange;
        case 1: return Color::Yellow;
        default: return Color::White;
    }
}

void BrickActor::hit() {
    if (hp > 0) hp--;
    if (hp <= 0) {
        active = false;
        this->setCollisionLayer(0);
        this->setCollisionMask(0);
    }
}

void BrickActor::update(unsigned long deltaTime) {
    (void)deltaTime;
    // Prevent unused parameter warning
}

void BrickActor::draw(pr32::graphics::Renderer& renderer) {
    if (!active || hp <= 0) return;

    Color mainColor = getColor();
    
    // 1. Draw brick body
    renderer.drawFilledRectangle(static_cast<int>(x), static_cast<int>(y), width, height, mainColor);
    
    // 2. VISUAL EFFECT: Add a small dark border for relief
    // This makes bricks look much better on screen
    renderer.drawRectangle((int)x, (int)y, width, height, Color::Black); 
}

void BrickActor::onCollision(pixelroot32::core::Actor* other) {
    (void)other;
}

}
