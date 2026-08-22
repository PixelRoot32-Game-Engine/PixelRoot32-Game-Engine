#include "IsoDungeonScene.h"

#include "core/Engine.h"
#include "gameplay/DepthCompare.h"
#include "graphics/Color.h"
#include "graphics/SpanTable.h"
#include "platforms/PlatformMemory.h"
#include "assets/DungeonTiles.h"
#include "assets/IsoDungeonRoomTileMap.h"
#include "assets/IsoDungeonRoomTileMapPalette.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace gameplay = pr32::gameplay;

void IsoDungeonScene::init() {
    Scene::init();

    // Wires the exported TileMap4bpp objects to their flash-resident tileset,
    // foot table and index arrays. Must run before RoomRenderer::setRoom()
    // hands one of them to the renderer, which is why it is the first thing
    // here -- an exported map is inert until its init() has run.
    iso_dungeon::init();

    // Build per-row opaque span metadata for each tile so drawSpriteInternal
    // can skip leading/trailing transparent nibbles inside the projected
    // tilemap loop (change iso-perf-blit-fastpath).
    //
    // Native (PC/SDL2): the Sprite4bpp descriptors live in writable RAM, so
    // we can const_cast and assign the span pointers in place.
    //
    // ESP32: the Sprite4bpp descriptors in DungeonTiles.h may be placed in
    // flash by the linker. Writing rowMinX/rowMaxX back to them is UB and
    // triggers a LoadStoreError panic on first draw. The optimization is
    // therefore gated off on ESP32. To re-enable it, change `static const`
    // to `static` in DungeonTiles.h's Sprite4bpp declarations (the linker
    // then places them in RAM).
    //
    // computeSpanTable reads pixel data via PIXELROOT32_READ_BYTE_P which
    // expands to pgm_read_byte on ESP32, so reading is safe on both targets.
#if !defined(ESP32)
    {
        static uint8_t sFloorAMinX[iso_dungeon::FLOOR_A_HEIGHT];
        static uint8_t sFloorAMaxX[iso_dungeon::FLOOR_A_HEIGHT];
        static uint8_t sFloorBMinX[iso_dungeon::FLOOR_B_HEIGHT];
        static uint8_t sFloorBMaxX[iso_dungeon::FLOOR_B_HEIGHT];
        static uint8_t sFloorAccentMinX[iso_dungeon::FLOOR_ACCENT_HEIGHT];
        static uint8_t sFloorAccentMaxX[iso_dungeon::FLOOR_ACCENT_HEIGHT];
        static uint8_t sWallMinX[iso_dungeon::WALL_HEIGHT];
        static uint8_t sWallMaxX[iso_dungeon::WALL_HEIGHT];
        static uint8_t sDoorNeMinX[iso_dungeon::DOOR_NE_HEIGHT];
        static uint8_t sDoorNeMaxX[iso_dungeon::DOOR_NE_HEIGHT];
        static uint8_t sDoorNwMinX[iso_dungeon::DOOR_NW_HEIGHT];
        static uint8_t sDoorNwMaxX[iso_dungeon::DOOR_NW_HEIGHT];

        pr32::graphics::computeSpanTable(const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_A_SPRITE),       sFloorAMinX,     sFloorAMaxX);
        pr32::graphics::computeSpanTable(const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_B_SPRITE),       sFloorBMinX,     sFloorBMaxX);
        pr32::graphics::computeSpanTable(const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_ACCENT_SPRITE),  sFloorAccentMinX,sFloorAccentMaxX);
        pr32::graphics::computeSpanTable(const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::WALL_SPRITE),         sWallMinX,       sWallMaxX);
        pr32::graphics::computeSpanTable(const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::DOOR_NE_SPRITE),       sDoorNeMinX,     sDoorNeMaxX);
        pr32::graphics::computeSpanTable(const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::DOOR_NW_SPRITE),       sDoorNwMinX,     sDoorNwMaxX);

        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_A_SPRITE).rowMinX      = sFloorAMinX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_A_SPRITE).rowMaxX      = sFloorAMaxX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_B_SPRITE).rowMinX      = sFloorBMinX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_B_SPRITE).rowMaxX      = sFloorBMaxX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_ACCENT_SPRITE).rowMinX = sFloorAccentMinX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::FLOOR_ACCENT_SPRITE).rowMaxX = sFloorAccentMaxX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::WALL_SPRITE).rowMinX        = sWallMinX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::WALL_SPRITE).rowMaxX        = sWallMaxX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::DOOR_NE_SPRITE).rowMinX      = sDoorNeMinX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::DOOR_NE_SPRITE).rowMaxX      = sDoorNeMaxX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::DOOR_NW_SPRITE).rowMinX      = sDoorNwMinX;
        const_cast<pr32::graphics::Sprite4bpp&>(iso_dungeon::DOOR_NW_SPRITE).rowMaxX      = sDoorNwMaxX;
    }
#endif  // !ESP32

    // One palette for tiles and sprites alike: this dungeon has a single colour
    // scheme, so splitting it across background and sprite tables would buy
    // nothing and cost a second 16-entry lookup to keep in sync.
    //
    // setDualCustomPalette, NOT setBackgroundCustomPalette: the exported
    // mapping is shared by the hero and the props, and sprites resolve through
    // a different palette bank than tilemaps do. Loading only the background
    // bank would leave every sprite reading a stale table.
    gfx::setDualCustomPalette(TILEMAP_PALETTE_DATA, TILEMAP_PALETTE_DATA);

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

    // Rooms and connections both come from kRoomLayer, which RoomCatalog.h
    // derives from the door table at compile time. This is the same call a
    // dungeon authored in the PixelRoot32 Tilemap Editor would make -- the
    // editor emits `RoomData` and `RoomLayer`, and nothing here would change.
    //
    // A short count has two possible causes, and neither is survivable here.
    // A rejected layer builds nothing at all. A layer larger than the graph's
    // capacity builds a PARTIAL graph -- which is a normal outcome for
    // buildRoomGraph, but not for this scene: RoomGraph<kRoomCount> is sized
    // from the same constant kRooms is, so truncation would mean the two have
    // drifted apart, and the rooms that survived would be indexed against a
    // catalog that no longer matches.
    //
    // Bailing out leaves currentRoomIndex() at 0xFFFF and the scene with no
    // entities. update() checks isValidIdx() before polling doors, so it runs
    // empty rather than indexing kRooms with a sentinel.
    if (gameplay::buildRoomGraph(kRoomLayer, rooms_) != kRoomCount) {
        return;
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
    //
    // Polling once per frame is enough, and that deserves a proof rather than
    // a shrug, because it is not obvious: HeroActor::update drains a fixed
    // logic clock with a `while`, so a long frame runs SEVERAL steps, and a
    // hero that stepped onto a door and off it between two polls would take
    // the door with it. Three facts rule that out.
    //
    // 1. Engine calls inputManager.update() once per frame, before
    //    sceneManager.update(), and isButtonDown() only reads the cached
    //    buttonState[]. Every logicStep in one drain therefore sees the SAME
    //    held direction -- input cannot change mid-frame.
    // 2. logicStep only reads input on its at-rest branch, and a step's
    //    direction is fixed at beginStep, so the direction that reached the
    //    door is the direction still being held.
    // 3. everyDoorIsADeadEnd asserts that the tile beyond a door, in the
    //    direction that reaches it, is solid.
    //
    // So the step that would carry the hero off the door is exactly the one
    // isSolidTile refuses. Leaving requires reversing, which needs a direction
    // that cannot arrive until the next frame's inputManager.update(). The
    // hero always ends the frame at rest ON the door, and this poll sees it.
    //
    // Fact 3 is the one a future edit can break -- and it fails loudly, at the
    // static_assert, rather than as a door that works except on slow frames.
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
