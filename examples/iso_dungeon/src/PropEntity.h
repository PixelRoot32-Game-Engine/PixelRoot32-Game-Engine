#pragma once

#include "core/Entity.h"
#include "graphics/Renderer.h"

#include "IsoDungeonConstants.h"

namespace iso_dungeon {

/**
 * @class PropEntity
 * @brief A block standing on one tile -- the altar, a pillar -- that the hero
 *        can walk both behind and in front of.
 *
 * Props are entities rather than part of the static room pass for exactly one
 * reason: the hero can reach tiles on both sides of them, so whether a prop
 * draws over or under the hero changes from frame to frame. That decision
 * belongs to the same depth sort the hero participates in, and nothing else
 * here can make it.
 *
 * This is also where the isometric bomberbot experiment failed and this room
 * succeeds. Correct occlusion needs every occluder in the sorted pass, and a
 * 13x11 bomberbot board carries ~114 blocks against `MAX_ENTITIES` of 64 --
 * with `Scene::sortEntities()` being an insertion sort on top. A dungeon room
 * has three props. The capability did not change; the entity budget did.
 */
class PropEntity : public pixelroot32::core::Entity {
public:
    /// @param sprite Block bitmap.
    /// @param footY  Its `*_FOOT_Y` (see IsoDraw.h).
    PropEntity(const pixelroot32::graphics::Sprite4bpp& sprite, int footY,
               int tileX, int tileY);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    const pixelroot32::graphics::Sprite4bpp& sprite_;
    int footY_;
    int centreX_;
    int centreY_;
};

}  // namespace iso_dungeon
