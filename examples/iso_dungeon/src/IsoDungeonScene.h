#pragma once

#include "core/Scene.h"

#include "IsoDungeonConstants.h"
#include "RoomRenderer.h"
#include "HeroActor.h"
#include "PropEntity.h"
#include "assets/DungeonTiles.h"

namespace iso_dungeon {

// The prop entities below are written out by hand rather than scanned out of
// the layout at runtime, and the asserts are what make that safe: each one
// pins a prop to the tile the layout gives it, and the counts prove the layout
// holds no prop the list forgot. Move a pillar in kRoomLayout and the build
// stops here.
static_assert(kRoomLayout[3][3] == 'A', "The altar entity is not on an 'A' tile.");
static_assert(kRoomLayout[3][5] == 'P', "The east pillar entity is not on a 'P' tile.");
static_assert(kRoomLayout[5][3] == 'P', "The west pillar entity is not on a 'P' tile.");
static_assert(countTiles('A') == 1, "The layout has an altar with no entity.");
static_assert(countTiles('P') == 2, "The layout has a pillar with no entity.");

/**
 * @class IsoDungeonScene
 * @brief A single isometric dungeon room the player walks around freely.
 *
 * The scene owns five entities and one decision: `depthComparator`.
 *
 * `gameplay::compareByDepthKey` is used rather than the older
 * `compareByBottomY` because the latter orders by world Y, which is the correct
 * paint order only while screen depth is a monotone function of world Y --
 * true for an axis-aligned top-down room, false the moment a projection
 * shears the grid. Each entity writes its own key from its projected anchor;
 * the engine never interprets the value.
 */
class IsoDungeonScene : public pixelroot32::core::Scene {
public:
    IsoDungeonScene() = default;

    void init() override;

    /**
     * @brief Lets the engine skip draw() and present() while the room looks
     *        identical to the frame already on the panel.
     *
     * The room, the altar and both pillars never move, so the hero is the only
     * entity whose appearance can change -- which makes this a one-line
     * question rather than a per-entity sweep. It matters far more than the
     * CPU it saves: `present()` pushes all 240x240 pixels over SPI every time
     * it is called, about 23 ms at 40 MHz, and the ST7789 holds the last frame
     * on its own memory in the meantime. Standing still therefore costs
     * nothing instead of costing the entire frame budget.
     */
    bool shouldRedrawFramebuffer() const override { return hero_.needsRedraw(); }

    /**
     * @brief Runs before `Renderer::beginFrame` and lets the room's snapshot
     *        tell the renderer its clear is about to be redone anyway.
     *
     * Purely an optimisation handshake: skipping it costs one redundant clear
     * per frame, never a wrong frame.
     */
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) override {
        room_.adviseFramebufferBeforeBeginFrame(renderer);
    }

private:
    RoomRenderer room_;
    HeroActor    hero_{kSpawnTileX, kSpawnTileY};

    PropEntity altar_{ALTAR_SPRITE, ALTAR_FOOT_Y, 3, 3};
    PropEntity pillarEast_{PILLAR_SPRITE, PILLAR_FOOT_Y, 5, 3};
    PropEntity pillarWest_{PILLAR_SPRITE, PILLAR_FOOT_Y, 3, 5};
};

}  // namespace iso_dungeon
