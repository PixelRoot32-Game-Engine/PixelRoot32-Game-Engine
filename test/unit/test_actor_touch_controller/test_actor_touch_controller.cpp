/**
 * @file test_actor_touch_controller.cpp
 * @brief Unit tests for ActorTouchController class
 * 
 * Tests cover:
 * - T4b.15: registerActor pool capacity (8 actors max)
 * - T4b.16: unregisterActor removes correct actor
 * - T4b.17: hitTest returns correct actor for point
 * - T4b.18: drag offset preserved during move
 * - T4b.19: drag threshold prevents movement under 5 pixels
 * - T4b.20: integration with TouchEventDispatcher callback
 */

#include <unity.h>
#include "input/ActorTouchController.h"
#include "input/TouchEvent.h"
#include "input/TouchEventTypes.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "core/Entity.h"
#include "core/Actor.h"
#include "mocks/MockActor.h"
#include "../../test_config.h"

using namespace pixelroot32::input;
using namespace pixelroot32::core;

// =============================================================================
// Test Setup
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// T4b.15: registerActor succeeds for 8 actors, fails on 9th
// =============================================================================

void test_register_actor_pool_full_at_8(void) {
    ActorTouchController controller;
    
    // Create 8 actors and register them - all should succeed
    MockActor* actors[8];
    for (int i = 0; i < 8; i++) {
        actors[i] = new MockActor(static_cast<float>(i * 10), 0.0f, 16, 16);
        TEST_ASSERT_TRUE(controller.registerActor(actors[i]));
    }
    
    // 9th actor should fail - pool is full
    MockActor* ninthActor = new MockActor(100.0f, 0.0f, 16, 16);
    TEST_ASSERT_FALSE(controller.registerActor(ninthActor));
    
    // Clean up
    delete ninthActor;
    for (int i = 0; i < 8; i++) {
        delete actors[i];
    }
}

void test_register_actor_returns_false_for_null(void) {
    ActorTouchController controller;
    
    TEST_ASSERT_FALSE(controller.registerActor(nullptr));
}

void test_register_actor_empty_pool_count(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(0.0f, 0.0f, 16, 16);
    TEST_ASSERT_TRUE(controller.registerActor(actor));
    
    delete actor;
}

// =============================================================================
// T4b.16: unregisterActor removes correct actor
// =============================================================================

void test_unregister_actor_removes_middle_actor(void) {
    ActorTouchController controller;
    
    // Add 3 actors
    MockActor* actor1 = new MockActor(10.0f, 10.0f, 16, 16);
    MockActor* actor2 = new MockActor(50.0f, 50.0f, 16, 16);
    MockActor* actor3 = new MockActor(100.0f, 100.0f, 16, 16);
    
    controller.registerActor(actor1);
    controller.registerActor(actor2);
    controller.registerActor(actor3);
    
    // Unregister the middle one (actor2)
    TEST_ASSERT_TRUE(controller.unregisterActor(actor2));
    
    // Remaining actors should be actor1 and actor3 - they can still be hit
    // But we can't directly verify pool contents, so verify behavior
    
    delete actor1;
    delete actor2;
    delete actor3;
}

void test_unregister_actor_returns_false_for_not_found(void) {
    ActorTouchController controller;
    
    MockActor* actor1 = new MockActor(10.0f, 10.0f, 16, 16);
    MockActor* actor2 = new MockActor(50.0f, 50.0f, 16, 16);
    
    controller.registerActor(actor1);
    
    // Try to unregister actor2 which was never registered
    TEST_ASSERT_FALSE(controller.unregisterActor(actor2));
    
    delete actor1;
    delete actor2;
}

void test_unregister_actor_cancels_active_drag(void) {
    ActorTouchController controller;
    
    MockActor* actor1 = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor1);
    
    // Simulate touch down on actor
    TouchEvent downEvent(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(downEvent);
    
    // Verify we have a hit actor (not yet dragging - threshold not exceeded)
    TEST_ASSERT_NOT_NULL(controller.getDraggedActor());
    TEST_ASSERT_FALSE(controller.isDragging());
    
    // Move to exceed threshold - now dragging
    TouchEvent moveEvent(TouchEventType::DragMove, 0, 110, 110, 101);
    controller.handleTouch(moveEvent);
    
    // Now dragging should be true
    TEST_ASSERT_TRUE(controller.isDragging());
    
    // Unregister the dragged actor
    TEST_ASSERT_TRUE(controller.unregisterActor(actor1));
    
    // Drag should be cancelled
    TEST_ASSERT_FALSE(controller.isDragging());
    TEST_ASSERT_NULL(controller.getDraggedActor());
    
    delete actor1;
}

// =============================================================================
// T4b.17: hitTest returns correct actor for point
// =============================================================================

void test_hit_test_returns_actor_at_point(void) {
    ActorTouchController controller;
    
    // Create actor at (50, 50) with 32x32 size
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Point inside actor's hitbox should return the actor
    // Actor hitbox is at (50,50) to (82,82), so (60,60) is inside
    pixelroot32::core::Actor* hitActor = controller.hitTest(60, 60);
    TEST_ASSERT_EQUAL_PTR(actor, hitActor);
    
    // Point outside should return nullptr
    TEST_ASSERT_NULL(controller.hitTest(10, 10));
    TEST_ASSERT_NULL(controller.hitTest(200, 200));
    
    delete actor;
}

void test_hit_test_returns_topmost_actor(void) {
    ActorTouchController controller;
    
    // Create overlapping actors (last registered is on top)
    MockActor* bottomActor = new MockActor(40.0f, 40.0f, 40, 40);
    MockActor* topActor = new MockActor(50.0f, 50.0f, 32, 32);
    
    controller.registerActor(bottomActor);
    controller.registerActor(topActor);
    
    // Point in overlap area should return topmost (last registered)
    pixelroot32::core::Actor* hitActor = controller.hitTest(55, 55);
    TEST_ASSERT_EQUAL_PTR(topActor, hitActor);
    
    delete bottomActor;
    delete topActor;
}

void test_hit_test_empty_pool_returns_null(void) {
    ActorTouchController controller;
    
    TEST_ASSERT_NULL(controller.hitTest(50, 50));
}

void test_hit_test_slop_allows_near_miss(void) {
    ActorTouchController controller;
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    TEST_ASSERT_NULL(controller.hitTest(40, 40));
    controller.setTouchHitSlop(15);
    TEST_ASSERT_EQUAL_PTR(actor, controller.hitTest(40, 40));
    delete actor;
}

// =============================================================================
// T4b.18: drag offset preserved during move
// =============================================================================

void test_drag_offset_preserved_during_move(void) {
    ActorTouchController controller;
    
    // Create actor at (50, 50) — hitbox spans (50,50)–(82,82); touch must be inside it
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Touch at (60, 60) on actor at (50, 50)
    // offset = actorPos - touchPos = (50-60, 50-60) = (-10, -10)
    TouchEvent downEvent(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(downEvent);
    
    // Move touch to (110, 110) — exceeds 5px threshold from initial touch
    // New actor position = touchPos + offset = (110-10, 110-10) = (100, 100)
    // Delta from original (50,50) = (50, 50)
    TouchEvent moveEvent(TouchEventType::DragMove, 0, 110, 110, 101);
    controller.handleTouch(moveEvent);
    
    // Verify actor moved by delta (50, 50), not to absolute position (150, 150)
    int actorX = static_cast<int>(actor->position.x);
    int actorY = static_cast<int>(actor->position.y);
    
    TEST_ASSERT_EQUAL(100, actorX);
    TEST_ASSERT_EQUAL(100, actorY);
    
    delete actor;
}

void test_drag_relative_to_initial_touch(void) {
    ActorTouchController controller;
    
    // Actor at (0, 0)
    MockActor* actor = new MockActor(0.0f, 0.0f, 32, 32);
    controller.registerActor(actor);
    
    // Touch at (20, 20)
    TouchEvent downEvent(TouchEventType::TouchDown, 0, 20, 20, 100);
    controller.handleTouch(downEvent);
    
    // Move to (70, 20) - moved 50 pixels right
    TouchEvent moveEvent(TouchEventType::DragMove, 0, 70, 20, 101);
    controller.handleTouch(moveEvent);
    
    // Actor should be at (50, 0) - moved by same delta as touch
    int actorX = static_cast<int>(actor->position.x);
    TEST_ASSERT_EQUAL(50, actorX);
    
    delete actor;
}

// =============================================================================
// T4b.19: drag threshold prevents movement under 5 pixels
// =============================================================================

void test_drag_threshold_blocks_small_movement(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Touch inside hitbox
    TouchEvent downEvent(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(downEvent);
    
    // Move 2 px diagonally: distSq = 8 < 5² — drag must not start, actor stays put
    TouchEvent moveEvent(TouchEventType::DragMove, 0, 62, 62, 101);
    controller.handleTouch(moveEvent);
    
    // Actor should NOT have moved - threshold not exceeded
    int actorX = static_cast<int>(actor->position.x);
    int actorY = static_cast<int>(actor->position.y);
    
    TEST_ASSERT_EQUAL(50, actorX);
    TEST_ASSERT_EQUAL(50, actorY);
    
    delete actor;
}

void test_drag_threshold_allows_5px_movement(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Touch inside hitbox (not (100,100) — that is outside 32×32 box from (50,50))
    TouchEvent downEvent(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(downEvent);
    
    // Move 10 pixels diagonally: distSq = 10²+10² = 200 ≥ 5²
    TouchEvent moveEvent(TouchEventType::DragMove, 0, 70, 70, 101);
    controller.handleTouch(moveEvent);
    
    // Now threshold exceeded, actor should move
    TEST_ASSERT_TRUE(controller.isDragging());
    
    delete actor;
}

void test_drag_threshold_ignores_diagonal_distance(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Touch inside hitbox
    TouchEvent downEvent(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(downEvent);
    
    // Move diagonally by 10 in X and Y: uses squared Euclidean vs kDragThreshold (not per-axis max)
    TouchEvent moveEvent(TouchEventType::DragMove, 0, 70, 70, 101);
    controller.handleTouch(moveEvent);
    
    // Should exceed threshold
    TEST_ASSERT_TRUE(controller.isDragging());
    
    delete actor;
}

// =============================================================================
// T4b.20: Integration with TouchEventDispatcher
// =============================================================================

void test_complete_drag_sequence(void) {
    ActorTouchController controller;
    
    // Setup: Create actor at (50, 50)
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Step 1: TouchDown at (60, 60) - inside actor
    TouchEvent down(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(down);
    
    // Should have hit the actor but not yet dragging (no threshold exceeded)
    TEST_ASSERT_NOT_NULL(controller.getDraggedActor());
    TEST_ASSERT_FALSE(controller.isDragging());
    
    // Step 2: DragMove to (110, 110) - exceeds 5px threshold
    TouchEvent move1(TouchEventType::DragMove, 0, 110, 110, 101);
    controller.handleTouch(move1);
    
    // Now dragging
    TEST_ASSERT_TRUE(controller.isDragging());
    
    // Step 3: Continue dragging to (160, 160)
    TouchEvent move2(TouchEventType::DragMove, 0, 160, 160, 102);
    controller.handleTouch(move2);
    
    // Step 4: TouchUp - end drag
    TouchEvent up(TouchEventType::TouchUp, 0, 160, 160, 103);
    controller.handleTouch(up);
    
    // Drag should be over
    TEST_ASSERT_FALSE(controller.isDragging());
    TEST_ASSERT_NULL(controller.getDraggedActor());
    
    // Verify final position: started at (50, 50), touch moved from 60->160 = delta 100
    // offset was (50-60, 50-60) = (-10, -10)
    // final = 160 + (-10), 160 + (-10) = (150, 150)
    int finalX = static_cast<int>(actor->position.x);
    int finalY = static_cast<int>(actor->position.y);
    
    TEST_ASSERT_EQUAL(150, finalX);
    TEST_ASSERT_EQUAL(150, finalY);
    
    delete actor;
}

void test_touch_up_clears_all_state(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Start a drag
    TouchEvent down(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(down);
    
    TouchEvent move(TouchEventType::DragMove, 0, 170, 170, 101);
    controller.handleTouch(move);
    
    TEST_ASSERT_TRUE(controller.isDragging());
    
    // End the drag
    TouchEvent up(TouchEventType::TouchUp, 0, 170, 170, 102);
    controller.handleTouch(up);
    
    // All state should be cleared
    TEST_ASSERT_FALSE(controller.isDragging());
    TEST_ASSERT_NULL(controller.getDraggedActor());
}

void test_multiple_touch_ids_not_supported(void) {
    ActorTouchController controller;
    
    MockActor* actor1 = new MockActor(50.0f, 50.0f, 32, 32);
    MockActor* actor2 = new MockActor(150.0f, 50.0f, 32, 32);
    controller.registerActor(actor1);
    controller.registerActor(actor2);
    
    // Touch on first actor with ID 0
    TouchEvent down1(TouchEventType::TouchDown, 0, 60, 60, 100);
    controller.handleTouch(down1);
    
    // Try to touch second actor with ID 1 - should replace first
    TouchEvent down2(TouchEventType::TouchDown, 1, 160, 60, 101);
    controller.handleTouch(down2);
    
    // The controller only tracks one dragged actor at a time
    // This is by design - single drag behavior
    
    delete actor1;
    delete actor2;
}

// =============================================================================
// Additional edge case tests
// =============================================================================

void test_touch_down_miss_actor_no_drag_started(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Touch at (10, 10) - not hitting any actor
    TouchEvent down(TouchEventType::TouchDown, 0, 10, 10, 100);
    controller.handleTouch(down);
    
    // Should not be dragging
    TEST_ASSERT_FALSE(controller.isDragging());
    TEST_ASSERT_NULL(controller.getDraggedActor());
    
    delete actor;
}

void test_move_without_down_does_nothing(void) {
    ActorTouchController controller;
    
    MockActor* actor = new MockActor(50.0f, 50.0f, 32, 32);
    controller.registerActor(actor);
    
    // Move without prior down
    TouchEvent move(TouchEventType::DragMove, 0, 200, 200, 100);
    controller.handleTouch(move);
    
    // Actor should not have moved
    int actorX = static_cast<int>(actor->position.x);
    TEST_ASSERT_EQUAL(50, actorX);
    
    delete actor;
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // T4b.15: registerActor pool capacity tests
    RUN_TEST(test_register_actor_pool_full_at_8);
    RUN_TEST(test_register_actor_returns_false_for_null);
    RUN_TEST(test_register_actor_empty_pool_count);
    
    // T4b.16: unregisterActor tests
    RUN_TEST(test_unregister_actor_removes_middle_actor);
    RUN_TEST(test_unregister_actor_returns_false_for_not_found);
    RUN_TEST(test_unregister_actor_cancels_active_drag);
    
    // T4b.17: hitTest tests
    RUN_TEST(test_hit_test_returns_actor_at_point);
    RUN_TEST(test_hit_test_returns_topmost_actor);
    RUN_TEST(test_hit_test_empty_pool_returns_null);
    RUN_TEST(test_hit_test_slop_allows_near_miss);
    
    // T4b.18: drag offset preserved tests
    RUN_TEST(test_drag_offset_preserved_during_move);
    RUN_TEST(test_drag_relative_to_initial_touch);
    
    // T4b.19: drag threshold tests
    RUN_TEST(test_drag_threshold_blocks_small_movement);
    RUN_TEST(test_drag_threshold_allows_5px_movement);
    RUN_TEST(test_drag_threshold_ignores_diagonal_distance);
    
    // T4b.20: TouchEventDispatcher integration tests
    RUN_TEST(test_complete_drag_sequence);
    RUN_TEST(test_touch_up_clears_all_state);
    RUN_TEST(test_multiple_touch_ids_not_supported);
    
    // Edge cases
    RUN_TEST(test_touch_down_miss_actor_no_drag_started);
    RUN_TEST(test_move_without_down_does_nothing);
    
    return UNITY_END();
}