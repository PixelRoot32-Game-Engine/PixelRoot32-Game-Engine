/**
 * @file test_collision_primitives.cpp
 * @brief Unit tests for physics/CollisionPrimitives module
 * @version 1.1
 * @date 2026-02-08
 */

#include <unity.h>
#include <cstdint>
#include "../../test_config.h"
#include "physics/CollisionTypes.h"
#include "core/Entity.h"

using namespace pixelroot32::physics;
using namespace pixelroot32::core;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_collision_circle_vs_circle(void) {
    Circle c1 = { 0, 0, 10 };
    Circle c2 = { 15, 0, 10 }; // Should intersect (dist 15 < radius sum 20)
    Circle c3 = { 25, 0, 10 }; // Should not intersect (dist 25 > radius sum 20)
    
    TEST_ASSERT_TRUE(intersects(c1, c2));
    TEST_ASSERT_FALSE(intersects(c1, c3));
}

void test_collision_circle_vs_rect(void) {
    Circle c = { 0, 0, 10 };
    Rect r1 = { 5, 0, 10, 10 };  // Should intersect
    Rect r2 = { 15, 0, 10, 10 }; // Should not intersect
    
    TEST_ASSERT_TRUE(intersects(c, r1));
    TEST_ASSERT_FALSE(intersects(c, r2));
}

void test_collision_segment_vs_rect(void) {
    Segment s1 = { -10, 5, 20, 5 }; // Crosses rect from left to right
    Segment s2 = { -10, -10, -5, -5 }; // Completely outside
    Rect r = { 0, 0, 10, 10 };
    
    TEST_ASSERT_TRUE(intersects(s1, r));
    TEST_ASSERT_FALSE(intersects(s2, r));
}

void test_collision_sweep_circle_vs_rect(void) {
    Circle start = { -20, 5, 5 };
    Circle end = { 20, 5, 5 };
    Rect r = { 0, 0, 10, 10 };
    float tHit = 0;
    
    // Should hit the rect
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
    // tHit should be around 0.375 (distance from -20 to -5 is 15, total distance 40. 15/40 = 0.375)
    TEST_ASSERT_FLOAT_EQUAL(0.375f, tHit);
    
    // Should not hit
    Circle end2 = { -20, 50, 5 };
    TEST_ASSERT_FALSE(sweepCircleVsRect(start, end2, r, tHit));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_collision_circle_vs_circle);
    RUN_TEST(test_collision_circle_vs_rect);
    RUN_TEST(test_collision_segment_vs_rect);
    RUN_TEST(test_collision_sweep_circle_vs_rect);
    
    return UNITY_END();
}
