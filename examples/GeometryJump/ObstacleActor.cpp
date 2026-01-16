#include "ObstacleActor.h"

namespace pr32 = pixelroot32;

namespace geometryjump {

using Color = pr32::graphics::Color;

ObstacleActor::ObstacleActor(float x, float y, int w, int h)
    : pr32::core::Actor(x, y, w, h) {
    setCollisionLayer(Layers::OBSTACLE);
    setCollisionMask(Layers::PLAYER);
}

void ObstacleActor::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void ObstacleActor::draw(pr32::graphics::Renderer& renderer) {
    renderer.drawFilledRectangle(static_cast<int>(x), static_cast<int>(y), width, height, Color::White);
}

void ObstacleActor::onCollision(pixelroot32::core::Actor* other) {
    (void)other;
}

}

