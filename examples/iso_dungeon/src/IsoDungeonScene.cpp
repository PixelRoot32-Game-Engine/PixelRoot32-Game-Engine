#include "IsoDungeonScene.h"

#include "core/Engine.h"
#include "gameplay/DepthCompare.h"
#include "graphics/Color.h"
#include "assets/DungeonPalette.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace gameplay = pr32::gameplay;

void IsoDungeonScene::init() {
    Scene::init();

    // One palette for tiles and sprites alike: this dungeon has a single colour
    // scheme, so splitting it across background and sprite tables would buy
    // nothing and cost a second 16-entry lookup to keep in sync.
    gfx::setDualCustomPalette(DUNGEON_PALETTE_RGB565, DUNGEON_PALETTE_RGB565);

    depthComparator = &gameplay::compareByDepthKey;
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
    // One buffer serves all three rooms: they share a resolution, and a room
    // change invalidates the contents rather than the allocation. Rooms are
    // therefore free after the first.
    //
    // A game with a real recovery (drop to a smaller room, free another cache,
    // warn the player) reads the bool instead; reserveSnapshot() is
    // [[nodiscard]] so that choice is never made by omission.
    (void)room_.reserveSnapshot(engine.getRenderer());

    // Every room is one screen, so nothing scrolls and every camera rect is
    // the whole display. The rects are still filled in honestly rather than
    // zeroed: a later camera would clamp to them, and a zero rect would pin it
    // to the top-left corner in a way that looks like a camera bug rather than
    // like missing data.
    for (uint16_t i = 0; i < kRoomCount; ++i) {
        const uint16_t idx = rooms_.addRoom(pr32::math::toScalar(0),
                                            pr32::math::toScalar(0),
                                            pr32::math::toScalar(kDisplaySize),
                                            pr32::math::toScalar(kDisplaySize));
        // addRoom returns 0xFFFF when full. RoomGraph<kRoomCount> is sized from
        // the same constant this loop runs to, so a mismatch is a bug in the
        // catalog rather than a runtime condition -- but a silent 0xFFFF would
        // desynchronise every index below, so it is worth not assuming.
        if (idx != i) {
            return;
        }
    }

    // Connections mirror the doors, in both directions. `dirFromCellStep`
    // derives the slot from the step the player takes to reach the door, so
    // the graph and the layout cannot disagree about which way is Up.
    for (uint16_t from = 0; from < kRoomCount; ++from) {
        const RoomSpec& spec = kRooms[from];
        for (uint8_t d = 0; d < spec.doorCount; ++d) {
            const DoorSpec& door = spec.doors[d];
            // A door sits ON the room's edge, so the step that reaches it
            // points from the room's interior outward -- which is the
            // direction the connection describes.
            const int dx = door.tileX == 0 ? -1 : (door.tileX == kRoomTiles - 1 ? 1 : 0);
            const int dy = door.tileY == 0 ? -1 : (door.tileY == kRoomTiles - 1 ? 1 : 0);
            rooms_.connect(from, door.targetRoom, dirFromCellStep(dx, dy));
        }
    }

    rooms_.setOnEnter(onRoomEnterCallback, this);
    setRoomGraph(&rooms_);

    // Layer 0: the static room. Layer 1: everything the sort has to order.
    addEntity(&room_);
    for (uint8_t i = 0; i < kMaxPropsPerRoom; ++i) {
        addEntity(&props_[i]);
    }
    addEntity(&hero_);

    // Populates the room contents through the same path a door takes.
    enterRoom(0, kSpawnTileX, kSpawnTileY);
}

void IsoDungeonScene::enterRoom(uint16_t roomIdx, int arriveTileX, int arriveTileY) {
    if (!rooms_.isValidIdx(roomIdx)) {
        return;
    }
    // Stashed for onRoomEnter, which is where the work happens. RoomGraph's
    // callback signature carries the two room indices and a void* and nothing
    // else, by design -- a spawn point is game data, not graph data.
    arriveTileX_ = arriveTileX;
    arriveTileY_ = arriveTileY;
    rooms_.enterRoom(roomIdx, /*camera=*/nullptr);
}

void IsoDungeonScene::onRoomEnter(int /*fromIdx*/, int toIdx) {
    const RoomSpec& spec = kRooms[toIdx];

    room_.setRoom(spec);
    hero_.enterRoom(spec, arriveTileX_, arriveTileY_);

    // Fill the pool from the front and empty whatever the previous room used
    // beyond it, so no slot survives holding a prop from the room just left.
    for (uint8_t i = 0; i < kMaxPropsPerRoom; ++i) {
        if (i < spec.propCount) {
            props_[i].configure(spec.props[i]);
        } else {
            props_[i].release();
        }
    }

    // Two separate things have to be told the screen is stale, and neither
    // implies the other.
    //
    // roomDirty_ makes the engine call draw() at all -- without it, a hero that
    // happens to land on the same pixel in the same pose reports "nothing
    // changed" and the panel keeps showing the previous room.
    //
    // forceFullRedraw() decides what draw() is allowed to skip. The dirty grid
    // only knows which cells the hero disturbed, so a selective clear plus a
    // per-cell snapshot restore would repaint a hero-sized patch and leave the
    // rest of the OLD room standing. Marking the grid fully dirty forces the
    // whole-framebuffer clear that a background swap actually needs.
    roomDirty_ = true;
    engine.getRenderer().forceFullRedraw();
}

void IsoDungeonScene::onRoomEnterCallback(int fromIdx, int toIdx, void* userData) {
    if (userData == nullptr) {
        return;
    }
    static_cast<IsoDungeonScene*>(userData)->onRoomEnter(fromIdx, toIdx);
}

void IsoDungeonScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);

    const uint16_t current = rooms_.currentRoomIndex();
    if (!rooms_.isValidIdx(current)) {
        return;  // init() bailed out before entering a room
    }

    // Polled rather than pushed from HeroActor, because "I am standing on a
    // door" is a fact about the room and "that means leave" is a rule about
    // the game. The hero knows the first and has no business deciding the
    // second -- it does not even hold the door table.
    //
    // Only the at-rest reading counts. In flight, tileX()/tileY() still name
    // the cell being LEFT, so acting on it would fire the transition a whole
    // step early, from the tile before the door.
    if (hero_.isMoving()) {
        return;
    }

    const DoorSpec* door = doorAt(kRooms[current], hero_.tileX(), hero_.tileY());
    if (door == nullptr) {
        return;
    }

    // The arrival tile is never itself a door -- doorsAreWellFormed() asserts
    // it at compile time -- so this cannot re-enter on the next frame and
    // bounce the player between two rooms.
    enterRoom(door->targetRoom, door->arriveTileX, door->arriveTileY);
}

void IsoDungeonScene::draw(gfx::Renderer& renderer) {
    Scene::draw(renderer);

    // Cleared here rather than in update(), and rather than in
    // shouldRedrawFramebuffer(), for the reason HeroActor clears its own flag
    // in draw(): this is the only place that proves the new room actually
    // reached the framebuffer. A frame the engine skipped leaves the flag
    // standing instead of clearing it against a frame nobody saw.
    roomDirty_ = false;
}

}  // namespace iso_dungeon
