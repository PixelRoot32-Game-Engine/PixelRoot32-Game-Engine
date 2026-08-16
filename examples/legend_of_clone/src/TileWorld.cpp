#include "TileWorld.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "platforms/PlatformMemory.h"

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

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
