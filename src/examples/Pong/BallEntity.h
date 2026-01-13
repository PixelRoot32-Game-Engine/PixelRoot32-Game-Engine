#pragma once
#include "core/Actor.h"

class BallEntity : public Actor  {
public:
    float vx, vy;
    float speed;
    float radius;
    unsigned long respawnTimer;   
    bool active;                  

    BallEntity(float x, float y, int radius, float speed)
        : Actor(x, y, (int)radius, (int)radius), vx(0), vy(0),
            radius(radius),
            speed(speed),
            respawnTimer(0),
            active(false) {}

    void reset();

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    Rect getHitBox() override { return {x-radius, y-radius, radius*2, radius*2}; }

    void onCollision(Entity* other);
};
