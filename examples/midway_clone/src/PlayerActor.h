#pragma once

#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Entity.h"

#include "GameConstants.h"

namespace midway_clone {

/**
 * @class PlayerActor
 * @brief The player's aircraft: eight-way movement inside the playfield, a
 *        held-fire gun, and a respawn blink.
 *
 * **Position is kept in SCREEN space, not world space.** That is the one
 * decision in this class worth reading twice. The world scrolls underneath a
 * shmup's player; the player does not travel through it. Storing a world
 * position would mean adding the scroll delta back every single frame just to
 * stand still, and any frame that missed the addition would drag the aircraft
 * off the bottom of the screen. Screen space makes standing still the default
 * and costs one addition at draw time.
 *
 * Positions are whole pixels with a carried remainder (see advancePixels),
 * not Scalars. Movement here is pure translation with no physics to integrate,
 * and integers are exact on the non-FPU targets the engine also builds for.
 *
 * Derives from Entity rather than Actor because this example builds with
 * PIXELROOT32_ENABLE_PHYSICS=0: there is no body to integrate and no collision
 * system to register with. Collision is one AABB test in MidwayScene.
 */
class PlayerActor : public pixelroot32::core::Entity {
public:
    PlayerActor();

    /**
     * @brief Reads the stick, moves, and advances the propeller and bank.
     *
     * Eight-way, unlike legend_of_clone's single-axis walk: an aircraft has no
     * reason to refuse a diagonal, and a shmup that snaps to one axis feels
     * stuck. Diagonal travel is NOT normalised — the original moves the same
     * pixels per axis whether one direction is held or two, and correcting it
     * to a true 1/sqrt(2) makes the aircraft feel sluggish on the diagonal
     * compared to the machine being imitated.
     */
    void update(unsigned long deltaTime) override;

    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Tells the player where the playfield currently sits in the world.
     *
     * Set by the scene every frame before update(). Only draw() and hitbox()
     * consume it; movement and clamping are entirely in screen space.
     */
    void setViewportTop(int worldY) { viewportTop_ = worldY; }

    /**
     * @brief True once per fire interval while the fire button is held.
     *
     * Consuming: each true is returned exactly once. The scene calls this after
     * update() and spawns a bullet when it fires.
     */
    bool consumeShot();

    /// Muzzle position in WORLD pixels, for the bullet the scene spawns.
    void muzzleWorldPosition(int& outX, int& outY) const;

    /**
     * @brief Hitbox in WORLD pixels.
     *
     * Inset from the sprite on every side by kPlayerHitboxInset — see that
     * constant for why a shmup lies about its own hitbox.
     */
    Box hitbox() const;

    /// Puts the aircraft back at its spawn and starts the invulnerable window.
    void respawn();

    /// True while the respawn window is still running.
    bool isInvulnerable() const { return invulnMs_ > 0; }

    int screenX() const { return screenX_; }
    int screenY() const { return screenY_; }

private:
    int screenX_;
    int screenY_;
    int viewportTop_ = 0;

    /// Carried sub-pixel travel, in px*ms. One per axis: see advancePixels.
    int travelX_ = 0;
    int travelY_ = 0;

    /// Propeller frame, 0 or 1, and the time accumulated toward the next flip.
    uint8_t       propFrame_ = 0;
    unsigned long propTimer_ = 0;

    /**
     * Bank state: -1 left, 0 level, +1 right, plus the time left holding it.
     *
     * The hold is why this is not simply "is left pressed". See kBankHoldMs.
     */
    int           bank_ = 0;
    unsigned long bankHoldMs_ = 0;

    unsigned long fireTimer_ = 0;
    bool          shotPending_ = false;

    unsigned long invulnMs_ = 0;

    /// Mirrors the screen position into Entity::position, in world space.
    void syncEntityPosition();
};

} // namespace midway_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
