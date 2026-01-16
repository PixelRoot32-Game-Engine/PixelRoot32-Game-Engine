#pragma once
#include "core/Actor.h"
#include "GameLayers.h"


namespace brickbreaker {

class BrickActor : public pixelroot32::core::Actor {
public:
    int hp;      
    bool active;

    BrickActor(int x, int y, int hp);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    void hit(); 
    pixelroot32::graphics::Color getColor();

    // Improve HitBox for precision
    pixelroot32::core::Rect getHitBox() override { return {x, y, width, height}; }
    void onCollision(pixelroot32::core::Actor* other) override;
};

}
