#pragma once
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS

#include "physics/StaticActor.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "gameplay/InteractionComponent.h"

#include "GameLayers.h"
#include "assets/PickupSprites.h"
#include "assets/PlayerPalette.h"

namespace metroidvania {

/**
 * @class PickupActor
 * @brief A collectible orb that reports contact instead of blocking it.
 *
 * This is the smallest useful shape of the interaction-trigger capability:
 * a body that exists only to notice the player. Two details make it work.
 *
 * **It is a sensor.** `setSensor(true)` means the physics step still produces
 * a contact for the pair, but resolves no response — the player walks through
 * the orb instead of standing on it. A solid pickup would be a wall.
 *
 * **Contact is not the same as collection.** The engine reports a contact for
 * every frame the boxes overlap; it is InteractionTracker that diffs frame
 * against frame and calls onEnter exactly once, on the frame the overlap
 * begins. Without that edge detection this callback would fire ten times as
 * the player crossed the orb, and the counter would be nonsense.
 *
 * The hooks are a plain `InteractionComponent` member rather than virtual
 * methods on Actor: an actor that does not opt in pays zero bytes for the
 * capability.
 */
class PickupActor : public pixelroot32::physics::StaticActor {
public:
    static constexpr int kSize = 8;

    PickupActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y)
        : pixelroot32::physics::StaticActor(x, y, kSize, kSize) {
        setShape(pixelroot32::core::CollisionShape::AABB);
        // ENEMY is the layer GameLayers.h reserves for enemies, projectiles
        // and pickups, and the player already carries it in every branch of
        // its mask — including while climbing. Reusing it means this pickup
        // needs no change to PlayerActor's mask juggling.
        setCollisionLayer(Layers::ENEMY);
        setCollisionMask(Layers::PLAYER);
        setSensor(true);

        interaction.owner = this;
        interaction.onEnter = &PickupActor::onPlayerEnter;
    }

    pixelroot32::core::Rect getHitBox() override {
        return {position, width, height};
    }

    void update(unsigned long deltaTime) override {
        if (collected_) {
            return;
        }
        frameElapsed_ += deltaTime;
        if (frameElapsed_ >= PICKUP_FRAME_DURATION_MS) {
            frameElapsed_ = 0;
            frame_ ^= 1;
        }
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        if (collected_) {
            return;
        }
        renderer.drawSprite(kFrames[frame_],
                            static_cast<int>(position.x),
                            static_cast<int>(position.y));
    }

    bool isCollected() const { return collected_; }

    /** Non-owning hook block handed to InteractionTracker::registerActor(). */
    pixelroot32::gameplay::InteractionComponent interaction;

private:
    /**
     * @brief Fired once, on the frame the player's box starts overlapping.
     *
     * Signature is fixed by InteractionComponent::TriggerFn, so this is a
     * static function taking the owner back as `void*`.
     */
    static void onPlayerEnter(void* owner, pixelroot32::core::Actor* /*other*/) {
        auto* self = static_cast<PickupActor*>(owner);
        if (self == nullptr || self->collected_) {
            return;
        }
        self->collected_ = true;
        // Stop producing contacts once taken: an already-collected orb must
        // not keep the pair alive in the tracker's contact set.
        self->setEnabled(false);
        self->isVisible = false;
    }

    /**
     * The palette member is the sprite-index → engine-Color mapping, exactly
     * as the player's frames do it. The actual RGB565 values come from
     * setSpriteCustomPalette(), which MetroidvaniaScene::init() already
     * installed before the first draw.
     */
    static constexpr pixelroot32::graphics::Sprite4bpp kFrames[2] = {
        { PICKUP_ORB_SPRITE_0_4BPP, PLAYER_PALETTE_MAPPING, kSize, kSize, 8 },
        { PICKUP_ORB_SPRITE_1_4BPP, PLAYER_PALETTE_MAPPING, kSize, kSize, 8 },
    };

    unsigned long frameElapsed_ = 0;
    uint8_t frame_ = 0;
    bool collected_ = false;
};

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
