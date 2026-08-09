#include "assets/OverworldMap.h"
#include "assets/OverworldTiles.h"

namespace zelda_overworld {

namespace gfx = pixelroot32::graphics;

namespace {

// ---------------------------------------------------------------------------
// The character map — the single source of truth for the world
// ---------------------------------------------------------------------------
//
// Four adjacent overworld screens, 15x11 tiles each, laid out 2x2:
//
//     +----------------+----------------+
//     | 0  north-west  | 1  north-east  |
//     +----------------+----------------+
//     | 2  START       | 3  south-east  |
//     +----------------+----------------+
//
// Legend:
//   '.'  sand           walkable
//   ','  grass tuft     walkable
//   'T'  tree           blocking
//   '#'  mountain       blocking
//   'C'  cave mouth     blocking (caves are not enterable in iteration 1)
//
// The screens are hand-authored to read like the start area of the NES first
// quest — mountains along the north, the cave in the rock face on the start
// screen, woods to the east. They are not tile-exact rips of the original ROM.
//
// Room seams have to line up by hand: the open cells on one side of a border
// must face open cells on the other, or the connection is decorative and the
// player walks into a wall. Every seam below is marked where it occurs.
constexpr const char* WORLD_ROWS[kWorldRows] = {
//   room 0 (north-west)    room 1 (north-east)
    "###############" "###############",  //  0
    "#....,....##..#" "#..,.......,..#",  //  1
    "#..##.....##..#" "#....TTT......#",  //  2
    "#..##.........." "....TTTTT.....#",  //  3  <- seam 0<->1
    "#.....,........" ".....TTT......#",  //  4  <- seam 0<->1
    "#.........,...." "......T.......#",  //  5  <- seam 0<->1
    "#...T.........#" "#....,........#",  //  6
    "#..TTT........#" "#........###..#",  //  7
    "#...T..,......#" "#..,.....###..#",  //  8
    "#.............#" "#........###..#",  //  9
    "####.....######" "######.....####",  // 10  <- seam 0<->2 and 1<->3
//   room 2 (start)         room 3 (south-east)
    "####.....######" "######.....####",  // 11  <- seam 0<->2 and 1<->3
    "#....,....#####" "#....,........#",  // 12
    "#..#####..#...#" "#......###....#",  // 13
    "#..##CC#......#" "#.....#####...#",  // 14
    "#..##CC#......." "......#####...#",  // 15  <- seam 2<->3
    "#..##..#......." ".......###....#",  // 16  <- seam 2<->3
    "#..,..........#" "#....,........#",  // 17
    "#....T....,...#" "#..T.......T..#",  // 18
    "#...TTT.......#" "#.TTT.....TTT.#",  // 19
    "#....T........#" "#..T..,....T..#",  // 20
    "###############" "###############",  // 21
};

// ---------------------------------------------------------------------------
// Layer index buffers
// ---------------------------------------------------------------------------
// TileMapGeneric::indices is a non-const pointer, so these cannot live in
// flash. 660 bytes each; three of them plus the collision map is 2.6 KB of RAM,
// which fits the tightest supported variant (ESP32-C3, 400 KB SRAM) with room
// to spare.

uint8_t gGroundIndices[kWorldCells];
uint8_t gFoliageIndices[kWorldCells];
uint8_t gRockIndices[kWorldCells];
bool    gSolid[kWorldCells];

bool gBuilt = false;

/// Builds a full-world tilemap over one index buffer.
SceneTileMap makeWorldMap(uint8_t* indices) {
    SceneTileMap map{};
    map.indices     = indices;
    map.width       = static_cast<uint8_t>(kWorldCols);
    map.height      = static_cast<uint8_t>(kWorldRows);
    map.tiles       = OVERWORLD_TILES;
    map.tileWidth   = static_cast<uint8_t>(kTileSize);
    map.tileHeight  = static_cast<uint8_t>(kTileSize);
    map.tileCount   = kTileCount;
    map.runtimeMask = nullptr;
    return map;
}

} // namespace

WorldLayer OVERWORLD_LAYERS[OVERWORLD_LAYER_COUNT] = {
    { {}, gfx::Color::Orange },    ///< Sand — the field everything else sits on.
    { {}, gfx::Color::Green  },    ///< Grass tufts and tree canopies.
    { {}, gfx::Color::Brown  },    ///< Mountains and the cave mouth.
};

void buildOverworld() {
    if (gBuilt) return;

    for (int row = 0; row < kWorldRows; ++row) {
        const char* line = WORLD_ROWS[row];
        for (int col = 0; col < kWorldCols; ++col) {
            const int  cell = row * kWorldCols + col;
            const char c    = line[col];

            uint8_t ground  = TILE_EMPTY;
            uint8_t foliage = TILE_EMPTY;
            uint8_t rock    = TILE_EMPTY;
            bool    solid   = false;

            switch (c) {
                case '.':
                    ground = TILE_GROUND;
                    break;
                case ',':
                    ground  = TILE_GROUND;
                    foliage = TILE_GRASS;
                    break;
                case 'T':
                    ground  = TILE_GROUND;
                    foliage = TILE_TREE;
                    solid   = true;
                    break;
                case '#':
                    rock  = TILE_ROCK;
                    solid = true;
                    break;
                case 'C':
                    rock  = TILE_CAVE;
                    solid = true;
                    break;
                default:
                    // A typo in the map — a short row, a stray character — would
                    // otherwise become invisible walkable ground. Solid rock is
                    // the loud failure: the player runs into it immediately.
                    rock  = TILE_ROCK;
                    solid = true;
                    break;
            }

            gGroundIndices[cell]  = ground;
            gFoliageIndices[cell] = foliage;
            gRockIndices[cell]    = rock;
            gSolid[cell]          = solid;
        }
    }

    OVERWORLD_LAYERS[0].map = makeWorldMap(gGroundIndices);
    OVERWORLD_LAYERS[1].map = makeWorldMap(gFoliageIndices);
    OVERWORLD_LAYERS[2].map = makeWorldMap(gRockIndices);

    gBuilt = true;
}

bool isSolidCell(int col, int row) {
    if (col < 0 || col >= kWorldCols || row < 0 || row >= kWorldRows) {
        return true;  // Outside the world is a wall.
    }
    return gSolid[row * kWorldCols + col];
}

} // namespace zelda_overworld
