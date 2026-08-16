#pragma once
#include "core/Entity.h"
#include "GameConstants.h"

namespace room_screen {

/**
 * @enum Facing
 * @brief The four directions the player sprite can face.
 */
enum class Facing : uint8_t { Down = 0, Up, Left, Right };

/**
 * @brief The player: a 16x16 hero with idle + walk sprites.
 *
 * Position is stored as whole pixels (the same trick the legend_of_clone
 * example uses) so the tile-grid collision test is exact and the hot path
 * stays float-free on FPU-less targets. A travel accumulator carries the
 * sub-pixel remainder between frames, keeping the speed exact without a float.
 *
 * The player collides against the exported Items behavior layer (TILE_SOLID).
 * The collision strategy is selectable at compile time via kCollisionMode in
 * GameConstants.h: whole-tile, per-pixel, or per-pixel with morphological
 * erosion (the default). Room transitions are the scene's job: the scene
 * detects a boundary crossing and rewrites the position via setPixelPosition().
 */
class Player : public pixelroot32::core::Entity {
public:
    Player(int startX, int startY);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    int pixelX() const { return pixelX_; }
    int pixelY() const { return pixelY_; }

    /// Teleports the player (used by the scene on a room transition).
    void setPixelPosition(int x, int y);

private:
    int pixelX_;
    int pixelY_;

    /// Fractional travel carried between frames, in px*ms.
    int travelAccumulator_ = 0;

    Facing facing_ = Facing::Down;
    uint8_t walkFrame_ = 0;
    unsigned long walkTimer_ = 0;
    bool moving_ = false;

    /// Whether a kPlayerSize box at (x, y) clears every overlapped tile.
    bool canOccupy(int x, int y) const;

    /// Walks up to `steps` pixels along one axis, stopping flush against a wall.
    void stepAxis(int steps, int deltaX, int deltaY);

    /// Mirrors the integer position into Entity::position for the engine.
    void syncEntityPosition();
};

} // namespace room_screen
