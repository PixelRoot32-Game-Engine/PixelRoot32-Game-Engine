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
#else
    RUN_TEST(test_gameplay_room_layout_zero_cost_when_disabled);
#endif

    return UNITY_END();
}
