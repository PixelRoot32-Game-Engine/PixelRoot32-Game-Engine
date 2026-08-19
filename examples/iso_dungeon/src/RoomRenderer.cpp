#include "RoomRenderer.h"

#include "math/Vector2.h"
#include "assets/IsoDungeonRoomTileMap.h"

namespace pr32 = pixelroot32;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

RoomRenderer::RoomRenderer()
    : core::Entity(math::Vector2::ZERO(), kDisplaySize, kDisplaySize,
                   core::EntityType::GENERIC) {
    setRenderLayer(0);

    // Nothing to wire. Every field the renderer used to fill in here --
    // geometry, tileset, tile count, foot table -- is set by the export's
    // init(), which the scene calls before the first draw. setRoom() only has
    // to choose which of the exported maps to point at.
}

void RoomRenderer::update(unsigned long deltaTime) {
    (void)deltaTime;
}

bool RoomRenderer::reserveSnapshot(gfx::Renderer& renderer) {
    return snapshot_.allocateForRenderer(renderer);
}

void RoomRenderer::adviseFramebufferBeforeBeginFrame(gfx::Renderer& renderer) const {
    snapshot_.adviseFramebufferBeforeBeginFrame(renderer);
}

void RoomRenderer::setRoom(const RoomSpec& room) {
    room_ = &room;

    // kRooms and ROOM_LAYERS are indexed alike by construction -- the export
    // emits one map per catalog entry, in catalog order -- so the room's
    // position in the catalog IS its layer index. Deriving it here rather
    // than storing it keeps setRoom()'s signature as it was.
    const auto index = static_cast<std::size_t>(&room - &kRooms[0]);
    map_ = ROOM_LAYERS[index];

    // The cached picture is of the room being left. Dropping it is what makes
    // the next draw() take the drawTiles() path and re-capture; without it the
    // snapshot would keep restoring the previous room over the new one, and it
    // would never self-correct because that path never redraws.
    snapshot_.invalidate();
}

void RoomRenderer::draw(gfx::Renderer& renderer) {
    // No backdrop fill. `Renderer::beginFrame` has already cleared the
    // framebuffer to black, and under this example's palette kVoidColor
    // (index 1) IS 0x0000 -- so painting it would write 57,600 identical
    // bytes over 57,600 identical bytes. On the ESP32 that is a second
    // full-screen pass per frame for no visible pixel.
    //
    // The backdrop is still deliberate, not dropped: the room simply does not
    // cover the whole display, and everything it does not cover reads as the
    // drop beyond the floor. Change the palette so index 1 stops being black
    // and this needs a fill again -- kVoidColor stays in
    // IsoDungeonConstants.h naming that intent.

    // A room never changes while the hero is standing in it, so it is worth
    // drawing exactly once per visit. On every later frame the snapshot puts
    // it back -- and with dirty regions on, only
    // over the cells the hero and props disturbed last frame, which is a few
    // hundred bytes against 35,072 pixels of 4bpp decode.
    //
    // drawTiles() hands the tile layer to `drawTileMap` through a projection
    // (PIXELROOT32_ENABLE_TILEMAP_PROJECTION), so the diamonds are placed by
    // the engine's projected path rather than by a hand-rolled sprite loop.
    // The snapshot still earns its keep on top of that: it removes the whole
    // draw call on a hit -- 49 cells of 4bpp decode plus the tileset's
    // cull-padding scan -- not just the loop that used to build it.
    if (map_ == nullptr) {
        return;  // before the scene's init() has chosen a room
    }

    if (snapshot_.restore(renderer)) {
        return;
    }

    drawTiles(renderer);

    // Captured here rather than in the scene: this is the exact moment the
    // framebuffer holds the static room and nothing else. The props and the
    // hero are separate entities drawn after this one (layer 1), so they are
    // correctly absent from the snapshot -- they have to stay in the per-frame
    // depth sort, since the hero passes both behind and in front of them.
    (void)snapshot_.capture(renderer);
}

void RoomRenderer::drawTiles(gfx::Renderer& renderer) {
    // Row-major iteration IS the isometric painter's order here, and not by
    // luck: screen depth under kTileProjection is 8 * (x + y), so a tile is
    // always drawn after both (x-1, y) and (x, y-1) -- exactly the two
    // neighbours whose extruded blocks can overlap it from behind. No sort.
    //
    // `gameplay::rowMajorIsPainterOrder` (include/math/Projection.h) is
    // asserted against `kTileProjection` in IsoDungeonConstants.h, so editing
    // the projection breaks the build here rather than producing walls that
    // paint over the hero. `drawTileMap`'s projected path iterates row-major
    // too and never sorts, so that guarantee still governs draw order here.
    //
    // map_ is exported data. The checkerboard rule that used to pick FLOOR_A
    // or FLOOR_B per cell at runtime was resolved at export time and is not
    // reproduced anywhere in this example any more -- see
    // assets/IsoDungeonRoomTileMap.cpp's ROOMn_INDICES.
    renderer.drawTileMap(*map_, 0, 0, gfx::LayerType::Static, ISO_PROJECTION);
}

}  // namespace iso_dungeon
