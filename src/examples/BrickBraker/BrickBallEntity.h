#pragma once
#include "core/Actor.h"

class BrickBallEntity : public Actor {
public:
    float vx, vy;
    float radius;
    int speed;
    uint32_t color;

    BrickBallEntity(int x, int y, float radius, int speed);

    void reset();

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
    
    Rect getHitBox() override { return {x-radius, y-radius, radius*2, radius*2}; }

    void onCollision(Entity* other);
    
};
