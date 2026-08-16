#pragma once

#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Entity.h"
#include "GameConstants.h"
#include "TileWorld.h"

namespace legend_of_clone {

/**
 * @enum Facing
 * @brief The four directions the player sprite can face.
 */
enum class Facing : uint8_t { Down = 0, Up, Left, Right };

/**
 * @class PlayerActor
 * @brief The only character so far. Walks a TileWorld and collides with its
 *        solid tiles, whichever world that happens to be.
 *
 * **Positions are whole pixels, not Scalars.** The NES advances Link by an
 * integer number of pixels every frame and never lands between them, and a
 * tile-grid collision test is exact in integers and merely approximate in
 * Q16.16. `Entity::position` is still kept in sync every step because the
 * scene, the camera and Scene::draw's viewport culling all read it.
 *
 * Derives from Entity rather than Actor: this example builds with
 * PIXELROOT32_ENABLE_PHYSICS=0, so there is no body to integrate and no
 * collision system to register with. Movement is a tile query against the
 * TileWorld it was handed, which is all a top-down game of this kind needs.
 *
 * The scene disables the actor (`setEnabled(false)`) for the duration of a
 * screen transition and writes the position itself — that is how input stays
 * locked out while the camera slides, exactly as on the NES.
 */
class PlayerActor : public pixelroot32::core::Entity {
public:
    /**
     * @brief Places the player on a world tile.
     * @param startCol World tile column.
     * @param startRow World tile row.
     */
    PlayerActor(int startCol, int startRow);

    /**
     * @brief Reads the D-pad and walks one frame's worth of pixels.
     * @param deltaTime Milliseconds since the last frame.
     *
     * Movement is single-axis: the NES has no diagonals, and pressing two
     * directions at once resolves to the horizontal one.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the sprite for the current facing and walk frame.
     *
     * Reproduces the original's sprite economy exactly: west is east mirrored,
     * and the north walk cycle is one bitmap whose mirror *is* the second
     * frame. See assets/PlayerSprites.h for which pairs are mirrors and which
     * are not — it was measured against the rip, not assumed.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /// Teleports the player, bypassing collision. Used to place them across a seam.
    void setPixelPosition(int x, int y);

    /**
     * @brief Points the player at the map it collides against.
     *
     * Set by the scene at init(). Until it is set, every position is treated as
     * blocked: better a player who cannot move than one who walks through a
     * dungeon wall because a scene forgot to wire this up.
     */
    void setWorld(const TileWorld* world) { world_ = world; }

    /// Faces the player without moving them. Used when entering a scene.
    void setFacing(Facing facing) { facing_ = facing; }

    int pixelX() const { return pixelX_; }
    int pixelY() const { return pixelY_; }

    Facing facing() const { return facing_; }

private:
    int    pixelX_;
    int    pixelY_;
    /**
     * Fractional travel carried between frames, in px*ms.
     *
     * Speed is in px/s and frames arrive in ms, so a frame is worth a fraction
     * of a pixel. Dropping that fraction each frame would make the player
     * measurably slower than kPlayerSpeedPxPerSec; carrying it in an integer
     * keeps the rate exact without introducing a float on the hot path.
     */
    int    travelAccumulator_ = 0;
    Facing facing_ = Facing::Down;

    /**
     * Walk frame, 0 or 1. Two states, never a longer sequence — the original's
     * counter rollover is `ObjAnimFrame EOR #$01`.
     */
    uint8_t walkFrame_ = 0;

    /// Milliseconds accumulated toward the next walk-frame toggle.
    unsigned long walkTimer_ = 0;

    /**
     * @brief Advances the walk cycle while a direction is held.
     *
     * Keyed on input, not on movement, which is what the original does: the
     * gate is `LDA ObjInputDir / BEQ` before the counter is touched. So Link
     * keeps walking on the spot when he is pressed against a wall, and the
     * frame he stops on is the frame he keeps — nothing snaps him to a neutral
     * pose on release.
     */
    void advanceWalkCycle(unsigned long deltaTime, bool directionHeld);

    /// The map this player collides against. Never owned.
    const TileWorld* world_ = nullptr;

    /// Whether a 16x16 box at (x, y) clears every world tile it overlaps.
    bool canOccupy(int x, int y) const;

    /// Walks up to `steps` pixels along one axis, stopping flush against a wall.
    void stepAxis(int steps, int deltaX, int deltaY);

    /// Mirrors the integer position into Entity::position for the rest of the engine.
    void syncEntityPosition();
};

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
