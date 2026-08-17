#include "IsoDungeonScene.h"

#include "gameplay/DepthCompare.h"
#include "graphics/Color.h"
#include "assets/DungeonPalette.h"

namespace pr32 = pixelroot32;

namespace iso_dungeon {

namespace gfx = pr32::graphics;

void IsoDungeonScene::init() {
    Scene::init();

    // One palette for tiles and sprites alike: this room has a single colour
    // scheme, so splitting it across background and sprite tables would buy
    // nothing and cost a second 16-entry lookup to keep in sync.
    gfx::setDualCustomPalette(DUNGEON_PALETTE_RGB565, DUNGEON_PALETTE_RGB565);

    depthComparator = &pr32::gameplay::compareByDepthKey;
    depthSortEnabled = true;

    // Layer 0: the static room. Layer 1: everything the sort has to order.
    addEntity(&room_);
    addEntity(&altar_);
    addEntity(&pillarEast_);
    addEntity(&pillarWest_);
    addEntity(&hero_);
}

}  // namespace iso_dungeon
