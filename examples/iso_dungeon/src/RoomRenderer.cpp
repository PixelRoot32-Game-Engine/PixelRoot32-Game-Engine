#include "RoomRenderer.h"

#include "math/Vector2.h"
#include "IsoDraw.h"
#include "assets/DungeonTiles.h"

namespace pr32 = pixelroot32;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

RoomRenderer::RoomRenderer()
    : core::Entity(math::Vector2::ZERO(), kDisplaySize, kDisplaySize,
                   core::EntityType::GENERIC) {
    setRenderLayer(0);
}

void RoomRenderer::update(unsigned long deltaTime) {
    (void)deltaTime;
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

    // Row-major iteration IS the isometric painter's order here, and not by
    // luck: screen depth under kTileProjection is 8 * (x + y), so a tile is
    // always drawn after both (x-1, y) and (x, y-1) -- exactly the two
    // neighbours whose extruded blocks can overlap it from behind. No sort.
    for (int y = 0; y < kRoomTiles; ++y) {
        for (int x = 0; x < kRoomTiles; ++x) {
            const char cell = kRoomLayout[y][x];
            const int cx = gameplay::cellToScreenX(x, y, kTileProjection);
            const int cy = gameplay::cellToScreenY(x, y, kTileProjection);

            switch (cell) {
                case 'W':
                    drawAtCell(renderer, WALL_SPRITE, WALL_FOOT_Y, cx, cy);
                    break;
                case 'D':
                    drawAtCell(renderer, DOOR_NE_SPRITE, DOOR_NE_FOOT_Y, cx, cy);
                    break;
                case 'E':
                    drawAtCell(renderer, DOOR_NW_SPRITE, DOOR_NW_FOOT_Y, cx, cy);
                    break;
                default: {
                    // Floor. An isometric room has to draw its ground
                    // explicitly -- there is no rectangular frame underneath to
                    // fall back on the way an axis-aligned board has.
                    //
                    // The checkerboard is not decoration either: a single flat
                    // tone gives the eye nothing to judge which cell the hero
                    // is standing on, which is the one thing a player of an
                    // isometric game constantly needs to know.
                    const bool accent = (cell == 'a' || cell == 'A');
                    const gfx::Sprite4bpp& floor =
                        accent            ? FLOOR_ACCENT_SPRITE
                        : ((x + y) & 1)   ? FLOOR_B_SPRITE
                                          : FLOOR_A_SPRITE;
                    drawAtCell(renderer, floor, FLOOR_A_FOOT_Y, cx, cy);
                    break;
                }
            }
        }
    }
}

}  // namespace iso_dungeon
