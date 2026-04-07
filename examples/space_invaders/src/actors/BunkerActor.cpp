#include "BunkerActor.h"
#include "GameConstants.h"

namespace spaceinvaders {

BunkerActor::BunkerActor(pixelroot32::math::Vector2 position, int w, int h, int maxHealth)
    : StaticActor(position, w, h), health(maxHealth), maxHealth(maxHealth) {
    setShape(pixelroot32::core::CollisionShape::AABB);
}

void BunkerActor::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void BunkerActor::draw(pixelroot32::graphics::Renderer& renderer) {
    if (health <= 0) return;

    using Color = pixelroot32::graphics::Color;
    float ratio = (maxHealth > 0) ? (float)health / (float)maxHealth : 0.0f;
    if (ratio <= 0.0f) return;

    int visibleHeight = (int)(height * ratio);
    if (visibleHeight <= 0) return;

    int drawY = (int)(position.y + (height - visibleHeight));
    Color c = Color::Green;
    if (ratio < 0.5f && ratio >= 0.25f) {
        c = Color::Yellow;  // Warning: Damaged
    } else if (ratio < 0.25f) {
        c = Color::Red;
    }

    renderer.drawFilledRectangle((int)position.x, drawY, width, visibleHeight, c);
}

void BunkerActor::onCollision(pixelroot32::core::Actor* other) {
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
