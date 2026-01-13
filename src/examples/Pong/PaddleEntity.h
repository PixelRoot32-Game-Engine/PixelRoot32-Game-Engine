#pragma once
#include "core/Actor.h"

class PaddleEntity : public Actor  {
public:
    // float x, y;
    // int width, height;
    float velocity; 
    float accumulator;
    bool isAI;

    PaddleEntity(float x, float y, int w, int h, bool ai = false)
        : Actor(x, y, w, h),
            velocity(0), 
            accumulator(0), 
            isAI(ai) {}

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    Rect getHitBox() override { return {x, y, width, height}; }

    void onCollision(Entity* other) override;
};
