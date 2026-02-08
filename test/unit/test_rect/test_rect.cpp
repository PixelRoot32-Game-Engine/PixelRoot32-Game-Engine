/**
 * @file test_rect.cpp
 * @brief Unit tests for core/Rect structure
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for Rect structure including:
 * - Basic initialization
 * - Intersection detection (AABB)
 * - Edge cases for collision
 */

#include <unity.h>
#include "../../test_config.h"

// Copiar solo la definición de Rect para evitar dependencias de Arduino
namespace pixelroot32 {
namespace core {

struct Rect {
    float x, y;
    int width, height;

    bool intersects(const Rect& other) const {
        return !(x + width < other.x || x > other.x + other.width ||
                 y + height < other.y || y > other.y + other.height);
    }
};

} // namespace core
} // namespace pixelroot32

using namespace pixelroot32::core;

// =============================================================================
// Setup / Teardown
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for Rect initialization
// =============================================================================

void test_rect_initialization(void) {
    Rect r = {10.0f, 20.0f, 30, 40};
    TEST_ASSERT_EQUAL_FLOAT(10.0f, r.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, r.y);
    TEST_ASSERT_EQUAL_INT(30, r.width);
    TEST_ASSERT_EQUAL_INT(40, r.height);
}

void test_rect_zero_initialization(void) {
    Rect r = {0.0f, 0.0f, 0, 0};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, r.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, r.y);
    TEST_ASSERT_EQUAL_INT(0, r.width);
    TEST_ASSERT_EQUAL_INT(0, r.height);
}

void test_rect_negative_position(void) {
    Rect r = {-10.0f, -20.0f, 30, 40};
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, r.x);
    TEST_ASSERT_EQUAL_FLOAT(-20.0f, r.y);
    TEST_ASSERT_EQUAL_INT(30, r.width);
    TEST_ASSERT_EQUAL_INT(40, r.height);
}

// =============================================================================
// Tests for Rect::intersects - Basic cases
// =============================================================================

void test_rect_intersects_identical(void) {
    Rect r1 = {10.0f, 10.0f, 20, 20};
    Rect r2 = {10.0f, 10.0f, 20, 20};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_overlapping(void) {
    Rect r1 = {10.0f, 10.0f, 20, 20};
    Rect r2 = {15.0f, 15.0f, 20, 20};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_touching_corner(void) {
    Rect r1 = {10.0f, 10.0f, 10, 10};
    Rect r2 = {20.0f, 20.0f, 10, 10};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_touching_edge(void) {
    Rect r1 = {10.0f, 10.0f, 10, 10};
    Rect r2 = {20.0f, 10.0f, 10, 10};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_separate(void) {
    Rect r1 = {10.0f, 10.0f, 10, 10};
    Rect r2 = {30.0f, 30.0f, 10, 10};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

void test_rect_intersects_contained(void) {
    Rect r1 = {10.0f, 10.0f, 30, 30};
    Rect r2 = {15.0f, 15.0f, 10, 10};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

// =============================================================================
// Tests for Rect::intersects - Directional separation
// =============================================================================

void test_rect_intersects_separated_right(void) {
    Rect r1 = {10.0f, 10.0f, 10, 10};
    Rect r2 = {25.0f, 10.0f, 10, 10};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

void test_rect_intersects_separated_left(void) {
    Rect r1 = {30.0f, 10.0f, 10, 10};
    Rect r2 = {10.0f, 10.0f, 10, 10};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

void test_rect_intersects_separated_above(void) {
    Rect r1 = {10.0f, 30.0f, 10, 10};
    Rect r2 = {10.0f, 10.0f, 10, 10};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

void test_rect_intersects_separated_below(void) {
    Rect r1 = {10.0f, 10.0f, 10, 10};
    Rect r2 = {10.0f, 30.0f, 10, 10};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

// =============================================================================
// Tests for Rect::intersects - Edge cases
// =============================================================================

void test_rect_intersects_zero_width(void) {
    Rect r1 = {10.0f, 10.0f, 0, 10};
    Rect r2 = {10.0f, 10.0f, 10, 10};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_zero_height(void) {
    Rect r1 = {10.0f, 10.0f, 10, 0};
    Rect r2 = {10.0f, 10.0f, 10, 10};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_both_zero_size_same_pos(void) {
    Rect r1 = {10.0f, 10.0f, 0, 0};
    Rect r2 = {10.0f, 10.0f, 0, 0};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_both_zero_size_diff_pos(void) {
    Rect r1 = {10.0f, 10.0f, 0, 0};
    Rect r2 = {20.0f, 20.0f, 0, 0};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

void test_rect_intersects_large_values(void) {
    Rect r1 = {0.0f, 0.0f, 10000, 10000};
    Rect r2 = {5000.0f, 5000.0f, 10000, 10000};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_float_precision(void) {
    Rect r1 = {0.5f, 0.5f, 10, 10};
    Rect r2 = {10.5f, 0.5f, 10, 10};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_negative_positions(void) {
    Rect r1 = {-20.0f, -20.0f, 30, 30};
    Rect r2 = {-10.0f, -10.0f, 30, 30};
    TEST_ASSERT_TRUE(r1.intersects(r2));
}

void test_rect_intersects_negative_positions_separated(void) {
    Rect r1 = {-50.0f, -50.0f, 10, 10};
    Rect r2 = {-20.0f, -20.0f, 10, 10};
    TEST_ASSERT_FALSE(r1.intersects(r2));
}

// =============================================================================
// Symmetry tests
// =============================================================================

void test_rect_intersects_symmetric_overlap(void) {
    Rect r1 = {10.0f, 10.0f, 20, 20};
    Rect r2 = {15.0f, 15.0f, 20, 20};
    TEST_ASSERT_EQUAL(r1.intersects(r2), r2.intersects(r1));
}

void test_rect_intersects_symmetric_no_overlap(void) {
    Rect r1 = {10.0f, 10.0f, 10, 10};
    Rect r2 = {50.0f, 50.0f, 10, 10};
    TEST_ASSERT_EQUAL(r1.intersects(r2), r2.intersects(r1));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_rect_initialization);
    RUN_TEST(test_rect_zero_initialization);
    RUN_TEST(test_rect_negative_position);
    
    // Basic intersection tests
    RUN_TEST(test_rect_intersects_identical);
    RUN_TEST(test_rect_intersects_overlapping);
    RUN_TEST(test_rect_intersects_touching_corner);
    RUN_TEST(test_rect_intersects_touching_edge);
    RUN_TEST(test_rect_intersects_separate);
    RUN_TEST(test_rect_intersects_contained);
    
    // Directional separation tests
    RUN_TEST(test_rect_intersects_separated_right);
    RUN_TEST(test_rect_intersects_separated_left);
    RUN_TEST(test_rect_intersects_separated_above);
    RUN_TEST(test_rect_intersects_separated_below);
    
    // Edge case tests
    RUN_TEST(test_rect_intersects_zero_width);
    RUN_TEST(test_rect_intersects_zero_height);
    RUN_TEST(test_rect_intersects_both_zero_size_same_pos);
    RUN_TEST(test_rect_intersects_both_zero_size_diff_pos);
    RUN_TEST(test_rect_intersects_large_values);
    RUN_TEST(test_rect_intersects_float_precision);
    RUN_TEST(test_rect_intersects_negative_positions);
    RUN_TEST(test_rect_intersects_negative_positions_separated);
    
    // Symmetry tests
    RUN_TEST(test_rect_intersects_symmetric_overlap);
    RUN_TEST(test_rect_intersects_symmetric_no_overlap);
    
    return UNITY_END();
}
