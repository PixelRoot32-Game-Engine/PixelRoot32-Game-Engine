#include "AlienActor.h"
#include "GameConstants.h"
#include "assets/AlienSprites.h"

namespace spaceinvaders {

namespace core = pixelroot32::core;
namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;

static const gfx::SpriteAnimationFrame SQUID_ANIM_FRAMES[] = {
    { &SQUID_F1, nullptr },
    { &SQUID_F2, nullptr }
};

static const gfx::SpriteAnimationFrame CRAB_ANIM_FRAMES[] = {
    { nullptr, &CRAB_F1_MULTI }
};

static const gfx::SpriteAnimationFrame OCTOPUS_ANIM_FRAMES[] = {
    { &OCTOPUS_F1, nullptr },
    { &OCTOPUS_F2, nullptr }
};

AlienActor::AlienActor(math::Vector2 position, AlienType type)
    : core::Actor(position, 0, 0), type(type), active(true) {

    switch (type) {
        case AlienType::SQUID:
            width = ALIEN_SQUID_W;
            height = ALIEN_SQUID_H;
            break;
        case AlienType::CRAB:
            width = ALIEN_CRAB_W;
            height = ALIEN_CRAB_H;
            break;
        case AlienType::OCTOPUS:
            width = ALIEN_OCTOPUS_W;
            height = ALIEN_OCTOPUS_H;
            break;
    }

    if (type == AlienType::SQUID) {
        animation.frames     = SQUID_ANIM_FRAMES;
        animation.frameCount = static_cast<uint8_t>(sizeof(SQUID_ANIM_FRAMES) / sizeof(gfx::SpriteAnimationFrame));
    } else if (type == AlienType::CRAB) {
        animation.frames     = CRAB_ANIM_FRAMES;
        animation.frameCount = static_cast<uint8_t>(sizeof(CRAB_ANIM_FRAMES) / sizeof(gfx::SpriteAnimationFrame));
    } else {
        animation.frames     = OCTOPUS_ANIM_FRAMES;
        animation.frameCount = static_cast<uint8_t>(sizeof(OCTOPUS_ANIM_FRAMES) / sizeof(gfx::SpriteAnimationFrame));
    }
    animation.reset();
}

void AlienActor::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void AlienActor::move(math::Scalar dx, math::Scalar dy) {
    position.x += dx;
    position.y += dy;
    animation.step();
}

void AlienActor::draw(gfx::Renderer& renderer) {
    if (!active) return;

    const gfx::Sprite*      sprite      = animation.getCurrentSprite();
    const gfx::MultiSprite* multiSprite = animation.getCurrentMultiSprite();

    const int drawX = static_cast<int>(position.x);
    const int drawY = static_cast<int>(position.y);

    if (multiSprite) {
        renderer.drawMultiSprite(*multiSprite, drawX, drawY, SPRITE_SCALE, SPRITE_SCALE);
    } else if (sprite) {
        renderer.drawSprite(*sprite, drawX, drawY, SPRITE_SCALE, SPRITE_SCALE, gfx::Color::Orange);
    }
}

core::Rect AlienActor::getHitBox() {
    return {position, width, height};
}

void AlienActor::onCollision(core::Actor* other) {
    (void)other;
}

int AlienActor::getScoreValue() const {
    switch (type) {
        case AlienType::SQUID: return 30;
        case AlienType::CRAB: return 20;
        case AlienType::OCTOPUS: return 10;
        default: return 0;
    }
}

}
