#include "PlayerActor.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Engine.h"
#include "assets/PlayerSprites.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace midway_clone {

namespace gfx = pr32::graphics;
namespace math = pr32::math;

PlayerActor::PlayerActor()
    : Entity(math::Vector2(kPlayerStartX, kPlayerStartY),
             kPlayerSize, kPlayerSize,
             pr32::core::EntityType::GENERIC)
    , screenX_(kPlayerStartX)
    , screenY_(kPlayerStartY) {
}

void PlayerActor::syncEntityPosition() {
    // Entity::position is world space: the scene's viewport culling and the
    // engine's render sort both read it, and both run after camera_.apply().
    position.x = math::toScalar(screenX_);
    position.y = math::toScalar(viewportTop_ + screenY_);
}

void PlayerActor::respawn() {
    screenX_ = kPlayerStartX;
    screenY_ = kPlayerStartY;
    travelX_ = 0;
    travelY_ = 0;
    bank_ = 0;
    bankHoldMs_ = 0;
    invulnMs_ = kPlayerInvulnMs;
    shotPending_ = false;
    // Deliberately not reset: fireTimer_. Respawning should not hand the player
    // a free instant shot, which resetting it to the interval would.
    syncEntityPosition();
}

void PlayerActor::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();

    const bool up    = input.isButtonDown(BTN_UP);
    const bool down  = input.isButtonDown(BTN_DOWN);
    const bool left  = input.isButtonDown(BTN_LEFT);
    const bool right = input.isButtonDown(BTN_RIGHT);

    const int dirX = (right ? 1 : 0) - (left ? 1 : 0);
    const int dirY = (down ? 1 : 0) - (up ? 1 : 0);

    // --- Move -------------------------------------------------------------
    // Each axis carries its own remainder, so releasing one direction never
    // discards travel banked on the other.
    if (dirX != 0) {
        screenX_ += advancePixels(travelX_, dirX * kPlayerSpeedPxPerSec, deltaTime);
    } else {
        travelX_ = 0;
    }
    if (dirY != 0) {
        screenY_ += advancePixels(travelY_, dirY * kPlayerSpeedPxPerSec, deltaTime);
    } else {
        travelY_ = 0;
    }

    // Clamp to the playfield, not to the display: the HUD strip below is not
    // somewhere an aircraft may fly.
    const int maxX = kPlayfieldWidth  - kPlayerSize;
    const int maxY = kPlayfieldHeight - kPlayerSize;
    if (screenX_ < 0)    screenX_ = 0;
    if (screenX_ > maxX) screenX_ = maxX;
    if (screenY_ < 0)    screenY_ = 0;
    if (screenY_ > maxY) screenY_ = maxY;

    // --- Bank -------------------------------------------------------------
    if (dirX != 0) {
        bank_ = dirX;
        bankHoldMs_ = kBankHoldMs;
    } else if (bankHoldMs_ > deltaTime) {
        bankHoldMs_ -= deltaTime;
    } else {
        bankHoldMs_ = 0;
        bank_ = 0;
    }

    // --- Propeller --------------------------------------------------------
    propTimer_ += deltaTime;
    while (propTimer_ >= kPropFrameMs) {
        propTimer_ -= kPropFrameMs;
        propFrame_ ^= 1;
    }

    // --- Gun --------------------------------------------------------------
    // The timer runs whether or not the button is held, so tapping fire at the
    // right moment cannot beat the rate limit.
    fireTimer_ += deltaTime;
    if (fireTimer_ >= kPlayerFireIntervalMs) {
        fireTimer_ -= kPlayerFireIntervalMs;
        if (input.isButtonDown(BTN_A)) {
            shotPending_ = true;
        }
    }

    // --- Respawn window ---------------------------------------------------
    if (invulnMs_ > 0) {
        invulnMs_ = (invulnMs_ > deltaTime) ? invulnMs_ - deltaTime : 0;
    }

    syncEntityPosition();
}

bool PlayerActor::consumeShot() {
    const bool pending = shotPending_;
    shotPending_ = false;
    return pending;
}

void PlayerActor::muzzleWorldPosition(int& outX, int& outY) const {
    // Centred on the sprite horizontally, at its nose. The bullet is 8 px wide
    // and the aircraft 16, so half the difference centres it.
    outX = screenX_ + (kPlayerSize - kBulletSize) / 2;
    outY = viewportTop_ + screenY_ - kBulletSize / 2;
}

Box PlayerActor::hitbox() const {
    return Box{
        screenX_ + kPlayerHitboxInset,
        viewportTop_ + screenY_ + kPlayerHitboxInset,
        kPlayerSize - 2 * kPlayerHitboxInset,
        kPlayerSize - 2 * kPlayerHitboxInset,
    };
}

void PlayerActor::draw(gfx::Renderer& renderer) {
    // Blink while invulnerable: hidden for half of each period. Dividing the
    // remaining time rather than a free-running clock means the blink always
    // ends on a visible frame.
    if (invulnMs_ > 0 && ((invulnMs_ / kPlayerBlinkMs) & 1u) != 0u) {
        return;
    }

    uint8_t frame;
    if (bank_ < 0) {
        frame = PLAYER_BANK_LEFT;
    } else if (bank_ > 0) {
        frame = PLAYER_BANK_RIGHT;
    } else {
        frame = propFrame_ ? PLAYER_LEVEL_B : PLAYER_LEVEL_A;
    }
    // The banked frames carry only one propeller position, so the blur stops
    // while rolling. At kBankHoldMs the aircraft is never level-locked long
    // enough for that to read as the propeller stopping.

    renderer.drawSprite(PLAYER_FRAMES[frame], screenX_, viewportTop_ + screenY_);
}

} // namespace midway_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
