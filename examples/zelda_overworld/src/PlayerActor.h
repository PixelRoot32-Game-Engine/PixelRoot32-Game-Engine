#pragma once

#include "core/Entity.h"
#include "GameConstants.h"

namespace zelda_overworld {

/**
 * @enum Facing
 * @brief The four directions the player sprite can face.
 */
enum class Facing : uint8_t { Down = 0, Up, Left, Right };

/**
 * @class PlayerActor
 * @brief The only character in iteration 1. Walks the overworld and collides
 *        with the world's solid tiles.
 *
 * **Positions are whole pixels, not Scalars.** The NES advances Link by an
 * integer number of pixels every frame and never lands between them, and a
 * tile-grid collision test is exact in integers and merely approximate in
 * Q16.16. `Entity::position` is still kept in sync every step because the
 * scene, the camera and Scene::draw's viewport culling all read it.
 *
 * Derives from Entity rather than Actor: this example builds with
 * PIXELROOT32_ENABLE_PHYSICS=0, so there is no body to integrate and no
 * collision system to register with. Movement is a tile query against
 * isSolidCell(), which is all a top-down overworld needs.
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
     * @brief Draws the sprite for the current facing.
     *
     * West reuses the east bitmap with flipX, the same trick the original
     * uses to keep the pattern table small.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /// Teleports the player, bypassing collision. Used to place them across a seam.
    void setPixelPosition(int x, int y);

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

    /// Whether a 16x16 box at (x, y) clears every world tile it overlaps.
    static bool canOccupy(int x, int y);

    /// Walks up to `steps` pixels along one axis, stopping flush against a wall.
    void stepAxis(int steps, int deltaX, int deltaY);

    /// Mirrors the integer position into Entity::position for the rest of the engine.
    void syncEntityPosition();
};

} // namespace zelda_overworld
