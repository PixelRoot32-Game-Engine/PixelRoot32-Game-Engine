#pragma once
#include "Actor.h"
#include "GameLayers.h"

class PaddleActor : public pixelroot32::core::Actor {
public:
    float velocity;      // for player movement
    float accumulator;   // for AI movement
    bool isAI;

    PaddleActor(float x, float y, int w, int h, bool ai = false)
        : pixelroot32::core::Actor(x, y, w, h), velocity(0), accumulator(0), isAI(ai) {
        
        this->setCollisionLayer(Layers::PADDLE);
        this->setCollisionMask(Layers::BALL);
    }

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    pixelroot32::core::Rect getHitBox() override { return {x, y, width, height}; }

    void onCollision(pixelroot32::core::Actor* other) override;
};
