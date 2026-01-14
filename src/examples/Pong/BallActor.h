#pragma once
#include "Actor.h"
#include "GameLayers.h"

class BallActor : public Actor {
public:
    float vx, vy;
    float speed;
    int radius;
    bool isActive;
    unsigned long respawnTimer;   // respawn delay timer

    inline BallActor(float x, float y, float speed, int radius)
        : Actor(x, y, radius * 2.0f, radius * 2.0f),
            vx(0), 
            vy(0), 
            speed(speed), 
            radius(radius), 
            isActive(false),
            respawnTimer(0) {

        this->setCollisionLayer(Layers::BALL);
        this->setCollisionMask(Layers::PADDLE);
    }
    
    void reset();

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    Rect getHitBox() override { return {x-radius, y-radius, radius*2, radius*2}; }
    void onCollision(Actor* other);
};
