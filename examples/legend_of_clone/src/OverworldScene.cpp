#include "OverworldScene.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Engine.h"
#include "graphics/Color.h"

#include "Scenes.h"
#include "assets/OverworldTileMap.h"
#include "assets/OverworldRooms.h"
#include "assets/PlayerPalette.h"
#include "assets/TilemapPalette.h"

#include <cstdio>

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace legend_of_clone {

namespace gfx = pr32::graphics;

// The exported map and the gameplay constants describe the same grid from two
// directions. If they ever disagree the player walks off the edge of a map that
// is a different size than the room table says — so make them fail at compile
// time instead.
static_assert(overworld::MAP_WIDTH == kWorldCols, "exported map is not kWorldCols wide");
static_assert(overworld::MAP_HEIGHT == kWorldRows, "exported map is not kWorldRows tall");
static_assert(overworld::TILE_SIZE == kTileSize, "exported tiles are not kTileSize");

TopDownScene::Setup OverworldScene::setup() {
    // Wires the exported descriptors to their data. Pointers only — no pixels
    // are copied and nothing is unpacked, so calling it on every scene swap
    // costs a handful of stores.
    overworld::init();
    world_.attach(&overworld::terrain, overworld::TILE_SOLID, overworld::TILE_COUNT);

    Setup config;
    config.backgroundPalette = TILEMAP_PALETTE_DATA;
    config.spritePalette     = PLAYER_SPRITE_PALETTE_RGB565;
    config.roomLayer         = &OVERWORLD_ROOM_LAYER;
    config.world             = &world_;
    config.startRoom         = kRoomSouthWest;
    config.startCol          = spawnCol_;
    config.startRow          = spawnRow_;
    config.startFacing       = spawnFacing_;
    return config;
}

void OverworldScene::spawnAtCaveMouth() {
    spawnCol_    = kCaveExitCol;
    spawnRow_    = kCaveExitRow;
    // Facing away from the cave, so the player is not walking back into it on
    // the frame they arrive.
    spawnFacing_ = Facing::Down;
}

void OverworldScene::onPlayerSettled() {
    int col = 0;
    int row = 0;
    playerCell(col, row);

    if (world().tileAt(col, row) != overworld::TILE_CAVE) return;

    // Coming back out will need to put the player below the mouth rather than
    // at the start of the game.
    spawnAtCaveMouth();

    // A fade rather than an instant swap: the engine blanks the framebuffer,
    // swaps scenes on the dark frame, and fades the dungeon in. The swap runs
    // init() on the target, which is where the dungeon attaches its map and
    // places the player.
    engine.triggerTransition(&dungeonScene,
                             gfx::TransitionType::Fade,
                             kDoorwayFadeMs);
}

void OverworldScene::drawStatusBar(gfx::Renderer& renderer) {
    TopDownScene::drawStatusBar(renderer);

    const bool oldBypass = renderer.isOffsetBypassEnabled();
    renderer.setOffsetBypass(true);

    // Placeholder readout. The heart row and item slots belong here, and this is
    // where UISpriteRow lands once the player has something to lose.
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "OVERWORLD %u",
                  static_cast<unsigned>(currentRoom()));
    renderer.drawText(buffer, 8, kStatusBarY + 12, gfx::Color::White, 1);

    if (lastFromRoom() >= 0) {
        std::snprintf(buffer, sizeof(buffer), "FROM %d", lastFromRoom());
        renderer.drawText(buffer, 8, kStatusBarY + 30, gfx::Color::Gray, 1);
    }

    renderer.setOffsetBypass(oldBypass);
}

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
