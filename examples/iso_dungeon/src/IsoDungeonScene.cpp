#include "IsoDungeonScene.h"

#include "core/Engine.h"
#include "gameplay/DepthCompare.h"
#include "graphics/Color.h"
#include "assets/DungeonPalette.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

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

    // Reserved here, off the game loop, because it allocates one logical
    // framebuffer (57,600 B at 240x240) -- see ARCH_MEMORY_SYSTEM.md.
    //
    // Discarded deliberately, and this cast is the record of that decision:
    // the room falls back to redrawing its 49 tiles every frame, which is
    // exactly what it did before the snapshot existed, so there is nothing for
    // THIS game to do about it. StaticLayerSnapshot has already logged the
    // size it could not get, which is the part a developer needs -- a second
    // message here would only repeat it.
    //
    // A game with a real recovery (drop to a smaller room, free another cache,
    // warn the player) reads the bool instead; reserveSnapshot() is
    // [[nodiscard]] so that choice is never made by omission.
    (void)room_.reserveSnapshot(engine.getRenderer());

    // Layer 0: the static room. Layer 1: everything the sort has to order.
    addEntity(&room_);
    addEntity(&altar_);
    addEntity(&pillarEast_);
    addEntity(&pillarWest_);
    addEntity(&hero_);
}

}  // namespace iso_dungeon
