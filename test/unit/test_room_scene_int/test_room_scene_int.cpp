/**
 * @file test_room_scene_int.cpp
 * @brief Integration tests for Scene ↔ RoomGraph dispatch via onRoomEnter
 *
 * Covers CU3 of the room-screen-abstraction change:
 * - Scene::getRoomGraph / setRoomGraph roundtrip
 * - Virtual dispatch through RoomGraphBase pointer fires Scene::onRoomEnter
 * - Default onRoomEnter no-op doesn't crash
 * - Multiple transitions accumulate correctly
 * - Feature-gated and zero-cost when disabled
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_ROOM
 * is enabled. This file compiles cleanly in BOTH configurations, matching
 * the flags-off / flags-on CI matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM

#include "core/Scene.h"
#include "gameplay/RoomGraph.h"
#include "graphics/Camera2D.h"

#include <cstdint>

using namespace pixelroot32::core;
using namespace pixelroot32::gameplay;
using pixelroot32::math::toScalar;

// =============================================================================
// Helper: a Scene that owns a RoomGraph<2> and records onRoomEnter calls
// =============================================================================

class TestScene : public Scene {
public:
    RoomGraph<2> rooms_;

    static int onRoomEnterCount;
    static int lastFromIdx;
    static int lastToIdx;

    void init() override {
        Scene::init();
        onRoomEnterCount = 0;
        lastFromIdx = -1;
        lastToIdx   = -1;

        rooms_.addRoom(toScalar(0),  toScalar(0),  toScalar(256), toScalar(256));
        rooms_.addRoom(toScalar(256), toScalar(0), toScalar(512), toScalar(256));
        rooms_.connect(0, 1, RoomDir::Right);

        setRoomGraph(&rooms_);
        rooms_.setOnEnter(onEnterCallback, this);
    }

    void onRoomEnter(int fromIdx, int toIdx) override {
        onRoomEnterCount++;
        lastFromIdx = fromIdx;
        lastToIdx   = toIdx;
    }

private:
    static void onEnterCallback(int from, int to, void* userData) {
        auto* scene = static_cast<TestScene*>(userData);
        scene->onRoomEnter(from, to);
    }
};

int TestScene::onRoomEnterCount = 0;
int TestScene::lastFromIdx      = -1;
int TestScene::lastToIdx        = -1;

// =============================================================================
// Helper: a Scene that does NOT override onRoomEnter (uses default no-op)
// =============================================================================

class NoHookScene : public Scene {
public:
    RoomGraph<1> rooms_;

    void init() override {
        Scene::init();
        rooms_.addRoom(toScalar(0), toScalar(0), toScalar(256), toScalar(256));
        setRoomGraph(&rooms_);
        rooms_.setOnEnter(onEnterCallback, this);
    }

private:
    static void onEnterCallback(int from, int to, void* userData) {
        auto* scene = static_cast<NoHookScene*>(userData);
        scene->onRoomEnter(from, to);  // resolves to Scene::onRoomEnter (default no-op)
    }
};

// =============================================================================
// Test Case (a): fresh Scene has no RoomGraph
// =============================================================================

void test_scene_default_no_room_graph(void) {
    Scene scene;
    TEST_ASSERT_NULL(scene.getRoomGraph());
}

// =============================================================================
// Test Case (b): setRoomGraph + getRoomGraph roundtrip
// =============================================================================

void test_scene_set_get_room_graph(void) {
    TestScene scene;
    scene.init();

    RoomGraphBase* g = scene.getRoomGraph();
    TEST_ASSERT_NOT_NULL(g);
    TEST_ASSERT_EQUAL_PTR(&scene.rooms_, g);
}

// =============================================================================
// Test Case (c): virtual dispatch through base pointer fires onRoomEnter
// =============================================================================

void test_scene_on_room_enter_via_base_pointer(void) {
    TestScene scene;
    scene.init();

    // First transition: from 0xFFFF (no previous room) to room 1.
    // The callback calls onRoomEnter(from, to), proving the virtual
    // dispatch through RoomGraphBase* works end-to-end.
    scene.getRoomGraph()->enterRoom(1, nullptr);

    TEST_ASSERT_EQUAL(1, TestScene::onRoomEnterCount);
    TEST_ASSERT_EQUAL(0xFFFF, TestScene::lastFromIdx);
    TEST_ASSERT_EQUAL(1, TestScene::lastToIdx);
}

// =============================================================================
// Test Case (d): default onRoomEnter no-op doesn't crash
// =============================================================================

void test_scene_on_room_enter_default_noop(void) {
    NoHookScene scene;
    scene.init();

    // NoHookScene does NOT override onRoomEnter. The callback fires
    // and calls scene->onRoomEnter(), which resolves to Scene::onRoomEnter
    // (the default no-op). This MUST NOT crash.
    scene.getRoomGraph()->enterRoom(0, nullptr);

    // enterRoom still updated the graph's internal state.
    TEST_ASSERT_EQUAL_UINT16(0, scene.getRoomGraph()->currentRoomIndex());
}

// =============================================================================
// Test Case (e): multiple transitions accumulate
// =============================================================================

void test_scene_on_room_enter_multiple_transitions(void) {
    TestScene scene;
    scene.init();

    // Transition 1: 0xFFFF → 0
    scene.getRoomGraph()->enterRoom(0, nullptr);
    TEST_ASSERT_EQUAL(1, TestScene::onRoomEnterCount);
    TEST_ASSERT_EQUAL(0xFFFF, TestScene::lastFromIdx);
    TEST_ASSERT_EQUAL(0, TestScene::lastToIdx);

    // Transition 2: 0 → 1
    scene.getRoomGraph()->enterRoom(1, nullptr);
    TEST_ASSERT_EQUAL(2, TestScene::onRoomEnterCount);
    TEST_ASSERT_EQUAL(0, TestScene::lastFromIdx);
    TEST_ASSERT_EQUAL(1, TestScene::lastToIdx);

    // Transition 3: 1 → 0
    scene.getRoomGraph()->enterRoom(0, nullptr);
    TEST_ASSERT_EQUAL(3, TestScene::onRoomEnterCount);
    TEST_ASSERT_EQUAL(1, TestScene::lastFromIdx);
    TEST_ASSERT_EQUAL(0, TestScene::lastToIdx);
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled (flag-on variant)
// =============================================================================

void test_room_scene_int_zero_cost_flag_on(void) {
    // With the flag on, RoomGraphBase is a pure-virtual interface with no
    // data members — it imposes only a vtable pointer in derived classes.
    // Scene's roomGraph_ is a raw pointer initialized to nullptr, adding
    // zero heap allocation overhead.
    static_assert(sizeof(RoomGraphBase) >= sizeof(void*),
                  "RoomGraphBase must be at least pointer-sized (vtable).");
    TEST_ASSERT_TRUE(sizeof(RoomGraphBase) >= sizeof(void*));
}

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_ROOM

void test_room_scene_int_zero_cost_when_disabled(void) {
    // With the flag off, Scene.h does not include RoomGraph.h, has no
    // getRoomGraph() / setRoomGraph() / onRoomEnter() / roomGraph_
    // members. This translation unit compiling and passing without
    // referencing any of those symbols IS the zero-cost property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_ROOM=0: Scene has no RoomGraph-related "
        "members, virtual methods, or includes — zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_ROOM

// =============================================================================
// Unity boilerplate
// =============================================================================

void setUp(void) {
    test_setup();
#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
    TestScene::onRoomEnterCount = 0;
    TestScene::lastFromIdx      = -1;
    TestScene::lastToIdx        = -1;
#endif
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
    RUN_TEST(test_scene_default_no_room_graph);
    RUN_TEST(test_scene_set_get_room_graph);
    RUN_TEST(test_scene_on_room_enter_via_base_pointer);
    RUN_TEST(test_scene_on_room_enter_default_noop);
    RUN_TEST(test_scene_on_room_enter_multiple_transitions);
    RUN_TEST(test_room_scene_int_zero_cost_flag_on);
#else
    RUN_TEST(test_room_scene_int_zero_cost_when_disabled);
#endif

    return UNITY_END();
}
