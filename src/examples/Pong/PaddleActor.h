#pragma once
#include "Actor.h"
#include "GameLayers.h"

class PaddleActor : public Actor {
public:
    float velocity;      // for player movement
    float accumulator;   // for AI movement
    bool isAI;

    PaddleActor(float x, float y, int w, int h, bool ai = false)
        : Actor(x, y, w, h), velocity(0), accumulator(0), isAI(ai) {
        
        this->setCollisionLayer(Layers::PADDLE);
        this->setCollisionMask(Layers::BALL);
    }

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    Rect getHitBox() override { return {x, y, width, height}; }

    void onCollision(Actor* other) override;
};
