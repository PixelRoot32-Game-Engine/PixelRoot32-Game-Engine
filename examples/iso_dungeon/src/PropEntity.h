#pragma once

#include "core/Entity.h"
#include "graphics/Renderer.h"

#include "IsoDungeonConstants.h"
#include "RoomCatalog.h"

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
    /// Starts empty and hidden. A prop only becomes real when a room claims a
    /// slot for it, which is what lets one fixed pool serve every room.
    PropEntity();

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Points this slot at one of the entering room's props.
     *
     * Re-pointing beats destroying and re-creating entities on every door: the
     * scene's entity list, its render layers and its sort order all stay
     * exactly as they were, so a room change cannot leave a dangling pointer
     * in `Scene::entities` -- which is the failure mode that would show up as
     * a crash three rooms later rather than at the transition that caused it.
     *
     * @param spec Prop to become. Its sprite must outlive this entity, which
     *             every `RoomCatalog.h` sprite does -- they are all statics.
     */
    void configure(const PropSpec& spec);

    /// Empties the slot: hidden, drawn by nobody, sorted harmlessly last.
    void release();

private:
    /// nullptr while the slot is unused. A pointer rather than the reference
    /// this class used to hold, because a reference cannot be re-seated and
    /// re-seating is the whole point of the pool.
    const pixelroot32::graphics::Sprite4bpp* sprite_ = nullptr;
    int footY_ = 0;
    int centreX_ = 0;
    int centreY_ = 0;
};

}  // namespace iso_dungeon
