/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Differential test for the iso_dungeon room's conversion from a hand-rolled
 * drawSprite loop to a projected Renderer::drawTileMap call
 * (RoomRenderer::drawTiles), and from a runtime index bake to exported data.
 *
 * The deliverable is the memcmp comparison below, not the individual
 * assertions: a frozen, verbatim copy of the pre-conversion drawTiles body
 * (the "oracle") is rendered into one framebuffer, the shipped exported map
 * is drawn through drawTileMap() into a second, and
 * the two are compared pixel-for-pixel.
 *
 * The oracle still walks the room's layout CHARS, which is what makes this
 * test worth more than it was: it now also proves that the indices resolved
 * at export time say the same thing as the chars they were resolved from.
 * Nothing in the shipped example reproduces that mapping any more. On mismatch this suite reports the
 * first differing pixel, the total differing count and the containing cell,
 * and stays red -- the example is never retuned to force a match. See the
 * proposal's divergence disposition (Engram
 * sdd/iso-dungeon-projected-conversion/proposal) for what a genuine mismatch
 * requires: a finding against the engine's projected path, not a tweak here.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

using namespace pixelroot32::graphics;

namespace {

constexpr int kFbWidth = 240;
constexpr int kFbHeight = 240;

/// Fill value no palette entry below packs to, so "left alone" and "drawn"
/// stay distinguishable in the framebuffer.
constexpr uint8_t kSentinel = 0xAAu;

uint8_t gFrameBuffer[kFbWidth * kFbHeight];

/// Owns a Renderer wired to a MockDrawSurface exposing gFrameBuffer. Same
/// idiom as test/unit/test_tilemap_projected_draw/.
struct Harness {
    std::unique_ptr<Renderer> renderer;

    Harness() {
        auto mock = std::make_unique<MockDrawSurface>();
        MockDrawSurface* mockRaw = mock.get();
        mockRaw->setSpriteBuffer(gFrameBuffer, sizeof(gFrameBuffer));

        DisplayConfig config =
            PIXELROOT32_CUSTOM_DISPLAY(mock.release(), kFbWidth, kFbHeight);
        renderer = std::make_unique<Renderer>(std::move(config));
        renderer->init();
        renderer->beginFrame();

        // Seeded AFTER beginFrame so the sentinel survives whatever clearing
        // strategy the dirty-region configuration picked for this build.
        std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
    }
};

}  // namespace

void setUp(void) {}

void tearDown(void) {
    test_teardown();
}

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

// RoomCatalog.h needs GAMEPLAY_ROOM and IsoDungeonConstants.h needs
// PROJECTION, both off in [env:native_test] -- these includes sit inside the
// guard for exactly that reason, per the shipped idiom used by the projected
// drawTileMap suite itself.
#include "math/Projection.h"
#include "graphics/StaticLayerSnapshot.h"
#include "assets/IsoDungeonRoomTileMapPalette.h"
#include "assets/DungeonTiles.h"
#include "IsoDungeonConstants.h"
#include "RoomCatalog.h"
#include "assets/IsoDungeonRoomTileMap.h"

// The export's definitions -- TILESET_SPRITES, TILESET_FOOT_Y, the ROOMn
// index arrays and init() -- live in a .cpp, because that is the shape the
// Tilemap Editor emits. This suite links only headers out of the example,
// so it pulls that one translation unit in directly rather than teaching the
// test environment to compile a file from examples/. Including it here keeps
// the test reading the SHIPPED export data: a copy would prove nothing.
#include "assets/IsoDungeonRoomTileMap.cpp"

namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;

namespace {

/// Names for the exported tile ids. The export emits bare numbers, exactly as
/// the orthogonal reference export does -- an index array is data, and naming
/// its values is the consumer's business. These mirror TILESET_SPRITES' order
/// in assets/IsoDungeonRoomTileMap.cpp, so reordering the tileset without
/// re-exporting the layers fails these assertions instead of silently
/// repainting the dungeon.
enum ExportedTileId : uint8_t {
    TILE_EMPTY = 0,
    TILE_FLOOR_A,
    TILE_FLOOR_B,
    TILE_FLOOR_ACCENT,
    TILE_WALL,
    TILE_DOOR_NE,
    TILE_DOOR_NW,
};

/// The exported map for `room`, with init() guaranteed to have run.
///
/// kRooms and ROOM_LAYERS are indexed alike by construction -- one exported
/// layer per catalog entry, in catalog order -- which is the same derivation
/// RoomRenderer::setRoom() uses.
const gfx::TileMap4bpp& exportedMapFor(const iso_dungeon::RoomSpec& room) {
    static bool initialised = false;
    if (!initialised) {
        iso_dungeon::init();
        initialised = true;
    }
    const auto index = static_cast<std::size_t>(&room - &iso_dungeon::kRooms[0]);
    return *iso_dungeon::ROOM_LAYERS[index];
}

/// AC-6 (sentinel): no layout char may bake to id 0 -- drawTileMap always
/// skips index 0, so a real cell landing on it would silently vanish.
void assertNoCellBakesToSentinel(const uint8_t* indices, int count) {
    for (int i = 0; i < count; ++i) {
        TEST_ASSERT_TRUE_MESSAGE(
            indices[i] != TILE_EMPTY,
            "a layout cell baked to the empty sentinel id (0) -- AC-6");
    }
}

/// AC-4 (anti-vacuity guard 1): a room whose layout is symmetric under
/// transpose could let an indices[x * w + y] transposition bug pass the
/// differential cases below vacuously -- the plain checkerboard alone IS
/// transpose-symmetric; only the walls, doors and accent cells break it.
bool isTransposeSymmetric(const iso_dungeon::RoomSpec& room) {
    for (int y = 0; y < iso_dungeon::kRoomTiles; ++y) {
        for (int x = 0; x < iso_dungeon::kRoomTiles; ++x) {
            if (room.layout[y][x] != room.layout[x][y]) {
                return false;
            }
        }
    }
    return true;
}

// AC-3 (frozen oracle): verbatim copy of the pre-conversion
// RoomRenderer::drawTiles body (examples/iso_dungeon/src/RoomRenderer.cpp,
// before this unit) and IsoDraw.h:28-29's drawAtCell anchoring formula.
// FROZEN ORACLE -- do NOT "improve" this to make the differential test
// below pass. Any correctness fix belongs in the shipped example or the
// engine, never here.
void oracleDrawAtCell(gfx::Renderer& renderer, const gfx::Sprite4bpp& sprite,
                       int footY, int centreX, int centreY, bool flipX = false) {
    renderer.drawSprite(sprite, centreX - sprite.width / 2, centreY - footY, 0, flipX);
}

void oracleDrawTiles(gfx::Renderer& renderer, const iso_dungeon::RoomSpec& room) {
    for (int y = 0; y < iso_dungeon::kRoomTiles; ++y) {
        for (int x = 0; x < iso_dungeon::kRoomTiles; ++x) {
            const char cell = room.layout[y][x];
            const int cx = math::cellToScreenX(x, y, iso_dungeon::kTileProjection);
            const int cy = math::cellToScreenY(x, y, iso_dungeon::kTileProjection);

            switch (cell) {
                case 'W':
                    oracleDrawAtCell(renderer, iso_dungeon::WALL_SPRITE, iso_dungeon::WALL_FOOT_Y, cx, cy);
                    break;
                case 'D':
                    oracleDrawAtCell(renderer, iso_dungeon::DOOR_NE_SPRITE, iso_dungeon::DOOR_NE_FOOT_Y, cx, cy);
                    break;
                case 'E':
                    oracleDrawAtCell(renderer, iso_dungeon::DOOR_NW_SPRITE, iso_dungeon::DOOR_NW_FOOT_Y, cx, cy);
                    break;
                default: {
                    const bool accent = (cell == 'a' || cell == 'A');
                    const gfx::Sprite4bpp& floor =
                        accent            ? iso_dungeon::FLOOR_ACCENT_SPRITE
                        : ((x + y) & 1)   ? iso_dungeon::FLOOR_B_SPRITE
                                          : iso_dungeon::FLOOR_A_SPRITE;
                    oracleDrawAtCell(renderer, floor, iso_dungeon::FLOOR_A_FOOT_Y, cx, cy);
                    break;
                }
            }
        }
    }
}

/// First differing pixel, total differing count -- enough to diagnose an
/// engine defect, which is the whole reason this suite exists (a bare
/// "buffers differ" is not).
struct FramebufferDiff {
    bool identical = true;
    int x = -1;
    int y = -1;
    uint8_t a = 0;
    uint8_t b = 0;
    long count = 0;
};

FramebufferDiff diffFramebuffers(const uint8_t* a, const uint8_t* b, int w, int h) {
    FramebufferDiff r;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int idx = y * w + x;
            if (a[idx] != b[idx]) {
                if (r.count == 0) {
                    r.x = x;
                    r.y = y;
                    r.a = a[idx];
                    r.b = b[idx];
                }
                ++r.count;
            }
        }
    }
    r.identical = (r.count == 0);
    return r;
}

/// AC-1/AC-2/AC-5: renders `room` through the frozen oracle (Frame A) and
/// through the shipped exported map + drawTileMap() path (Frame B -- the same
/// TileMap4bpp object RoomRenderer::drawTiles() hands the renderer), then
/// compares the two framebuffers pixel-for-pixel.
void runDifferentialCase(const iso_dungeon::RoomSpec& room, const char* roomName) {
    uint8_t bufferA[kFbWidth * kFbHeight];
    {
        Harness h;
        oracleDrawTiles(*h.renderer, room);
        std::memcpy(bufferA, gFrameBuffer, sizeof(bufferA));
    }

    uint8_t bufferB[kFbWidth * kFbHeight];
    {
        const gfx::TileMap4bpp& map = exportedMapFor(room);

        Harness h;
        h.renderer->drawTileMap(map, 0, 0, gfx::LayerType::Static,
                                iso_dungeon::ISO_PROJECTION);
        std::memcpy(bufferB, gFrameBuffer, sizeof(bufferB));
    }

    const FramebufferDiff diff = diffFramebuffers(bufferA, bufferB, kFbWidth, kFbHeight);
    if (!diff.identical) {
        const int cellX = math::screenToCellX(diff.x, diff.y, iso_dungeon::kTileProjection);
        const int cellY = math::screenToCellY(diff.x, diff.y, iso_dungeon::kTileProjection);
        char message[256];
        std::snprintf(message, sizeof(message),
            "%s room: projected drawTileMap diverges from the frozen oracle. "
            "First differing pixel (%d, %d): oracle=%u engine=%u. "
            "%ld differing pixels total. Containing cell: (%d, %d).",
            roomName, diff.x, diff.y,
            static_cast<unsigned>(diff.a), static_cast<unsigned>(diff.b),
            diff.count, cellX, cellY);
        TEST_FAIL_MESSAGE(message);
    }
}

}  // namespace

/// Checks the EXPORTED indices carry the expected 1-based ids for known cells
/// of the ritual room (kRooms[0]), including the two cases finding 1 of the
/// proposal calls out: 'A' (altar) and 'P' (pillar) are PropEntity sprites,
/// not tile-layer entries, so on the tile layer they resolve to plain/accent
/// floor exactly like their neighbours.
///
/// The assertions are unchanged from when a runtime bake produced these
/// values. What changed is where they come from: they are now literals in
/// ROOM0_INDICES, resolved once at export time. Reading them back against the
/// same expectations is what proves the export says what the layout meant --
/// including the checkerboard, whose rule no longer exists anywhere in the
/// shipped example and therefore cannot be re-derived to check itself.
void test_exported_indices_carry_expected_ids_for_ritual_room(void) {
    using namespace iso_dungeon;

    const uint8_t* indices = exportedMapFor(kRooms[0]).indices;

    // Row 0: "WWWDWWW" -- back wall with a door at x=3.
    TEST_ASSERT_EQUAL_UINT8(TILE_WALL, indices[0 * kRoomTiles + 0]);
    TEST_ASSERT_EQUAL_UINT8(TILE_DOOR_NE, indices[0 * kRoomTiles + 3]);

    // Row 3: "E.aAaP." -- the other back wall's door at x=0.
    TEST_ASSERT_EQUAL_UINT8(TILE_DOOR_NW, indices[3 * kRoomTiles + 0]);

    // Row 1: "W......" -- plain floor checkerboard.
    // (x=1,y=1): (x+y)&1 == 0 -> FLOOR_A. (x=2,y=1): (x+y)&1 == 1 -> FLOOR_B.
    TEST_ASSERT_EQUAL_UINT8(TILE_FLOOR_A, indices[1 * kRoomTiles + 1]);
    TEST_ASSERT_EQUAL_UINT8(TILE_FLOOR_B, indices[1 * kRoomTiles + 2]);

    // Row 2 (x=2): accent floor 'a'.
    TEST_ASSERT_EQUAL_UINT8(TILE_FLOOR_ACCENT, indices[2 * kRoomTiles + 2]);

    // Row 3 (x=3): altar 'A' -- accent floor on the tile layer; the altar
    // itself is drawn separately as a PropEntity on layer 1 (finding 1).
    TEST_ASSERT_EQUAL_UINT8(TILE_FLOOR_ACCENT, indices[3 * kRoomTiles + 3]);

    // Row 3 (x=5): pillar 'P' -- plain checkerboard floor on the tile layer;
    // the pillar itself is a PropEntity, never a tile-layer entry.
    // (x+y)&1 == (5+3)&1 == 0 -> FLOOR_A.
    TEST_ASSERT_EQUAL_UINT8(TILE_FLOOR_A, indices[3 * kRoomTiles + 5]);

    assertNoCellBakesToSentinel(indices, MAP_WIDTH * MAP_HEIGHT);
}

/// AC-4, anti-vacuity guard 1: every room the differential cases below use
/// must NOT be transpose-symmetric.
void test_anti_vacuity_guard_rooms_are_not_transpose_symmetric(void) {
    TEST_ASSERT_FALSE_MESSAGE(isTransposeSymmetric(iso_dungeon::kRooms[0]),
        "kRooms[0] (ritual) is transpose-symmetric -- an indices[x*w+y] "
        "transposition bug could pass the differential cases vacuously");
    TEST_ASSERT_FALSE_MESSAGE(isTransposeSymmetric(iso_dungeon::kRooms[1]),
        "kRooms[1] (pillar hall) is transpose-symmetric");
    TEST_ASSERT_FALSE_MESSAGE(isTransposeSymmetric(iso_dungeon::kRooms[2]),
        "kRooms[2] (shrine) is transpose-symmetric");
}

/// AC-1: memcmp(A, B) == 0 for the ritual room (kRooms[0]).
void test_projected_drawTileMap_matches_frozen_oracle_ritual_room(void) {
    runDifferentialCase(iso_dungeon::kRooms[0], "ritual");
}

/// AC-1: memcmp(A, B) == 0 for the pillar hall (kRooms[1]).
void test_projected_drawTileMap_matches_frozen_oracle_pillar_hall_room(void) {
    runDifferentialCase(iso_dungeon::kRooms[1], "pillar hall");
}

/// AC-1: memcmp(A, B) == 0 for the shrine (kRooms[2]) -- three of the three
/// shipped rooms, satisfying the "at least two" acceptance criterion with
/// margin.
void test_projected_drawTileMap_matches_frozen_oracle_shrine_room(void) {
    runDifferentialCase(iso_dungeon::kRooms[2], "shrine");
}

/// AC-6 (index skip, full-frame): a map baked entirely to the sentinel id 0
/// draws nothing at all, even though its tileset slot holds drawable data.
void test_projected_drawTileMap_index_zero_draws_nothing(void) {
    using namespace iso_dungeon;

    uint8_t indices[MAP_WIDTH * MAP_HEIGHT];
    std::memset(indices, TILE_EMPTY, sizeof(indices));

    // Deliberately assembled here rather than taken from ROOM_LAYERS: no
    // exported layer contains a sentinel cell, which is exactly what
    // assertNoCellBakesToSentinel() pins. This is the negative case that
    // proves the skip is real, so its map has to be built on purpose.
    gfx::TileMap4bpp map{};
    map.indices = indices;
    map.width = MAP_WIDTH;
    map.height = MAP_HEIGHT;
    map.tiles = TILESET_SPRITES;
    map.tileWidth = TILE_WIDTH;
    map.tileHeight = TILE_HEIGHT;
    map.tileCount = TILESET_TILE_COUNT;
    map.tileFootY = TILESET_FOOT_Y;

    Harness h;
    h.renderer->drawTileMap(map, 0, 0, gfx::LayerType::Static, ISO_PROJECTION);

    uint8_t allSentinel[kFbWidth * kFbHeight];
    std::memset(allSentinel, kSentinel, sizeof(allSentinel));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(allSentinel, gFrameBuffer, kFbWidth * kFbHeight);
}

/// AC-8: the same room, drawn through StaticLayerSnapshot's own
/// restore()/capture() cycle -- the exact sequence RoomRenderer::draw() runs
/// -- renders identically on frame 2 (restore path, or the documented
/// allocation-failure redraw fallback) as on frame 1 (miss + capture).
///
/// Deliberately does not go through the RoomRenderer class itself: this
/// suite links the example's headers plus the one exported translation unit,
/// the same reachability boundary every other case here relies on.
/// RoomRenderer.cpp is exercised by the example's own build
/// (examples/iso_dungeon), not by this native test binary.
void test_projected_drawTileMap_restores_identically_via_static_layer_snapshot(void) {
    using namespace iso_dungeon;

    const gfx::TileMap4bpp& map = exportedMapFor(kRooms[0]);

    auto mock = std::make_unique<MockDrawSurface>();
    mock->setSpriteBuffer(gFrameBuffer, sizeof(gFrameBuffer));
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), kFbWidth, kFbHeight);
    gfx::Renderer renderer(std::move(config));
    renderer.init();

    gfx::StaticLayerSnapshot snapshot;
    (void)snapshot.allocateForRenderer(renderer);

    // Frame 1: a normal per-frame clear, then a cache miss -- draw + capture,
    // exactly what RoomRenderer::draw() runs the first time a room is drawn.
    renderer.beginFrame();
    TEST_ASSERT_FALSE(snapshot.restore(renderer));
    renderer.drawTileMap(map, 0, 0, gfx::LayerType::Static, ISO_PROJECTION);
    (void)snapshot.capture(renderer);

    uint8_t frame1[kFbWidth * kFbHeight];
    std::memcpy(frame1, gFrameBuffer, sizeof(frame1));

    // Frame 2: another per-frame clear, then either a cache hit (restore) or,
    // if allocation failed, the same deterministic redraw -- both are a
    // legitimate outcome per StaticLayerSnapshot's own documented fallback,
    // and both must reproduce frame 1 exactly.
    renderer.beginFrame();
    if (!snapshot.restore(renderer)) {
        renderer.drawTileMap(map, 0, 0, gfx::LayerType::Static, ISO_PROJECTION);
    }

    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame1, gFrameBuffer, kFbWidth * kFbHeight);
}

void test_iso_dungeon_projected_conversion_zero_cost_when_disabled(void) {
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_TILEMAP_PROJECTION=1: the projected conversion "
        "suite is compiled; functional coverage is above.");
}

#else

void test_iso_dungeon_projected_conversion_zero_cost_when_disabled(void) {
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_TILEMAP_PROJECTION=0: the projected conversion "
        "suite is not compiled.");
}

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    RUN_TEST(test_exported_indices_carry_expected_ids_for_ritual_room);
    RUN_TEST(test_anti_vacuity_guard_rooms_are_not_transpose_symmetric);
    RUN_TEST(test_projected_drawTileMap_matches_frozen_oracle_ritual_room);
    RUN_TEST(test_projected_drawTileMap_matches_frozen_oracle_pillar_hall_room);
    RUN_TEST(test_projected_drawTileMap_matches_frozen_oracle_shrine_room);
    RUN_TEST(test_projected_drawTileMap_index_zero_draws_nothing);
    RUN_TEST(test_projected_drawTileMap_restores_identically_via_static_layer_snapshot);
#endif
    RUN_TEST(test_iso_dungeon_projected_conversion_zero_cost_when_disabled);

    return UNITY_END();
}
