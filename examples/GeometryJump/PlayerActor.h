#pragma once
#include "core/PhysicsActor.h"
#include "GameLayers.h"
#include "graphics/Color.h"

namespace geometryjump {

class PlayerActor : public pixelroot32::core::PhysicsActor {
public:
    PlayerActor(float x, float y, int w, int h, float speedX);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    inline pixelroot32::core::Rect getHitBox() override {
        return { x, y, width, height };
    }

    bool isGrounded() const;
    void jump();
    void reset(float startX, float startY);

protected:
    void onWorldCollision() override;

private:
    float moveSpeedX;
    bool grounded = false;
};

}
