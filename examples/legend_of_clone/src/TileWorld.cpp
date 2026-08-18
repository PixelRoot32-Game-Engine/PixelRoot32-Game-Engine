#include "TileWorld.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "platforms/PlatformMemory.h"
#include "physics/TilePixelCollision.h"

namespace legend_of_clone {

void TileWorld::attach(const pixelroot32::graphics::TileMap4bpp* map,
                       const bool* solidByTile,
                       uint16_t tileCount) {
    map_ = map;
    solid_ = solidByTile;
    tileCount_ = tileCount;
}

uint8_t TileWorld::tileAt(int col, int row) const {
    if (map_ == nullptr) return 0;
    if (col < 0 || row < 0 || col >= map_->width || row >= map_->height) return 0;

    // PIXELROOT32_READ_BYTE_P, not a plain dereference: the indices live in the
    // scene flash section, which on ESP32 is not directly addressable through a
    // normal load. On native it compiles to the dereference anyway.
    return PIXELROOT32_READ_BYTE_P(map_->indices + (row * map_->width + col));
}

bool TileWorld::isSolid(int col, int row) const {
    if (map_ == nullptr || solid_ == nullptr) return true;
    if (col < 0 || row < 0 || col >= map_->width || row >= map_->height) return true;

    const uint8_t tile = tileAt(col, row);

    // A tile id past the end of the table means the map and the tileset were
    // exported out of step. Solid is the loud failure: the player runs into it
    // immediately instead of walking through a room that should not exist.
    if (tile >= tileCount_) return true;

    return solid_[tile];
}

bool TileWorld::isSolidAtPixel(int worldX, int worldY, int erodePx) const {
    if (map_ == nullptr) return true;

    const int tileW = map_->tileWidth;
    const int tileH = map_->tileHeight;
    if (tileW <= 0 || tileH <= 0) return true;

    // Integer division truncates toward zero, so a negative world pixel would
    // land on col/row 0 and read a legal cell. Reject it before the divide, or
    // the player walks off the top-left edge of the world.
    if (worldX < 0 || worldY < 0) return true;

    const int col = worldX / tileW;
    const int row = worldY / tileH;

    // Cheap gate first: a cell the export never called solid can't be solid at
    // any pixel, and this keeps walkable ground off the bitmap path entirely.
    // It also carries the out-of-bounds and unattached cases for us.
    if (!isSolid(col, row)) return false;

    // isSolid() already rejected out-of-range ids, so this indexes the tileset
    // in range.
    const uint8_t tile = tileAt(col, row);
    if (map_->tiles == nullptr) return true;

    return pixelroot32::physics::isTilePixelSolid(
        &map_->tiles[tile],
        worldX % tileW,
        worldY % tileH,
        erodePx);
}

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
