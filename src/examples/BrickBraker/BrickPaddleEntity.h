#pragma once
#include "core/Actor.h"

class BrickPaddleEntity : public Actor {
public:
    float velocity;
    bool isAI;
    uint32_t color;

    BrickPaddleEntity(int x, int y, int w, int h, bool isAI);

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    
    Rect getHitBox() override { return {x, y, width, height}; }

    void onCollision(Entity* other) override;
};
