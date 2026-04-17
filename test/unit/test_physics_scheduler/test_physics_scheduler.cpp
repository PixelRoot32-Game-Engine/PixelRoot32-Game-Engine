/**
 * @file test_physics_scheduler.cpp
 * @brief Unit tests for physics/PhysicsScheduler module
 * @version 1.3.0
 * @date 2026-04-08
 * 
 * Tests for fixed timestep scheduler with time accumulator.
 */

#include <unity.h>
#include <cstring>
#include "../../test_config.h"
#include "physics/PhysicsScheduler.h"
#include "physics/CollisionSystem.h"

using namespace pixelroot32::physics;
using namespace pixelroot32::math;

// Global collision system for tests
static CollisionSystem* gCollisionSystem = nullptr;

// Test that FIXED_DT_MICROS is correct
void test_fixed_timestep_constant(void) {
    TEST_ASSERT_EQUAL_UINT32(16667, PhysicsScheduler::FIXED_DT_MICROS);
}

// Test that MAX_STEPS constants are correct
void test_max_steps_constants(void) {
    // MAX_STEPS_NORMAL = 1 (single integration per frame to avoid double integration bug)
    TEST_ASSERT_EQUAL_UINT8(1, PhysicsScheduler::MAX_STEPS_NORMAL);
    TEST_ASSERT_EQUAL_UINT8(4, PhysicsScheduler::MAX_STEPS_BACKLOG);
}

// Test initialization
void test_scheduler_init(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    TEST_ASSERT_EQUAL_UINT8(0, scheduler.getStepsExecuted());
    TEST_ASSERT_EQUAL_UINT32(0, scheduler.getAccumulator());
}

// Test: Normal frame at 60 FPS (16667 µs) should execute 1 step
void test_normal_frame_60fps(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // Simulate 60 FPS frame: 16667 µs
    uint8_t steps = scheduler.update(16667, *gCollisionSystem);
    
    // Should execute exactly 1 step (16667 >= 16667)
    TEST_ASSERT_EQUAL_UINT8(1, steps);
    TEST_ASSERT_EQUAL_UINT32(0, scheduler.getAccumulator());
}

// Test: Two normal frames should execute 2 steps total
void test_two_normal_frames(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // First frame: 16667 µs -> 1 step
    scheduler.update(16667, *gCollisionSystem);
    
    // Second frame: 16667 µs + 0 (accumulator) -> 1 step
    uint8_t steps = scheduler.update(16667, *gCollisionSystem);
    
    TEST_ASSERT_EQUAL_UINT8(1, steps);
}

// Test: Fast frame (8ms = 8000µs) should execute 0 steps
void test_fast_frame_no_steps(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // Simulate fast frame: 8000 µs (faster than 60 FPS)
    uint8_t steps = scheduler.update(8000, *gCollisionSystem);
    
    // Should execute 0 steps (8000 < 16667)
    TEST_ASSERT_EQUAL_UINT8(0, steps);
    TEST_ASSERT_EQUAL_UINT32(8000, scheduler.getAccumulator());
}

// Test: Backlog recovery - 3 frames worth should execute 3 steps
void test_backlog_recovery_3frames(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // Accumulate 3 frames worth of time: 3 * 16667 = 50001 µs
    // With backlog mode (> 2.5 frames), should execute 3 steps
    uint8_t steps = scheduler.update(50001, *gCollisionSystem);
    
    // Should execute 3 steps (16667 * 3 = 50001)
    TEST_ASSERT_EQUAL_UINT8(3, steps);
}

// Test: Maximum catch-up - 4+ frames should cap at 4 steps
void test_max_catchup_4frames(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // More than 4 frames worth: 4 * 16667 = 66668 µs
    uint8_t steps = scheduler.update(70000, *gCollisionSystem);
    
    // Should execute exactly 4 steps (MAX_STEPS_BACKLOG)
    TEST_ASSERT_EQUAL_UINT8(4, steps);
}

// Test: Very slow frame (less than 15 FPS) should still cap at 4 steps
void test_very_slow_frame_capped(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // Very slow: 100ms = 100000 µs
    uint8_t steps = scheduler.update(100000, *gCollisionSystem);
    
    // Should execute exactly 4 steps max
    TEST_ASSERT_EQUAL_UINT8(4, steps);
    // Should preserve excess time for catch-up
    TEST_ASSERT_TRUE(scheduler.getAccumulator() > 0);
}

// Test: No time discarding - excess time is preserved for next frame
void test_no_time_discarding(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // First frame: 100ms = 100000 µs = 6+ frames
    // Should execute 4 steps (max), remainder = 100000 - (4 * 16667) = 33332 µs
    scheduler.update(100000, *gCollisionSystem);
    uint32_t accumAfterFirst = scheduler.getAccumulator();
    
    // Accumulator should have ~33332 µs (not 0, time not discarded)
    TEST_ASSERT_TRUE(accumAfterFirst > 30000);
    
    // Second frame adds more time, total should grow
    scheduler.update(10000, *gCollisionSystem);
    uint32_t accumAfterSecond = scheduler.getAccumulator();
    
    // Accumulator should now have 33332 + 10000 = 43332 (minus any steps)
    TEST_ASSERT_TRUE(accumAfterSecond > 0);
}

// Test: getStepsExecuted returns correct value
void test_get_steps_executed(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    TEST_ASSERT_EQUAL_UINT8(0, scheduler.getStepsExecuted());
    
    // 50001 µs = 3 frames -> with backlog = 3 steps
    scheduler.update(50001, *gCollisionSystem);
    
    TEST_ASSERT_EQUAL_UINT8(3, scheduler.getStepsExecuted());
}

// Test: Adaptive threshold - backlog kicks in at 2.5+ frames
void test_adaptive_threshold(void) {
    PhysicsScheduler scheduler;
    scheduler.init();
    
    // At exactly 2.5 frames (41667 µs), should use MAX_STEPS_NORMAL (1 now)
    // With MAX_STEPS_NORMAL=1, we get 1 step (not 2 as before)
    uint8_t steps1 = scheduler.update(41667, *gCollisionSystem);
    TEST_ASSERT_EQUAL_UINT8(1, steps1);
    
    // Reset and try at 2.5+ frames (exceeds threshold, triggers catch-up)
    // This triggers MAX_STEPS_BACKLOG=4, but with 41667µs we get only 2 steps
    scheduler.init();
    uint8_t steps2 = scheduler.update(45000, *gCollisionSystem);
    // 45000/16667 = 2.7 frames, should get 2 steps (2 * 16667 = 33334, remainder 11666)
    TEST_ASSERT_EQUAL_UINT8(2, steps2);
}

void setUp(void) {
    // Initialize global collision system if needed
    if (gCollisionSystem == nullptr) {
        gCollisionSystem = new CollisionSystem();
    }
    gCollisionSystem->clear();
}

void tearDown(void) {
    // Cleanup after each test
    if (gCollisionSystem != nullptr) {
        gCollisionSystem->clear();
    }
}

int main(void) {
    UNITY_BEGIN();
    
    // Constants tests
    RUN_TEST(test_fixed_timestep_constant);
    RUN_TEST(test_max_steps_constants);
    
    // Initialization tests
    RUN_TEST(test_scheduler_init);
    
    // Step execution tests
    RUN_TEST(test_normal_frame_60fps);
    RUN_TEST(test_two_normal_frames);
    RUN_TEST(test_fast_frame_no_steps);
    RUN_TEST(test_backlog_recovery_3frames);
    RUN_TEST(test_max_catchup_4frames);
    RUN_TEST(test_very_slow_frame_capped);
    RUN_TEST(test_no_time_discarding);
    
    // Accessor tests
    RUN_TEST(test_get_steps_executed);
    RUN_TEST(test_adaptive_threshold);
    
    UNITY_END();
}
