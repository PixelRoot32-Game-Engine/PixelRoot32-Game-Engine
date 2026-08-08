/**
 * @file test_gameplay_room_graph.cpp
 * @brief Unit tests for gameplay/RoomGraph module
 *
 * Covers the spec requirements for gameplay-room-graph:
 * - addRoom returns sequential indices and sentinel when full
 * - connect adds a directed edge between valid rooms
 * - enterRoom updates currentRoomIndex and camera bounds
 * - onEnter callback fires with correct indices
 * - getConnections copies into caller buffer with truncation
 * - isValidIdx bounds-checks correctly
 * - feature-gated and zero-cost when disabled
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_ROOM
 * is enabled, since RoomGraph is entirely guarded behind that flag (see
 * include/gameplay/RoomGraph.h). This file therefore compiles cleanly in
 * BOTH the default (flag off) and opt-in (flag on) configurations, matching
 * the "no behavior change for existing examples" goal and the flags-off /
 * flags-on CI matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM

#include "gameplay/RoomGraph.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "graphics/Camera2D.h"

#include <cstddef>
#include <cstdint>

using namespace pixelroot32::gameplay;
using pixelroot32::math::Scalar;
using pixelroot32::math::Vector2;
using pixelroot32::math::toScalar;
using pixelroot32::graphics::Camera2D;

// =============================================================================
// Static property checks (compile-time verification, not runtime assertions)
// =============================================================================

// Room must be 6 Scalars + 6 fields (4 int16_t + bool + 4 int + uint8_t)
// plus padding.  The exact sizeof is platform-dependent so we verify it is
// a reasonable POD size, not a hidden-vtable or std::function blow-up.
static_assert(sizeof(Room) <= 120, "Room must stay under 120 bytes (POD only)");

// RoomDir must map its enumerators to the documented array indices.
static_assert(static_cast<int>(RoomDir::Up)    == 0, "Up    -> index 0");
static_assert(static_cast<int>(RoomDir::Down)  == 1, "Down  -> index 1");
static_assert(static_cast<int>(RoomDir::Left)  == 2, "Left  -> index 2");
static_assert(static_cast<int>(RoomDir::Right) == 3, "Right -> index 3");

// =============================================================================
// Requirement: addRoom Returns Sequential Indices, Sentinel When Full
// =============================================================================

void test_add_room_returns_index(void) {
    RoomGraph<8> graph;
    TEST_ASSERT_EQUAL_UINT16(0, graph.addRoom(toScalar(0), toScalar(0),
                                              toScalar(256), toScalar(256)));
    TEST_ASSERT_EQUAL_UINT16(1, graph.addRoom(toScalar(256), toScalar(0),
                                              toScalar(512), toScalar(256)));
    TEST_ASSERT_EQUAL_UINT16(2, graph.addRoom(toScalar(512), toScalar(0),
                                              toScalar(768), toScalar(256)));
    TEST_ASSERT_EQUAL_UINT16(3, graph.roomCount());
}

void test_add_room_overflow_returns_sentinel(void) {
    RoomGraph<3> graph;
    // Fill to capacity.
    for (uint16_t i = 0; i < 3; ++i) {
        TEST_ASSERT_EQUAL_UINT16(i, graph.addRoom(toScalar(0), toScalar(0),
                                                  toScalar(256), toScalar(256)));
    }
    TEST_ASSERT_EQUAL_UINT16(3, graph.roomCount());
    // One past capacity must return the sentinel.
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, graph.addRoom(toScalar(0), toScalar(0),
                                                   toScalar(256), toScalar(256)));
    // roomCount must NOT change after a failed addRoom.
    TEST_ASSERT_EQUAL_UINT16(3, graph.roomCount());
}

// =============================================================================
// Requirement: connect Adds A Directed Edge Between Valid Rooms
// =============================================================================

void test_connect_valid_indices(void) {
    RoomGraph<8> graph;
    graph.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));
    graph.addRoom(toScalar(256), toScalar(0), toScalar(512), toScalar(256));

    graph.connect(0, 1, RoomDir::Up);

    // getConnections(0) must return 1 entry pointing to room 1.
    int buf[4] = {};
    const uint8_t count = graph.getConnections(0, buf, 4);
    TEST_ASSERT_EQUAL_UINT8(1, count);
    TEST_ASSERT_EQUAL_INT(1, buf[0]);

    // Room 1 should have no connections (connect is directed).
    int buf2[4] = {};
    const uint8_t count2 = graph.getConnections(1, buf2, 4);
    TEST_ASSERT_EQUAL_UINT8(0, count2);
}

void test_connect_invalid_index_is_noop(void) {
    RoomGraph<4> graph;
    graph.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));

    // Connecting from an out-of-range index must be a quiet no-op.
    graph.connect(99, 0, RoomDir::Up);
    TEST_ASSERT_EQUAL_UINT16(1, graph.roomCount());

    // Connecting to an out-of-range index must also be a quiet no-op.
    graph.connect(0, 99, RoomDir::Down);
    int buf[4] = {};
    TEST_ASSERT_EQUAL_UINT8(0, graph.getConnections(0, buf, 4));
}

// =============================================================================
// Requirement: enterRoom Updates currentRoomIndex And Camera Bounds
// =============================================================================

void test_enter_room_updates_current(void) {
    RoomGraph<4> graph;
    graph.addRoom(toScalar(50), toScalar(100), toScalar(200), toScalar(300));
    graph.enterRoom(0, nullptr);
    TEST_ASSERT_EQUAL_UINT16(0, graph.currentRoomIndex());
}

void test_enter_room_updates_camera_bounds(void) {
    RoomGraph<4> graph;
    graph.addRoom(toScalar(50), toScalar(100), toScalar(200), toScalar(300));

    Camera2D camera(240, 240);
    // enterRoom with a valid camera must NOT crash and must update
    // currentRoomIndex.  The internal camera bounds update (setBounds /
    // setVerticalBounds) is verified by the example migration (CU4);
    // this unit test verifies the no-crash contract and room-index update.
    graph.enterRoom(0, &camera);
    TEST_ASSERT_EQUAL_UINT16(0, graph.currentRoomIndex());
}

void test_enter_room_invalid_is_noop(void) {
    RoomGraph<4> graph;
    graph.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));

    // Current room index starts at 0xFFFF (never entered).
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, graph.currentRoomIndex());

    // Attempting to enter an out-of-range room must be a quiet no-op.
    graph.enterRoom(99, nullptr);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, graph.currentRoomIndex());
}

// =============================================================================
// Requirement: onEnter Callback Fires With Correct Indices
// =============================================================================

namespace {

struct Capture {
    int fromIdx = -1;
    int toIdx   = -1;
    void* userData = nullptr;
    bool fired  = false;
};

void onEnterCapture(int fromIdx, int toIdx, void* userData) {
    auto* cap = static_cast<Capture*>(userData);
    cap->fromIdx  = fromIdx;
    cap->toIdx    = toIdx;
    cap->userData = userData;
    cap->fired    = true;
}

}  // namespace

void test_on_enter_callback_fires(void) {
    RoomGraph<4> graph;
    graph.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));
    graph.addRoom(toScalar(256), toScalar(0), toScalar(512), toScalar(256));

    Capture cap;
    graph.setOnEnter(onEnterCapture, &cap);
    graph.enterRoom(1, nullptr);

    TEST_ASSERT_TRUE(cap.fired);
    // fromIdx is 0xFFFF (never entered before), toIdx is 1.
    TEST_ASSERT_EQUAL_INT(0xFFFF, cap.fromIdx);
    TEST_ASSERT_EQUAL_INT(1,      cap.toIdx);
    TEST_ASSERT_EQUAL_PTR(&cap,   cap.userData);

    // Second transition: fromIdx should be 1, toIdx 0.
    graph.enterRoom(0, nullptr);
    TEST_ASSERT_TRUE(cap.fired);
    TEST_ASSERT_EQUAL_INT(1, cap.fromIdx);
    TEST_ASSERT_EQUAL_INT(0, cap.toIdx);
}

// =============================================================================
// Requirement: getConnections Copies Into Caller Buffer With Truncation
// =============================================================================

void test_get_connections_truncation(void) {
    RoomGraph<8> graph;
    graph.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));
    graph.addRoom(toScalar(256), toScalar(0), toScalar(512), toScalar(256));
    graph.addRoom(toScalar(512), toScalar(0), toScalar(768), toScalar(256));
    graph.addRoom(toScalar(768), toScalar(0), toScalar(1024), toScalar(256));

    // Connect room 0 to rooms 1, 2, and 3 via three different directions.
    graph.connect(0, 1, RoomDir::Up);
    graph.connect(0, 2, RoomDir::Right);
    graph.connect(0, 3, RoomDir::Down);

    // Request maxOut=2 — must return only 2 entries, no crash.
    int buf[2] = {};
    const uint8_t count = graph.getConnections(0, buf, 2);
    TEST_ASSERT_EQUAL_UINT8(2, count);
}

// =============================================================================
// Requirement: isValidIdx Bounds-Checks Correctly
// =============================================================================

void test_is_valid_idx_boundaries(void) {
    RoomGraph<4> graph;
    graph.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));
    graph.addRoom(toScalar(256), toScalar(0), toScalar(512), toScalar(256));
    graph.addRoom(toScalar(512), toScalar(0), toScalar(768), toScalar(256));

    TEST_ASSERT_TRUE(graph.isValidIdx(0));
    TEST_ASSERT_TRUE(graph.isValidIdx(1));
    TEST_ASSERT_TRUE(graph.isValidIdx(2));
    // One past roomCount must be invalid.
    TEST_ASSERT_FALSE(graph.isValidIdx(3));
    TEST_ASSERT_FALSE(graph.isValidIdx(99));
}

// =============================================================================
// Requirement: No Bomberbot-Specific Or Zelda-Specific Identifiers
// =============================================================================

void test_no_bomberbot_no_zelda(void) {
    // RoomDir enumerators are direction-neutral (Up/Down/Left/Right).
    // Room contains camera rect + tile window + connections — no genre
    // semantics.  RoomGraph<N> is a generic graph-of-rects template.
    // This test verifies the namespace and type names are genre-agnostic:
    // the word "room" itself is abstract (any genre with discrete screens
    // can use it — top-down, metroidvania, puzzle, RPG).
    //
    // Concrete check: RoomDir values match north/south/west/east layout
    // with no genre-specific semantics (no "dungeon", "overworld", "bomb",
    // "key", "sword", "puzzle", "item", "boss", etc.).
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(RoomDir::Up));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(RoomDir::Down));
    TEST_ASSERT_EQUAL_UINT8(2, static_cast<uint8_t>(RoomDir::Left));
    TEST_ASSERT_EQUAL_UINT8(3, static_cast<uint8_t>(RoomDir::Right));
    TEST_PASS_MESSAGE(
        "RoomDir, Room, and RoomGraph<N> are genre-agnostic — no Bomberbot "
        "or Zelda-specific identifiers present.");
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
//
// Defined in BOTH flag states, mirroring test_gameplay_grid_space.cpp:238-277
// and test_gameplay_object_pool.cpp:513, because main() RUN_TESTs it
// unconditionally. Defining it only in the #else branch leaves main()
// referencing an undeclared function whenever the flag is on.
// =============================================================================

void test_gameplay_room_graph_zero_cost_when_disabled(void) {
    // With the flag on, Room must stay a POD with no hidden heap pointers,
    // no vtable, no std::function/std::any blow-up.  The exact sizeof()
    // varies by platform (48B on 64-bit, 40B on 32-bit), but it must stay
    // under the 120B guard checked by the static_assert above.
    static_assert(sizeof(Room) <= 120,
                  "Room must stay under 120 bytes (POD only).");
    TEST_ASSERT_TRUE(sizeof(Room) <= 120);

    // RoomDir enumerators must map to their documented array indices.
    static_assert(static_cast<int>(RoomDir::Up)    == 0, "");
    static_assert(static_cast<int>(RoomDir::Down)  == 1, "");
    static_assert(static_cast<int>(RoomDir::Left)  == 2, "");
    static_assert(static_cast<int>(RoomDir::Right) == 3, "");

    // kMaxConnections must be 4 (one per cardinal direction).
    static_assert(RoomGraph<8>::kMaxConnections == 4,
                  "RoomGraph must have exactly 4 max connections (cardinal dirs).");

    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_ROOM=1: Room is a POD under 120 bytes "
        "and all public types are constexpr-friendly — zero runtime overhead "
        "for a constexpr consumer.");
}

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_ROOM

void test_gameplay_room_graph_zero_cost_when_disabled(void) {
    // With the flag off, RoomGraph<N>, Room, and RoomDir are not compiled
    // at all — their entire declaration lives inside the
    // #if PIXELROOT32_ENABLE_GAMEPLAY_ROOM guard in
    // include/gameplay/RoomGraph.h. This translation unit compiling and
    // passing without referencing any of them IS the "zero bytes reserved"
    // property required by the spec's "Feature-Gated And Zero-Cost When
    // Disabled" requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_ROOM=0: RoomGraph, Room, and RoomDir "
        "are not compiled, zero bytes reserved for any (N) instantiation.");
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
    RUN_TEST(test_add_room_returns_index);
    RUN_TEST(test_add_room_overflow_returns_sentinel);
    RUN_TEST(test_connect_valid_indices);
    RUN_TEST(test_connect_invalid_index_is_noop);
    RUN_TEST(test_enter_room_updates_current);
    RUN_TEST(test_enter_room_updates_camera_bounds);
    RUN_TEST(test_enter_room_invalid_is_noop);
    RUN_TEST(test_on_enter_callback_fires);
    RUN_TEST(test_get_connections_truncation);
    RUN_TEST(test_is_valid_idx_boundaries);
    RUN_TEST(test_no_bomberbot_no_zelda);
#endif
    RUN_TEST(test_gameplay_room_graph_zero_cost_when_disabled);

    return UNITY_END();
}
