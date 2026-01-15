#pragma once
#include "PhysicsActor.h"
#include "GameLayers.h"
#include <graphics/Color.h>

class BallActor : public pixelroot32::core::PhysicsActor {
public:
    int radius; 
    bool isActive;
    unsigned long respawnTimer;   // respawn delay timer

    BallActor(float x, float y, float initialSpeed, int radius);

    void reset();
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    pixelroot32::core::Rect getHitBox() override;

    void onCollision(pixelroot32::core::Actor* other) override;

private:
    float initialSpeed;  // speed at respawn
};
