#pragma once

#include "core/Scene.h"
#include "gameplay/RoomGraph.h"

#include "IsoDungeonConstants.h"
#include "RoomCatalog.h"
#include "RoomRenderer.h"
#include "HeroActor.h"
#include "PropEntity.h"

namespace iso_dungeon {

/**
 * @class IsoDungeonScene
 * @brief Three isometric rooms the player walks between through doorways.
 *
 * The scene owns three decisions the engine deliberately does not make.
 *
 * **`depthComparator`.** `gameplay::compareByDepthKey` is used rather than the
 * older `compareByBottomY`, because the latter orders by world Y, which is the
 * correct paint order only while screen depth is a monotone function of world
 * Y -- true for an axis-aligned top-down room, false the moment a projection
 * shears the grid. Each entity writes its own key from its projected anchor;
 * the engine never interprets the value.
 *
 * **What a room IS.** `gameplay::RoomGraph` stores connections, the current
 * index and a camera rect, and nothing else -- no tiles, no props, no spawn
 * points, because those differ for every game. `RoomCatalog.h` supplies them,
 * and the two are joined by using the same room index for both. Nothing checks
 * that alignment at runtime; `addRoom()` is called in catalog order and the
 * `kRoomCount` constant sizes both, which is what keeps it true.
 *
 * **When a door fires.** The graph has no notion of a player, so it cannot
 * know. This scene polls: after each update, a hero standing at rest on a
 * doorway tile is one that has just walked through it.
 *
 * ### The prop pool
 *
 * Props are a fixed pool of `kMaxPropsPerRoom` slots, re-pointed on every room
 * change rather than added to and removed from the scene. The entity list, its
 * render layers and the sort order all stay exactly as they were, so a door
 * cannot leave a dangling pointer in `Scene::entities` -- the failure that
 * would surface as a crash several rooms later instead of at the transition
 * that caused it. Unused slots are invisible and cost one comparison each in
 * an insertion sort over six entities.
 */
class IsoDungeonScene : public pixelroot32::core::Scene {
public:
    IsoDungeonScene() = default;

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Lets the engine skip draw() and present() while the dungeon looks
     *        identical to the frame already on the panel.
     *
     * The rooms and their props never move, so the hero is the only entity
     * whose appearance can change -- which makes this a one-line question
     * rather than a per-entity sweep. It matters far more than the CPU it
     * saves: `present()` pushes all 240x240 pixels over SPI every time it is
     * called, about 23 ms at 40 MHz, and the ST7789 holds the last frame on
     * its own memory in the meantime. Standing still therefore costs nothing
     * instead of costing the entire frame budget.
     *
     * A room change is the one event the hero's own flag cannot describe: the
     * background changes under a hero that may be drawn identically, so
     * `roomDirty_` forces the frame that repaints the new room.
     */
    bool shouldRedrawFramebuffer() const override {
        return roomDirty_ || hero_.needsRedraw();
    }

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

    /**
     * @brief Fires when the RoomGraph enters a room, including the first.
     *
     * Everything that has to change for the new room happens here rather than
     * at the call site, so the initial `enterRoom()` in init() and every later
     * door take exactly the same path. A first entry that behaved differently
     * from a door is how a scene ends up correct on boot and wrong afterwards.
     */
    void onRoomEnter(int fromIdx, int toIdx) override;

private:
    /// Walks the hero through a door: graph, then room contents, then hero.
    void enterRoom(uint16_t roomIdx, int arriveTileX, int arriveTileY);

    /// Bridges RoomGraph's C callback to the Scene::onRoomEnter virtual.
    static void onRoomEnterCallback(int fromIdx, int toIdx, void* userData);

    pixelroot32::gameplay::RoomGraph<kRoomCount> rooms_;

    RoomRenderer room_;
    HeroActor    hero_{kSpawnTileX, kSpawnTileY};
    PropEntity   props_[kMaxPropsPerRoom];

    /// Where the hero lands in the room being entered. Set by enterRoom()
    /// immediately before it asks the graph to transition, and read by
    /// onRoomEnter(), because RoomGraph's callback carries only the two room
    /// indices -- it has no field for a spawn point and should not grow one.
    int arriveTileX_ = kSpawnTileX;
    int arriveTileY_ = kSpawnTileY;

    /// Raised by a room change and cleared in draw(), never in update() --
    /// the same rule HeroActor's own flag follows, and for the same reason: a
    /// frame the engine chose to skip must leave the flag standing rather than
    /// clear it against a frame nobody saw. Starts raised so the opening frame
    /// is never skipped.
    bool roomDirty_ = true;
};

}  // namespace iso_dungeon
