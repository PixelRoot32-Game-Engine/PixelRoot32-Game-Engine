#pragma once
#include "core/Actor.h"
#include "input/InputManager.h"
#include "GameLayers.h"
#include "graphics/Color.h"

namespace brickbreaker {

class PaddleActor: public pixelroot32::core::Actor {
private:
    float speed = 200.0f;
    pixelroot32::graphics::Color color = pixelroot32::graphics::Color::White;
    float screenWidth;

public:
    PaddleActor(float x, float y, int w, int h, float sWidth) 
        : pixelroot32::core::Actor(x, y, w, h), screenWidth(sWidth) {
        
        this->setCollisionLayer(Layers::PADDLE);
        this->setCollisionMask(Layers::BALL);
    }

    void update(unsigned long dt) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    pixelroot32::core::Rect getHitBox() override { return {x, y, width, height}; }
    void onCollision(pixelroot32::core::Actor* other) override;
};

}
