#include "BunkerActor.h"
#include "GameConstants.h"

namespace spaceinvaders {

namespace core = pixelroot32::core;
namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;

BunkerActor::BunkerActor(math::Vector2 position, int w, int h, int maxHealth)
    : StaticActor(position, w, h), health(maxHealth), maxHealth(maxHealth) {
    setShape(core::CollisionShape::AABB);
}

void BunkerActor::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void BunkerActor::draw(gfx::Renderer& renderer) {
    if (health <= 0) return;

    math::Scalar ratio = (maxHealth > 0) ? math::toScalar(health) / math::toScalar(maxHealth) : math::toScalar(0.0f);
    if (ratio <= math::toScalar(0.0f)) return;

    int visibleHeight = static_cast<int>(height * ratio);
    if (visibleHeight <= 0) return;

    int drawY = static_cast<int>(position.y + math::toScalar(height - visibleHeight));
    gfx::Color c = gfx::Color::Green;
    if (ratio < math::toScalar(0.5f) && ratio >= math::toScalar(0.25f)) {
        c = gfx::Color::Yellow;  // Warning: Damaged
    } else if (ratio < math::toScalar(0.25f)) {
        c = gfx::Color::Red;
    }

    renderer.drawFilledRectangle(static_cast<int>(position.x), drawY, width, visibleHeight, c);
}

void BunkerActor::onCollision(core::Actor* other) {
    (void)other;
}

void BunkerActor::applyDamage(int amount) {
    health -= amount;
    if (health < 0) {
        health = 0;
    }
}

bool BunkerActor::isDestroyed() const {
    return health <= 0;
}

}
