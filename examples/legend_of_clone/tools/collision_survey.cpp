/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Reproduces every measured figure the example's README quotes about tile
 * collision, and asserts the properties the three CollisionModes must hold.
 *
 * The README used to state its numbers with nothing in the repository backing
 * them, which made them unverifiable by anyone but their author. This tool is
 * the backing. Build and run it as documented in the README's "Tile collision"
 * section; it exits non-zero if any asserted property fails.
 *
 * It links the example's own TileWorld and exported maps against the engine's
 * isTilePixelSolid, and mirrors PlayerActor::canOccupy rather than including it,
 * so it stays free of the Entity/Engine graph and builds as a plain host binary.
 * That mirroring is the one thing to keep in sync: if canOccupy changes shape,
 * change canOccupyPerPixel and canOccupyWholeTile below with it.
 */
#include "GameConstants.h"
#include "TileWorld.h"
#include "assets/OverworldTileMap.h"
#include "assets/DungeonTileMap.h"

#include <cstdio>

using namespace legend_of_clone;

namespace {

/** Mirrors PlayerActor::canOccupy's WholeTile branch. */
bool canOccupyWholeTile(const TileWorld& world, int x, int y) {
    const int leftCol   = x / kTileSize;
    const int rightCol  = (x + kPlayerSize - 1) / kTileSize;
    const int topRow    = y / kTileSize;
    const int bottomRow = (y + kPlayerSize - 1) / kTileSize;
    for (int row = topRow; row <= bottomRow; ++row) {
        for (int col = leftCol; col <= rightCol; ++col) {
            if (world.isSolid(col, row)) return false;
        }
    }
    return true;
}

/** Mirrors PlayerActor::canOccupy's per-pixel branch. */
bool canOccupyPerPixel(const TileWorld& world, int x, int y, int erodePx) {
    for (int py = 0; py < kPlayerSize; ++py) {
        for (int px = 0; px < kPlayerSize; ++px) {
            if (world.isSolidAtPixel(x + px, y + py, erodePx)) return false;
        }
    }
    return true;
}

int failures = 0;

void expect(const char* what, bool actual, bool wanted) {
    const bool ok = (actual == wanted);
    std::printf("  %-34s %-9s %s\n", what,
                actual ? "walkable" : "blocked",
                ok ? "" : (wanted ? "<== EXPECTED WALKABLE" : "<== EXPECTED BLOCKED"));
    if (!ok) ++failures;
}

/**
 * Every 16x16 body position over the map, stepped one pixel.
 *
 * A tile-aligned sweep cannot see per-pixel differences at all: the modes only
 * diverge when the body straddles a tile's transparent margin, which never
 * happens at offset 0. Hence one pixel.
 */
long sweep(const TileWorld& world, int mapCols, int mapRows, int erodePx, bool perPixel, long& total) {
    const int w = mapCols * kTileSize;
    const int h = mapRows * kTileSize;
    long walkable = 0;
    total = 0;
    for (int y = 0; y + kPlayerSize <= h; ++y) {
        for (int x = 0; x + kPlayerSize <= w; ++x) {
            ++total;
            const bool free = perPixel ? canOccupyPerPixel(world, x, y, erodePx)
                                       : canOccupyWholeTile(world, x, y);
            if (free) ++walkable;
        }
    }
    return walkable;
}

} // namespace

int main() {
    overworld::init();
    dungeon::init();

    TileWorld ow;
    ow.attach(&overworld::terrain, overworld::TILE_SOLID, overworld::TILE_COUNT);
    TileWorld dg;
    dg.attach(&dungeon::terrain, dungeon::TILE_SOLID, dungeon::TILE_COUNT);

    const int W = overworld::MAP_WIDTH  * kTileSize;
    const int H = overworld::MAP_HEIGHT * kTileSize;

    std::printf("overworld %dx%d px, tile %d px, erosion %d px\n\n",
                W, H, kTileSize, kTileSolidErosionPx);

    // The map edge is a wall in every mode. All FOUR edges: guarding only the
    // negative ones leaves the far edge open, which is how this example
    // shipped a walkable right and bottom border once already.
    std::printf("Edges (per-pixel, eroded):\n");
    expect("west   x = -16",  canOccupyPerPixel(ow, -16, 160, kTileSolidErosionPx), false);
    expect("north  y = -16",  canOccupyPerPixel(ow, 112, -16, kTileSolidErosionPx), false);
    expect("east   x = W",    canOccupyPerPixel(ow, W,   160, kTileSolidErosionPx), false);
    expect("south  y = H",    canOccupyPerPixel(ow, 112, H,   kTileSolidErosionPx), false);

    std::printf("\nEdges (whole-tile):\n");
    expect("west   x = -16",  canOccupyWholeTile(ow, -16, 160), false);
    expect("north  y = -16",  canOccupyWholeTile(ow, 112, -16), false);
    expect("east   x = W",    canOccupyWholeTile(ow, W,   160), false);
    expect("south  y = H",    canOccupyWholeTile(ow, 112, H  ), false);

    // The cells the game actually puts the player on must stay reachable.
    std::printf("\nGameplay cells (per-pixel, eroded):\n");
    expect("player spawn",   canOccupyPerPixel(ow, kPlayerStartCol * kTileSize,
                                               kPlayerStartRow * kTileSize, kTileSolidErosionPx), true);
    expect("cave exit",      canOccupyPerPixel(ow, kCaveExitCol * kTileSize,
                                               kCaveExitRow * kTileSize, kTileSolidErosionPx), true);
    expect("dungeon entry",  canOccupyPerPixel(dg, kDungeonEntryCol * kTileSize,
                                               kDungeonEntryRow * kTileSize, kTileSolidErosionPx), true);

    // The figures the README quotes.
    long total = 0;
    const long wholeTile = sweep(ow, overworld::MAP_WIDTH, overworld::MAP_HEIGHT, 0, false, total);
    const long perPixel  = sweep(ow, overworld::MAP_WIDTH, overworld::MAP_HEIGHT, 0, true,  total);
    const long eroded    = sweep(ow, overworld::MAP_WIDTH, overworld::MAP_HEIGHT,
                                 kTileSolidErosionPx, true, total);

    std::printf("\nWalkable body positions, 1 px step, %ld sampled:\n", total);
    std::printf("  WholeTile       %ld\n", wholeTile);
    std::printf("  PerPixel        %ld\n", perPixel);
    std::printf("  PerPixelEroded  %ld\n", eroded);

    // PerPixel can only ever free up positions WholeTile blocked, never the
    // reverse; erosion can only free up more still.
    if (perPixel < wholeTile) {
        std::printf("  <== PerPixel below WholeTile: impossible, the gate is a superset\n");
        ++failures;
    }
    if (eroded < perPixel) {
        std::printf("  <== Eroded below PerPixel: impossible, erosion only shrinks solids\n");
        ++failures;
    }
    if (perPixel == wholeTile) {
        std::printf("\n  PerPixel matches WholeTile exactly: no terrain tile on this map has a\n"
                    "  transparent pixel, so only erosion changes behaviour here.\n");
    }

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
