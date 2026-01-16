#pragma once
#include "core/Actor.h"
#include "core/PhysicsActor.h"
#include "GameLayers.h"
#include <graphics/Color.h>

namespace brickbreaker {

class BallActor : public pixelroot32::core::PhysicsActor {
public:
    int radius;
    bool isLaunched;

    BallActor(float x, float y, float initialSpeed, int radius);

    void attachTo(pixelroot32::core::Actor* paddle);
    void launch(float velocityX, float velocityY);
    void reset(pixelroot32::core::Actor* paddle);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    pixelroot32::core::Rect getHitBox() override { return { x - radius, y - radius, radius * 2, radius * 2 }; }

    void onCollision(pixelroot32::core::Actor* other) override;

private:
    float initialSpeed;  // speed at respawn
    pixelroot32::core::Actor* paddleReference = nullptr;
};

}
