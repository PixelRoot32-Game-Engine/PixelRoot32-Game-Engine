#pragma once
#include "core/Actor.h"
#include "GameLayers.h"
#include "graphics/Color.h"

namespace geometryjump {

class ObstacleActor : public pixelroot32::core::Actor {
public:
    ObstacleActor(float x, float y, int w, int h);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    inline pixelroot32::core::Rect getHitBox() override {
        return { x, y, width, height };
    }

    void onCollision(pixelroot32::core::Actor* other) override;
};

}

