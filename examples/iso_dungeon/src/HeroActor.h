#pragma once

#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "gameplay/GridMotion.h"

#include "IsoDungeonConstants.h"

namespace iso_dungeon {

/**
 * @class HeroActor
 * @brief The player: exact tile-to-tile movement across the room's grid.
 *
 * The hero is always either at rest ON a tile or travelling BETWEEN two named
 * tiles -- never anywhere else. `gameplay::GridMotion` owns that state and
 * `gameplay::tickStep` advances it; this class supplies only the two policies
 * the engine deliberately does not own: which cell may be entered, and where
 * the direction comes from.
 *
 * The isometric view enters in exactly ONE place: the call to
 * `gameplay::interpolatedWorld` with `kTileProjection`. Everything above it --
 * the cell the hero occupies, the cell it is walking to, whether the target is
 * solid -- is projection-blind integer logic. Hand that same call a
 * `GridSpec` instead and the class is an axis-aligned board game with no
 * other edit, which is the entire argument for keeping projection out of the
 * navigation code.
 */
class HeroActor : public pixelroot32::core::Entity {
public:
    HeroActor(int startTileX, int startTileY);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /// The logical cell. In flight this still names the cell being LEFT, which
    /// is what any gameplay rule reading the hero's position wants.
    int tileX() const { return motion_.cellX; }
    int tileY() const { return motion_.cellY; }

private:
    /// One fixed-clock movement step: finish a step in flight, or start one.
    void logicStep();

    void updateProjectedPosition();

    pixelroot32::gameplay::GridMotion motion_;

    /// Milliseconds carried toward the next logic step.
    unsigned long logicAccumulator_ = 0;

    /// True when the hero's back is to the camera (walking up-left or up-right).
    bool facingAway_ = false;
    /// True when the pose must be mirrored to face screen-left.
    bool flipX_ = false;
};

}  // namespace iso_dungeon
