/**
 * @file test_gameplay_room_layout.cpp
 * @brief Unit tests for gameplay/RoomLayout — the Tilemap Editor room export
 *        format and the RoomGraph builder that consumes it.
 *
 * Covers the spec requirements for gameplay-room-layout:
 * - RoomData/RoomLayer are trivially copyable PODs that can live in flash
 * - buildRoomGraph converts tile-space rects to world-space camera bounds
 * - buildRoomGraph populates each room's tile window from the export
 * - connections are wired in a second pass, so forward references resolve
 * - kNoRoomConnection slots and out-of-range targets are skipped, not fatal
 * - degenerate layers (null pointer, zero count, zero tile size) build nothing
 * - a layer with more rooms than the graph's capacity truncates, not overflows
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_ROOM is
 * enabled, since RoomLayout is guarded behind that same flag (see
 * include/gameplay/RoomLayout.h). This file therefore compiles cleanly in BOTH
 * the default (flag off) and opt-in (flag on) configurations, matching the
 * flags-off / flags-on CI matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM

#include "gameplay/RoomLayout.h"
#include "gameplay/RoomGraph.h"
#include "math/Scalar.h"
#include "math/MathUtil.h"
#include "graphics/Camera2D.h"

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
#include "graphics/ProjectedMapBounds.h"
#include "math/Projection.h"
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>

using namespace pixelroot32::gameplay;
using pixelroot32::math::Scalar;
using pixelroot32::math::toScalar;
using pixelroot32::math::roundToInt;
using pixelroot32::graphics::Camera2D;

// =============================================================================
// Static property checks (compile-time verification)
// =============================================================================

// The whole point of this format is that the editor can emit it as a
// `static const` array that the linker parks in flash. A non-trivial type
// would force a runtime constructor and drag the data into SRAM.
static_assert(std::is_trivially_copyable<RoomData>::value,
              "RoomData must be trivially copyable to live in flash.");
static_assert(std::is_trivially_copyable<RoomLayer>::value,
              "RoomLayer must be trivially copyable to live in flash.");
static_assert(kNoRoomConnection == 0xFFFF,
              "kNoRoomConnection must match RoomGraph's INVALID_ROOM sentinel.");

// The editor emits these byte-for-byte, so the layout is part of the contract
// and must not drift silently. 4 rects + 4 connections, all uint16_t.
static_assert(sizeof(RoomData) == 16,
              "RoomData must stay 16 bytes — the exported format depends on it.");
static_assert(alignof(RoomData) == 2,
              "RoomData must stay 2-byte aligned — no padding between entries.");

// =============================================================================
// Fixtures — hand-written stand-ins for what the Tilemap Editor emits
// =============================================================================

/// Two 20x15 tile rooms side by side, connected left/right. Tiles are 16x16,
/// so room 0 spans world x [0, 320] and room 1 spans world x [320, 640].
static const RoomData kTwoRoomData[] = {
    // originCol, originRow, cols, rows, {Up, Down, Left, Right}
    {  0, 0, 20, 15, { kNoRoomConnection, kNoRoomConnection, kNoRoomConnection, 1 } },
    { 20, 0, 20, 15, { kNoRoomConnection, kNoRoomConnection, 0, kNoRoomConnection } },
};

static const RoomLayer kTwoRoomLayer = { kTwoRoomData, 2, 16, 16 };

// =============================================================================
// Requirement: Tile-Space Rects Become World-Space Camera Bounds
// =============================================================================

void test_build_returns_room_count(void) {
    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(2, buildRoomGraph(kTwoRoomLayer, graph));
    TEST_ASSERT_EQUAL_UINT16(2, graph.roomCount());
}

void test_build_converts_tiles_to_world_units(void) {
    RoomGraph<4> graph;
    buildRoomGraph(kTwoRoomLayer, graph);

    // Room 0: cols 0..20 at 16 px/tile -> world x [0, 320], y [0, 240].
    const Room& first = graph.getRoom(0);
    TEST_ASSERT_EQUAL_INT(0,   roundToInt(first.cameraMinX));
    TEST_ASSERT_EQUAL_INT(320, roundToInt(first.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(0,   roundToInt(first.cameraMinY));
    TEST_ASSERT_EQUAL_INT(240, roundToInt(first.cameraMaxY));

    // Room 1 starts at col 20 -> world x [320, 640].
    const Room& second = graph.getRoom(1);
    TEST_ASSERT_EQUAL_INT(320, roundToInt(second.cameraMinX));
    TEST_ASSERT_EQUAL_INT(640, roundToInt(second.cameraMaxX));
}

void test_build_honours_non_square_tiles(void) {
    static const RoomData data[] = {
        { 2, 3, 10, 4, { kNoRoomConnection, kNoRoomConnection,
                         kNoRoomConnection, kNoRoomConnection } },
    };
    static const RoomLayer layer = { data, 1, 8, 16 };  // 8 wide, 16 tall

    RoomGraph<2> graph;
    buildRoomGraph(layer, graph);

    const Room& room = graph.getRoom(0);
    TEST_ASSERT_EQUAL_INT(16,  roundToInt(room.cameraMinX));  // col 2 * 8
    TEST_ASSERT_EQUAL_INT(96,  roundToInt(room.cameraMaxX));  // (2+10) * 8
    TEST_ASSERT_EQUAL_INT(48,  roundToInt(room.cameraMinY));  // row 3 * 16
    TEST_ASSERT_EQUAL_INT(112, roundToInt(room.cameraMaxY));  // (3+4) * 16
}

// =============================================================================
// Requirement: Tile Window Comes From The Export
// =============================================================================

void test_build_sets_tile_window(void) {
    RoomGraph<4> graph;
    buildRoomGraph(kTwoRoomLayer, graph);

    const Room& second = graph.getRoom(1);
    TEST_ASSERT_TRUE(second.hasTileWindow);
    TEST_ASSERT_EQUAL_INT16(20, second.tileOriginCol);
    TEST_ASSERT_EQUAL_INT16(0,  second.tileOriginRow);
    TEST_ASSERT_EQUAL_INT16(20, second.tileCols);
    TEST_ASSERT_EQUAL_INT16(15, second.tileRows);
}

// =============================================================================
// Requirement: Connections Wire Up, Including Forward References
// =============================================================================

void test_build_wires_forward_reference(void) {
    RoomGraph<4> graph;
    buildRoomGraph(kTwoRoomLayer, graph);

    // Room 0 points Right at room 1, which did not exist yet when room 0 was
    // added — the builder must resolve connections in a second pass.
    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(1, graph.getConnections(0, out, 4));
    TEST_ASSERT_EQUAL_INT(1, out[0]);
}

void test_build_wires_back_reference(void) {
    RoomGraph<4> graph;
    buildRoomGraph(kTwoRoomLayer, graph);

    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(1, graph.getConnections(1, out, 4));
    TEST_ASSERT_EQUAL_INT(0, out[0]);
}

void test_build_skips_unconnected_slots(void) {
    static const RoomData data[] = {
        { 0, 0, 4, 4, { kNoRoomConnection, kNoRoomConnection,
                        kNoRoomConnection, kNoRoomConnection } },
    };
    static const RoomLayer layer = { data, 1, 16, 16 };

    RoomGraph<2> graph;
    buildRoomGraph(layer, graph);

    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(0, graph.getConnections(0, out, 4));
}

void test_build_skips_out_of_range_target(void) {
    // Room 0 claims a Right connection to room 7, which the layer never
    // declares. A corrupt or hand-edited export must not create a dangling
    // edge into an index the graph does not have.
    static const RoomData data[] = {
        { 0, 0, 4, 4, { kNoRoomConnection, kNoRoomConnection,
                        kNoRoomConnection, 7 } },
    };
    static const RoomLayer layer = { data, 1, 16, 16 };

    RoomGraph<2> graph;
    TEST_ASSERT_EQUAL_UINT16(1, buildRoomGraph(layer, graph));

    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(0, graph.getConnections(0, out, 4));
}

// =============================================================================
// Requirement: Degenerate Layers Build Nothing
// =============================================================================

void test_build_null_rooms_pointer_builds_nothing(void) {
    static const RoomLayer layer = { nullptr, 3, 16, 16 };

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(layer, graph));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

void test_build_zero_room_count_builds_nothing(void) {
    static const RoomLayer layer = { kTwoRoomData, 0, 16, 16 };

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(layer, graph));
}

void test_build_zero_tile_size_builds_nothing(void) {
    // A zero tile size would collapse every room to a zero-area rect and clamp
    // the camera to the origin — reject the layer instead of building garbage.
    static const RoomLayer layer = { kTwoRoomData, 2, 0, 16 };

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(layer, graph));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

void test_build_rejects_room_beyond_scalar_range(void) {
    // Fixed16 (ESP32-C3, no FPU) only reaches +/-32767. A room whose far edge
    // lands past that would wrap silently, so the whole layer is rejected —
    // partially building it would shift indices and corrupt the connections.
    static const RoomData data[] = {
        { 0, 0, 4, 4, { kNoRoomConnection, kNoRoomConnection,
                        kNoRoomConnection, kNoRoomConnection } },
        { 4000, 0, 100, 4, { kNoRoomConnection, kNoRoomConnection,
                             kNoRoomConnection, kNoRoomConnection } },
    };
    static const RoomLayer layer = { data, 2, 16, 16 };  // far edge = 65600

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(layer, graph));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

// =============================================================================
// Requirement: Capacity Truncation Is Documented, Not An Overflow
// =============================================================================

void test_build_truncates_to_graph_capacity(void) {
    RoomGraph<1> graph;  // capacity 1, layer has 2
    TEST_ASSERT_EQUAL_UINT16(1, buildRoomGraph(kTwoRoomLayer, graph));
    TEST_ASSERT_EQUAL_UINT16(1, graph.roomCount());

    // Room 0's Right edge pointed at room 1, which was truncated away — the
    // edge must be dropped rather than dangling.
    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(0, graph.getConnections(0, out, 4));
}

// =============================================================================
// Requirement: Appending Onto A Populated Graph Keeps Wiring Correct
// =============================================================================

void test_build_appends_onto_populated_graph(void) {
    RoomGraph<4> graph;
    // A room the layer knows nothing about — e.g. one the game added itself,
    // or a second layer built earlier.
    graph.addRoom(toScalar(0), toScalar(0), toScalar(64), toScalar(64));

    TEST_ASSERT_EQUAL_UINT16(2, buildRoomGraph(kTwoRoomLayer, graph));
    TEST_ASSERT_EQUAL_UINT16(3, graph.roomCount());

    int out[4] = {};

    // The pre-existing room must not gain an edge from a layer that never
    // named it. Connections are layer-local; they may not reach outside.
    TEST_ASSERT_EQUAL_UINT8(0, graph.getConnections(0, out, 4));

    // Layer room 0 landed at graph index 1, and its Right edge points at
    // layer room 1 — which landed at graph index 2, not index 1.
    TEST_ASSERT_EQUAL_UINT8(1, graph.getConnections(1, out, 4));
    TEST_ASSERT_EQUAL_INT(2, out[0]);

    TEST_ASSERT_EQUAL_UINT8(1, graph.getConnections(2, out, 4));
    TEST_ASSERT_EQUAL_INT(1, out[0]);
}

// =============================================================================
// Requirement: A Built Graph Drives The Camera
// =============================================================================

void test_entering_built_room_clamps_camera(void) {
    RoomGraph<4> graph;
    buildRoomGraph(kTwoRoomLayer, graph);

    Camera2D camera(240, 240);
    graph.enterRoom(1, &camera);

    // Room 1 spans world x [320, 640]; a position left of that clamps to 320.
    camera.setPosition({toScalar(0), toScalar(0)});
    TEST_ASSERT_EQUAL_INT(320, roundToInt(camera.getX()));
}

// =============================================================================
// Requirement: The Orthogonal Builder Is Unchanged By The Projected Overload
// =============================================================================

void test_orthogonal_overload_unchanged(void) {
    // Pins the exact rects, window and wiring the orthogonal builder produced
    // before the projected overload existed. The projected overload is
    // additive: a call without a ProjectionSpec must resolve to this builder.
    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(2, buildRoomGraph(kTwoRoomLayer, graph));

    const Room& first = graph.getRoom(0);
    TEST_ASSERT_EQUAL_INT(0,   roundToInt(first.cameraMinX));
    TEST_ASSERT_EQUAL_INT(0,   roundToInt(first.cameraMinY));
    TEST_ASSERT_EQUAL_INT(320, roundToInt(first.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(240, roundToInt(first.cameraMaxY));

    const Room& second = graph.getRoom(1);
    TEST_ASSERT_EQUAL_INT(320, roundToInt(second.cameraMinX));
    TEST_ASSERT_EQUAL_INT(0,   roundToInt(second.cameraMinY));
    TEST_ASSERT_EQUAL_INT(640, roundToInt(second.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(240, roundToInt(second.cameraMaxY));
    TEST_ASSERT_TRUE(second.hasTileWindow);
    TEST_ASSERT_EQUAL_INT16(20, second.tileOriginCol);

    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(1, graph.getConnections(0, out, 4));
    TEST_ASSERT_EQUAL_INT(1, out[0]);
}

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

using pixelroot32::graphics::ScreenBounds;
using pixelroot32::math::ProjectionSpec;
using pixelroot32::math::cellToScreenX;
using pixelroot32::math::cellToScreenY;

/// The reference isometric 2:1 basis: 32x16 cells, scene cell (0,0) anchored
/// at screen (120, 88).
static constexpr ProjectionSpec kIso{120, 88, 16, 8, -16, 8};

/// Three 7x7 rooms on a 32x16 cell stride (O5): room 0 at scene cell (0,0),
/// room 1 to its +X side at (7,0), room 2 to its +Y side at (0,7).
static const RoomData kIsoRoomData[] = {
    { 0, 0, 7, 7, { kNoRoomConnection, 2, kNoRoomConnection, 1 } },
    { 7, 0, 7, 7, { kNoRoomConnection, kNoRoomConnection, 0, kNoRoomConnection } },
    { 0, 7, 7, 7, { 0, kNoRoomConnection, kNoRoomConnection, kNoRoomConnection } },
};

static const RoomLayer kIsoRoomLayer = { kIsoRoomData, 3, 32, 16 };

// =============================================================================
// Requirement: Projected Camera Rect Is The Screen Box Of The Cell Diamonds
// =============================================================================
//
// cellToScreenX/Y gives a cell's CENTRE (where the renderer anchors a tile);
// the cell's diamond is centre +/- (16, 8). The rect is the box of the room's
// diamonds: the cell-block corners moved half a cell by -(axisX + axisY) / 2,
// which for kIso is (0, -8). X is unchanged, Y moves up 8 px. A floor tile
// that exactly fills its diamond draws this box; see
// test_tilemap_projected_draw_floor_only_room_fills_its_camera_rect.

void test_iso_build_camera_rect_is_projected_cell_block(void) {
    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(3, buildRoomGraph(kIsoRoomLayer, graph, kIso));

    // Room 0 corners: (0,0)->(120,88) (7,0)->(232,144) (0,7)->(8,144)
    // (7,7)->(120,200), corner-anchor box x [8, 232], y [88, 200]; moved by
    // (0, -8): x [8, 232], y [80, 192]. Cell (0,0)'s diamond tops out at
    // 88 - 8 = 80; cell (6,6) (centre y 184) bottoms out at 192.
    const Room& first = graph.getRoom(0);
    TEST_ASSERT_EQUAL_INT(8,   roundToInt(first.cameraMinX));
    TEST_ASSERT_EQUAL_INT(80,  roundToInt(first.cameraMinY));
    TEST_ASSERT_EQUAL_INT(232, roundToInt(first.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(192, roundToInt(first.cameraMaxY));

    // Room 1 corners: (7,0)->(232,144) (14,0)->(344,200) (7,7)->(120,200)
    // (14,7)->(232,256), box x [120, 344], y [144, 256]; moved by (0, -8):
    // x [120, 344], y [136, 248].
    const Room& second = graph.getRoom(1);
    TEST_ASSERT_EQUAL_INT(120, roundToInt(second.cameraMinX));
    TEST_ASSERT_EQUAL_INT(136, roundToInt(second.cameraMinY));
    TEST_ASSERT_EQUAL_INT(344, roundToInt(second.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(248, roundToInt(second.cameraMaxY));
}

void test_iso_build_odd_basis_rounds_half_cell_outward(void) {
    // A 15 px orthogonal basis puts the footprint edge half a pixel off the
    // grid: cell (0,0) covers [-7.5, 7.5]. The rect rounds outward, floor on
    // the minimum and ceil on the maximum, so it still contains the cell:
    // x and y [-8, 8].
    static constexpr ProjectionSpec kOdd{0, 0, 15, 0, 0, 15};
    static const RoomData data[] = {
        { 0, 0, 1, 1, { kNoRoomConnection, kNoRoomConnection,
                        kNoRoomConnection, kNoRoomConnection } },
    };
    static const RoomLayer layer = { data, 1, 15, 15 };

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(1, buildRoomGraph(layer, graph, kOdd));
    const Room& room = graph.getRoom(0);
    TEST_ASSERT_EQUAL_INT(-8, roundToInt(room.cameraMinX));
    TEST_ASSERT_EQUAL_INT(-8, roundToInt(room.cameraMinY));
    TEST_ASSERT_EQUAL_INT(8,  roundToInt(room.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(8,  roundToInt(room.cameraMaxY));
}

void test_iso_build_keeps_tile_window_and_wiring(void) {
    RoomGraph<4> graph;
    buildRoomGraph(kIsoRoomLayer, graph, kIso);

    // The tile window stays in scene cells: projection only changes the
    // camera rect, never the room's identity in the map.
    const Room& second = graph.getRoom(1);
    TEST_ASSERT_TRUE(second.hasTileWindow);
    TEST_ASSERT_EQUAL_INT16(7, second.tileOriginCol);
    TEST_ASSERT_EQUAL_INT16(0, second.tileOriginRow);
    TEST_ASSERT_EQUAL_INT16(7, second.tileCols);
    TEST_ASSERT_EQUAL_INT16(7, second.tileRows);

    int out[4] = {};
    TEST_ASSERT_EQUAL_UINT8(2, graph.getConnections(0, out, 4));
    TEST_ASSERT_EQUAL_INT(2, out[0]);  // Down
    TEST_ASSERT_EQUAL_INT(1, out[1]);  // Right
}

void test_iso_build_admits_negative_min_coordinates(void) {
    // Room 2 corners: (0,7)->(8,144) (7,7)->(120,200) (0,14)->(-104,200)
    // (7,14)->(8,256), box x [-104, 120], y [144, 256]; moved half a cell by
    // (0, -8): x [-104, 120], y [136, 248]. An isometric basis puts +Y rooms
    // left of x = 0.
    RoomGraph<4> graph;
    buildRoomGraph(kIsoRoomLayer, graph, kIso);

    const Room& third = graph.getRoom(2);
    TEST_ASSERT_EQUAL_INT(-104, roundToInt(third.cameraMinX));
    TEST_ASSERT_EQUAL_INT(136,  roundToInt(third.cameraMinY));
    TEST_ASSERT_EQUAL_INT(120,  roundToInt(third.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(248,  roundToInt(third.cameraMaxY));

    // And the negative bound survives into the camera clamp.
    Camera2D camera(240, 240);
    graph.enterRoom(2, &camera);
    camera.setPosition({toScalar(-500), toScalar(0)});
    TEST_ASSERT_EQUAL_INT(-104, roundToInt(camera.getX()));
    TEST_ASSERT_EQUAL_INT(136,  roundToInt(camera.getY()));
}

// =============================================================================
// Requirement: Per-Room ScreenBounds Widen The Projected Box
// =============================================================================

void test_iso_build_widens_by_per_room_bounds(void) {
    // Room 0's art overhangs its cells (walls rise above the diamond); room 1
    // has no measured bounds (valid == false) and keeps the bare diamond box;
    // room 2's bounds lie entirely inside its box [-104, 120] x [136, 248]
    // and change nothing.
    ScreenBounds perRoom[3];
    perRoom[0] = ScreenBounds{0, 40, 240, 210, true};
    perRoom[2] = ScreenBounds{0, 150, 100, 240, true};

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(3, buildRoomGraph(kIsoRoomLayer, graph, kIso, perRoom));

    const Room& first = graph.getRoom(0);
    TEST_ASSERT_EQUAL_INT(0,   roundToInt(first.cameraMinX));
    TEST_ASSERT_EQUAL_INT(40,  roundToInt(first.cameraMinY));
    TEST_ASSERT_EQUAL_INT(240, roundToInt(first.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(210, roundToInt(first.cameraMaxY));

    const Room& second = graph.getRoom(1);
    TEST_ASSERT_EQUAL_INT(120, roundToInt(second.cameraMinX));
    TEST_ASSERT_EQUAL_INT(136, roundToInt(second.cameraMinY));
    TEST_ASSERT_EQUAL_INT(344, roundToInt(second.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(248, roundToInt(second.cameraMaxY));

    const Room& third = graph.getRoom(2);
    TEST_ASSERT_EQUAL_INT(-104, roundToInt(third.cameraMinX));
    TEST_ASSERT_EQUAL_INT(136,  roundToInt(third.cameraMinY));
    TEST_ASSERT_EQUAL_INT(120,  roundToInt(third.cameraMaxX));
    TEST_ASSERT_EQUAL_INT(248,  roundToInt(third.cameraMaxY));
}

// =============================================================================
// Requirement: An Invalid Layer Fails Closed Under A Projection Too
// =============================================================================

void test_iso_build_rejects_degenerate_layers(void) {
    static const RoomLayer nullLayer = { nullptr, 3, 32, 16 };
    static const RoomLayer emptyLayer = { kIsoRoomData, 0, 32, 16 };
    static const RoomLayer zeroTile = { kIsoRoomData, 3, 0, 16 };

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(nullLayer, graph, kIso));
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(emptyLayer, graph, kIso));
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(zeroTile, graph, kIso));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

void test_iso_build_rejects_degenerate_projection(void) {
    // A collinear basis (det == 0) has no inverse and no area: every room
    // would collapse onto a line.
    static constexpr ProjectionSpec kFlat{0, 0, 16, 8, 32, 16};

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(kIsoRoomLayer, graph, kFlat));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

void test_iso_build_rejects_room_beyond_scalar_range(void) {
    // Room 1 sits 3000 rows down: its (0, 3007) corner projects to
    // x = 120 - 16 * 3007 = -47992 (the half-cell shift leaves x alone under
    // kIso), past -kMaxRoomWorldCoord. Room 0 is fine,
    // and is still not built — the whole layer is rejected.
    static const RoomData data[] = {
        { 0, 0, 7, 7, { kNoRoomConnection, kNoRoomConnection,
                        kNoRoomConnection, kNoRoomConnection } },
        { 0, 3000, 7, 7, { kNoRoomConnection, kNoRoomConnection,
                           kNoRoomConnection, kNoRoomConnection } },
    };
    static const RoomLayer layer = { data, 2, 32, 16 };

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(layer, graph, kIso));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

void test_iso_build_rejects_widening_beyond_scalar_range(void) {
    // The cell box is in range; the measured art bounds are not.
    ScreenBounds perRoom[3];
    perRoom[1] = ScreenBounds{0, 0, 40000, 10, true};

    RoomGraph<4> graph;
    TEST_ASSERT_EQUAL_UINT16(0, buildRoomGraph(kIsoRoomLayer, graph, kIso, perRoom));
    TEST_ASSERT_EQUAL_UINT16(0, graph.roomCount());
}

// =============================================================================
// Requirement: roomProjection Places A Room-Local Map In The Scene
// =============================================================================

void test_room_projection_maps_local_origin_to_scene_cell(void) {
    const RoomData& room = kIsoRoomData[1];  // origin cell (7, 0)
    const ProjectionSpec local = roomProjection(kIso, room);

    TEST_ASSERT_EQUAL_INT(cellToScreenX(7, 0, kIso), cellToScreenX(0, 0, local));
    TEST_ASSERT_EQUAL_INT(cellToScreenY(7, 0, kIso), cellToScreenY(0, 0, local));
    TEST_ASSERT_EQUAL_INT(232, local.originX);
    TEST_ASSERT_EQUAL_INT(144, local.originY);

    // Same basis, so every local cell lands on its scene cell.
    TEST_ASSERT_EQUAL_INT(cellToScreenX(10, 2, kIso), cellToScreenX(3, 2, local));
    TEST_ASSERT_EQUAL_INT(cellToScreenY(10, 2, kIso), cellToScreenY(3, 2, local));
    TEST_ASSERT_EQUAL_INT(kIso.axisXx, local.axisXx);
    TEST_ASSERT_EQUAL_INT(kIso.axisXy, local.axisXy);
    TEST_ASSERT_EQUAL_INT(kIso.axisYx, local.axisYx);
    TEST_ASSERT_EQUAL_INT(kIso.axisYy, local.axisYy);
}

void test_room_projection_is_constexpr(void) {
    static constexpr RoomData kRoom{0, 7, 7, 7, { kNoRoomConnection, kNoRoomConnection,
                                                  kNoRoomConnection, kNoRoomConnection }};
    static constexpr ProjectionSpec kLocal = roomProjection(kIso, kRoom);
    static_assert(kLocal.originX == 8 && kLocal.originY == 144,
                  "roomProjection must fold at compile time.");
    TEST_ASSERT_EQUAL_INT(8, kLocal.originX);
}

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_ROOM

void test_gameplay_room_layout_zero_cost_when_disabled(void) {
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_ROOM=0: RoomLayout contributes no types, "
        "no builder, and no flash or SRAM cost.");
}

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_ROOM

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
    RUN_TEST(test_build_returns_room_count);
    RUN_TEST(test_build_converts_tiles_to_world_units);
    RUN_TEST(test_build_honours_non_square_tiles);
    RUN_TEST(test_build_sets_tile_window);
    RUN_TEST(test_build_wires_forward_reference);
    RUN_TEST(test_build_wires_back_reference);
    RUN_TEST(test_build_skips_unconnected_slots);
    RUN_TEST(test_build_skips_out_of_range_target);
    RUN_TEST(test_build_null_rooms_pointer_builds_nothing);
    RUN_TEST(test_build_zero_room_count_builds_nothing);
    RUN_TEST(test_build_zero_tile_size_builds_nothing);
    RUN_TEST(test_build_rejects_room_beyond_scalar_range);
    RUN_TEST(test_build_truncates_to_graph_capacity);
    RUN_TEST(test_build_appends_onto_populated_graph);
    RUN_TEST(test_entering_built_room_clamps_camera);
    RUN_TEST(test_orthogonal_overload_unchanged);
#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    RUN_TEST(test_iso_build_camera_rect_is_projected_cell_block);
    RUN_TEST(test_iso_build_odd_basis_rounds_half_cell_outward);
    RUN_TEST(test_iso_build_keeps_tile_window_and_wiring);
    RUN_TEST(test_iso_build_admits_negative_min_coordinates);
    RUN_TEST(test_iso_build_widens_by_per_room_bounds);
    RUN_TEST(test_iso_build_rejects_degenerate_layers);
    RUN_TEST(test_iso_build_rejects_degenerate_projection);
    RUN_TEST(test_iso_build_rejects_room_beyond_scalar_range);
    RUN_TEST(test_iso_build_rejects_widening_beyond_scalar_range);
    RUN_TEST(test_room_projection_maps_local_origin_to_scene_cell);
    RUN_TEST(test_room_projection_is_constexpr);
#endif
#else
    RUN_TEST(test_gameplay_room_layout_zero_cost_when_disabled);
#endif

    return UNITY_END();
}
