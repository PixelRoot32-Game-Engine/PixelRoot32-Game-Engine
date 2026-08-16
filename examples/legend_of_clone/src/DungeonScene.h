#pragma once

#include "TopDownScene.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

namespace legend_of_clone {

/**
 * @class DungeonScene
 * @brief Four rooms underground, entered from the overworld's cave and left by
 *        the staircase you arrived beside.
 *
 * The point of this scene is that it is almost empty. Rooms, doorways, the
 * scrolling change between them, collision, the player and his walk cycle are
 * all TopDownScene's; a dungeon differs from an overworld in its map, its
 * tiles and what its special tiles do, and that is all this file contains.
 *
 * No enemies, no items, no keys and no locked doors yet.
 */
class DungeonScene : public TopDownScene {
protected:
    Setup setup() override;
    void drawStatusBar(pixelroot32::graphics::Renderer& renderer) override;
    void onPlayerSettled() override;

private:
    /// Points at the exported dungeon map. Three pointers, no pixel data.
    TileWorld world_;
};

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
