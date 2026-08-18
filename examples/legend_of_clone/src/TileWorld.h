#pragma once

#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "graphics/Renderer.h"
#include <stdint.h>

/**
 * @file TileWorld.h
 * @brief A read-only view over one exported tilemap: what to draw, and what
 *        blocks movement.
 *
 * Both scenes need the same two answers out of their map — a layer the renderer
 * can take and a "can I stand here" for the player — and both maps are exported
 * by `tools/generate_assets.py` in the same shape. This is that pairing,
 * written once.
 *
 * It owns nothing and allocates nothing. The pixel data, the map indices and
 * the collision table are all `const` arrays in flash; this holds three
 * pointers into them. That is the whole reason the map costs no RAM: the
 * previous version expanded a character map into `.bss` at startup, which is
 * 1,320 bytes per scene an ESP32 does not have to spend.
 */
namespace legend_of_clone {

/**
 * @class TileWorld
 * @brief Binds an exported TileMap4bpp to its per-tile collision table.
 */
class TileWorld {
public:
    /**
     * @brief Points this view at an exported map.
     * @param map         Exported layer. Must outlive the view.
     * @param solidByTile TILE_SOLID from the same export, indexed by tile id.
     * @param tileCount   Entries in `solidByTile`.
     *
     * Safe to call again — a scene's init() runs on every swap, and re-pointing
     * at the same arrays is free.
     */
    void attach(const pixelroot32::graphics::TileMap4bpp* map,
                const bool* solidByTile,
                uint16_t tileCount);

    /// The drawable layer, or nullptr before attach().
    const pixelroot32::graphics::TileMap4bpp* layer() const { return map_; }

    /**
     * @brief Whether the cell at (col, row) blocks movement.
     * @return true for out-of-bounds cells — the edge of the map is a wall —
     *         and true when nothing is attached, so a scene that forgot to wire
     *         itself up freezes the player rather than dropping them through
     *         the floor.
     *
     * Read from the exported collision table, so it cannot drift away from the
     * art the way a hand-maintained copy would.
     */
    bool isSolid(int col, int row) const;

    /**
     * @brief Whether one world pixel is solid, refining isSolid() by the tile's
     *        own bitmap.
     * @param worldX  X in world pixels.
     * @param worldY  Y in world pixels.
     * @param erodePx Erosion radius forwarded to the engine helper; 0 disables it.
     * @return true when the cell is flagged solid AND the tile's 4bpp bitmap is
     *         opaque at that pixel. Out-of-bounds stays true, same as isSolid().
     *
     * The cell flag is checked first and short-circuits, so walkable ground
     * still costs one table lookup and never touches pixel data. Only cells the
     * export already calls solid pay for the bitmap read.
     *
     * This is what makes a round bush passable at its transparent corners while
     * a rectangular rock face stays a flat wall — without a second collision
     * table to keep in sync with the art.
     */
    bool isSolidAtPixel(int worldX, int worldY, int erodePx) const;

    /**
     * @brief Tile id at (col, row), or 0 outside the map.
     *
     * Exposed so a scene can ask what the player is standing on — a cave mouth,
     * a staircase — without keeping a second copy of the map.
     */
    uint8_t tileAt(int col, int row) const;

    bool isAttached() const { return map_ != nullptr; }

private:
    const pixelroot32::graphics::TileMap4bpp* map_ = nullptr;
    const bool* solid_ = nullptr;
    uint16_t tileCount_ = 0;
};

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
