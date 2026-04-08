#include "BrickActor.h"
#include "platforms/EngineConfig.h"

namespace pr32 = pixelroot32;

namespace brickbreaker {

namespace core = pr32::core;
namespace gfx = pr32::graphics;
namespace math = pr32::math;


BrickActor::BrickActor(math::Vector2 position, int hp)
    : StaticActor(position, 30, 12), hp(hp), active(true) {
    setShape(core::CollisionShape::AABB);
    setBounce(true);
    setCollisionLayer(Layers::BRICK);
    setCollisionMask(Layers::BALL);
}

gfx::Color BrickActor::getColor() {
    switch (this->hp) {
        case 4: return gfx::Color::DarkGray;
        case 3: return gfx::Color::Red;
        case 2: return gfx::Color::Orange;
        case 1: return gfx::Color::Yellow;
        default: return gfx::Color::White;
    }
}

void BrickActor::hit() {
    if (hp > 0) hp--;
    if (hp <= 0) {
        active = false;
        setCollisionLayer(0);
        setCollisionMask(0);
    }
}

void BrickActor::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void BrickActor::draw(gfx::Renderer& renderer) {
    if (!active || hp <= 0) return;

    gfx::Color mainColor = getColor();
    renderer.drawFilledRectangle(static_cast<int>(position.x), static_cast<int>(position.y), 
                                 width, height, mainColor);
    renderer.drawRectangle((int)position.x, (int)position.y, width, height, gfx::Color::Black); 
}

void BrickActor::onCollision(core::Actor* other) {
    (void)other;
}

}
