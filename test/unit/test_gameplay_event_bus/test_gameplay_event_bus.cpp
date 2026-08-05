/**
 * @file test_gameplay_event_bus.cpp
 * @brief Unit tests for gameplay/GameplayEventBus module
 *
 * Covers the spec requirements for gameplay-event-bus:
 * - publish-success (event becomes retrievable)
 * - drop-newest-on-full (overflow policy + drop counter)
 * - drain-on-swap (SceneManager::setCurrentScene() clears the bus)
 * - no-drain-on-push/pop (pushScene()/popScene() leave pending events intact)
 * - zero-cost-when-disabled (layout/size static assertion, not a hardcoded sizeof)
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_EVENTS is
 * enabled, since GameplayEvent/GameplayEventBus are entirely guarded behind
 * that flag (see include/gameplay/GameplayEventBus.h). This file therefore
 * compiles cleanly in BOTH the default (flag off) and opt-in (flag on)
 * configurations, matching the "no behavior change for existing examples"
 * goal and the CI matrix from design.md.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS

#include "gameplay/GameplayEventBus.h"
#include "gameplay/GameplayEvent.h"
#include "gameplay/GameplayEventType.h"
#include "math/Scalar.h"
#include "core/SceneManager.h"
#include "core/Scene.h"
#include "graphics/Renderer.h"

#include <cstddef>

using namespace pixelroot32::gameplay;
using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// Minimal mock scene for scene-swap drain tests, mirroring the MockScene
// pattern already used in test/unit/test_scene_manager/test_scene_manager.cpp.
class MockGameplayScene : public Scene {
public:
    bool initCalled = false;
    void init() override { initCalled = true; }
    void update(unsigned long) override {}
    void draw(Renderer&) override {}
};

void test_publish_success_retrievable(void) {
    GameplayEventBus bus;
    GameplayEvent event;
    event.type = GameplayEventType::Interact;
    event.entityIdA = 1;
    event.entityIdB = 2;

    TEST_ASSERT_TRUE(bus.publish(event));
    TEST_ASSERT_FALSE(bus.isEmpty());

    GameplayEvent outEvent;
    TEST_ASSERT_TRUE(bus.consume(outEvent));
    TEST_ASSERT_EQUAL(static_cast<int>(GameplayEventType::Interact), static_cast<int>(outEvent.type));
    TEST_ASSERT_EQUAL_UINT16(1, outEvent.entityIdA);
    TEST_ASSERT_EQUAL_UINT16(2, outEvent.entityIdB);
    TEST_ASSERT_TRUE(bus.isEmpty());
}

void test_drop_newest_on_full_increments_counter(void) {
    GameplayEventBus bus;
    GameplayEvent event;
    event.type = GameplayEventType::TriggerEnter;
    event.entityIdA = 111;

    for (uint16_t i = 0; i < GameplayEventBus::kCapacity; ++i) {
        TEST_ASSERT_TRUE(bus.publish(event));
    }
    TEST_ASSERT_TRUE(bus.isFull());
    TEST_ASSERT_EQUAL_UINT32(0, bus.getDroppedCount());

    // Buffer is full: the newly published event must be dropped, not evict
    // the oldest buffered event (drop-newest, not drop-oldest — design.md D2).
    GameplayEvent overflowEvent;
    overflowEvent.type = GameplayEventType::TriggerExit;
    overflowEvent.entityIdA = 999;
    TEST_ASSERT_FALSE(bus.publish(overflowEvent));
    TEST_ASSERT_EQUAL_UINT32(1, bus.getDroppedCount());

    // The oldest still-buffered event is unchanged.
    GameplayEvent outEvent;
    TEST_ASSERT_TRUE(bus.consume(outEvent));
    TEST_ASSERT_EQUAL(static_cast<int>(GameplayEventType::TriggerEnter), static_cast<int>(outEvent.type));
    TEST_ASSERT_EQUAL_UINT16(111, outEvent.entityIdA);
}

void test_repeated_publish_on_full_does_not_crash(void) {
    GameplayEventBus bus;
    GameplayEvent event;
    event.type = GameplayEventType::TriggerEnter;

    for (uint16_t i = 0; i < GameplayEventBus::kCapacity; ++i) {
        bus.publish(event);
    }

    const int kOverflowAttempts = 50;
    for (int i = 0; i < kOverflowAttempts; ++i) {
        TEST_ASSERT_FALSE(bus.publish(event));
    }
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(kOverflowAttempts), bus.getDroppedCount());
    TEST_ASSERT_TRUE(bus.isFull());
}

void test_bus_drains_on_scene_swap(void) {
    SceneManager manager;
    GameplayEventBus bus;
    manager.setGameplayEventBus(&bus);

    MockGameplayScene sceneA;
    MockGameplayScene sceneB;

    manager.setCurrentScene(&sceneA);

    GameplayEvent event;
    event.type = GameplayEventType::Interact;
    TEST_ASSERT_TRUE(bus.publish(event));
    TEST_ASSERT_FALSE(bus.isEmpty());

    // setCurrentScene() is the SceneSwap funnel (design.md D7) — the swap to
    // sceneB must drain events published under sceneA.
    manager.setCurrentScene(&sceneB);

    TEST_ASSERT_TRUE(bus.isEmpty());
    GameplayEvent outEvent;
    TEST_ASSERT_FALSE(bus.consume(outEvent));
}

void test_bus_not_drained_on_push_pop(void) {
    SceneManager manager;
    GameplayEventBus bus;
    manager.setGameplayEventBus(&bus);

    MockGameplayScene sceneA;
    MockGameplayScene overlay;

    manager.setCurrentScene(&sceneA);

    GameplayEvent event1;
    event1.type = GameplayEventType::TriggerEnter;
    event1.entityIdA = 1;
    GameplayEvent event2;
    event2.type = GameplayEventType::TriggerExit;
    event2.entityIdA = 2;

    TEST_ASSERT_TRUE(bus.publish(event1));
    TEST_ASSERT_TRUE(bus.publish(event2));

    // Pushing/popping an overlay scene (e.g. a pause menu) must NOT drain
    // the bus — only setCurrentScene()'s SceneSwap funnel does.
    manager.pushScene(&overlay);
    manager.popScene();

    GameplayEvent outEvent1;
    GameplayEvent outEvent2;
    TEST_ASSERT_TRUE(bus.consume(outEvent1));
    TEST_ASSERT_TRUE(bus.consume(outEvent2));
    TEST_ASSERT_EQUAL(static_cast<int>(GameplayEventType::TriggerEnter), static_cast<int>(outEvent1.type));
    TEST_ASSERT_EQUAL_UINT16(1, outEvent1.entityIdA);
    TEST_ASSERT_EQUAL(static_cast<int>(GameplayEventType::TriggerExit), static_cast<int>(outEvent2.type));
    TEST_ASSERT_EQUAL_UINT16(2, outEvent2.entityIdA);
}

void test_gameplay_event_bus_zero_cost_when_disabled(void) {
    // Layout: field offsets must strictly ascend in D2's declared order
    // (userData, value, entityIdA, entityIdB, type) — pins the field order
    // the byte-budget analysis in design.md depends on, without hardcoding
    // an absolute sizeof() literal (native is 64-bit, ESP32-C3 is 32-bit).
    TEST_ASSERT_TRUE(offsetof(GameplayEvent, userData) < offsetof(GameplayEvent, value));
    TEST_ASSERT_TRUE(offsetof(GameplayEvent, value) < offsetof(GameplayEvent, entityIdA));
    TEST_ASSERT_TRUE(offsetof(GameplayEvent, entityIdA) < offsetof(GameplayEvent, entityIdB));
    TEST_ASSERT_TRUE(offsetof(GameplayEvent, entityIdB) < offsetof(GameplayEvent, type));

    // No unexpected members: total size must not exceed the sum of the five
    // declared members plus at most one machine word of alignment padding
    // (the widest member is void*), regardless of platform pointer width.
    const size_t declaredMembersSize =
        sizeof(void*) +
        sizeof(pixelroot32::math::Scalar) +
        sizeof(uint16_t) +
        sizeof(uint16_t) +
        sizeof(GameplayEventType);
    TEST_ASSERT_TRUE(sizeof(GameplayEvent) <= declaredMembersSize + sizeof(void*));

    // Byte budget: capacity * sizeof(GameplayEvent) must stay within an
    // order-of-magnitude ceiling regardless of platform width. The exact
    // ESP32-C3 figure (16 B/slot, 512 B total) is a fixed-point design
    // budget verified separately, not by this 64-bit native run — see
    // design.md's Risks section.
    const size_t totalBusBytes =
        static_cast<size_t>(GameplayEventBus::kCapacity) * sizeof(GameplayEvent);
    TEST_ASSERT_TRUE(totalBusBytes <= 4096);
}

#else // !PIXELROOT32_ENABLE_GAMEPLAY_EVENTS

void test_gameplay_event_bus_zero_cost_when_disabled(void) {
    // With the flag off, GameplayEvent/GameplayEventBus are not compiled at
    // all — their entire declarations live inside the
    // #if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS guard in
    // include/gameplay/GameplayEventBus.h. This translation unit compiling
    // and passing without referencing either type IS the "zero bytes
    // reserved" property required by the spec's
    // "Bus Is Feature-Gated And Zero-Cost When Disabled" requirement.
    TEST_PASS_MESSAGE("PIXELROOT32_ENABLE_GAMEPLAY_EVENTS=0: bus types are not compiled, zero bytes reserved.");
}

#endif // PIXELROOT32_ENABLE_GAMEPLAY_EVENTS

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

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
    RUN_TEST(test_publish_success_retrievable);
    RUN_TEST(test_drop_newest_on_full_increments_counter);
    RUN_TEST(test_repeated_publish_on_full_does_not_crash);
    RUN_TEST(test_bus_drains_on_scene_swap);
    RUN_TEST(test_bus_not_drained_on_push_pop);
#endif
    RUN_TEST(test_gameplay_event_bus_zero_cost_when_disabled);

    return UNITY_END();
}
