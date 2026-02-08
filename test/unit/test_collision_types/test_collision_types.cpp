/**
 * @file test_collision_types.cpp
 * @brief Unit tests for physics/CollisionTypes module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for collision primitives including:
 * - Circle struct
 * - Segment struct
 * - Collision layer constants
 */

#include <unity.h>
#include "physics/CollisionTypes.h"
#include "../../test_config.h"

using namespace pixelroot32::physics;

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
// Tests for Circle struct
// =============================================================================

/**
 * @test Circle basic initialization
 * @expected All fields initialized correctly
 */
void test_circle_initialization(void) {
    Circle c = {10.0f, 20.0f, 5.0f};
    TEST_ASSERT_EQUAL_FLOAT(10.0f, c.x);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, c.y);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, c.radius);
}

/**
 * @test Circle zero initialization
 * @expected All fields are zero
 */
void test_circle_zero_initialization(void) {
    Circle c = {0.0f, 0.0f, 0.0f};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.radius);
}

/**
 * @test Circle with negative position
 * @expected Negative coordinates handled correctly
 */
void test_circle_negative_position(void) {
    Circle c = {-10.0f, -20.0f, 5.0f};
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, c.x);
    TEST_ASSERT_EQUAL_FLOAT(-20.0f, c.y);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, c.radius);
}

/**
 * @test Circle with large radius
 * @expected Large values handled correctly
 */
void test_circle_large_radius(void) {
    Circle c = {0.0f, 0.0f, 1000.0f};
    TEST_ASSERT_EQUAL_FLOAT(1000.0f, c.radius);
}

/**
 * @test Circle with very small radius
 * @expected Small values handled correctly
 */
void test_circle_small_radius(void) {
    Circle c = {0.0f, 0.0f, 0.001f};
    TEST_ASSERT_EQUAL_FLOAT(0.001f, c.radius);
}

// =============================================================================
// Tests for Segment struct
// =============================================================================

/**
 * @test Segment basic initialization
 * @expected All fields initialized correctly
 */
void test_segment_initialization(void) {
    Segment s = {10.0f, 20.0f, 30.0f, 40.0f};
    TEST_ASSERT_EQUAL_FLOAT(10.0f, s.x1);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, s.y1);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, s.x2);
    TEST_ASSERT_EQUAL_FLOAT(40.0f, s.y2);
}

/**
 * @test Segment zero initialization
 * @expected All fields are zero
 */
void test_segment_zero_initialization(void) {
    Segment s = {0.0f, 0.0f, 0.0f, 0.0f};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.x1);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.y1);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.x2);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.y2);
}

/**
 * @test Segment with negative coordinates
 * @expected Negative coordinates handled correctly
 */
void test_segment_negative_coordinates(void) {
    Segment s = {-10.0f, -20.0f, -30.0f, -40.0f};
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, s.x1);
    TEST_ASSERT_EQUAL_FLOAT(-20.0f, s.y1);
    TEST_ASSERT_EQUAL_FLOAT(-30.0f, s.x2);
    TEST_ASSERT_EQUAL_FLOAT(-40.0f, s.y2);
}

/**
 * @test Horizontal segment
 * @expected y1 equals y2
 */
void test_segment_horizontal(void) {
    Segment s = {0.0f, 10.0f, 100.0f, 10.0f};
    TEST_ASSERT_EQUAL_FLOAT(s.y1, s.y2);
}

/**
 * @test Vertical segment
 * @expected x1 equals x2
 */
void test_segment_vertical(void) {
    Segment s = {10.0f, 0.0f, 10.0f, 100.0f};
    TEST_ASSERT_EQUAL_FLOAT(s.x1, s.x2);
}

/**
 * @test Point segment (zero length)
 * @expected Start and end are the same point
 */
void test_segment_point(void) {
    Segment s = {10.0f, 20.0f, 10.0f, 20.0f};
    TEST_ASSERT_EQUAL_FLOAT(s.x1, s.x2);
    TEST_ASSERT_EQUAL_FLOAT(s.y1, s.y2);
}

// =============================================================================
// Tests for DefaultLayers constants
// =============================================================================

/**
 * @test kNone layer constant
 * @expected Value is 0
 */
void test_default_layers_none(void) {
    TEST_ASSERT_EQUAL_UINT16(0, DefaultLayers::kNone);
}

/**
 * @test kAll layer constant
 * @expected Value is 0xFFFF
 */
void test_default_layers_all(void) {
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, DefaultLayers::kAll);
}

/**
 * @test CollisionLayer type size
 * @expected Is uint16_t (2 bytes)
 */
void test_collision_layer_size(void) {
    TEST_ASSERT_EQUAL_INT(2, sizeof(CollisionLayer));
}

// =============================================================================
// Tests for CollisionLayer bitwise operations
// =============================================================================

/**
 * @test OR operation on layers
 * @expected Correctly combines layers
 */
void test_collision_layer_or_operation(void) {
    CollisionLayer layer1 = 0x0001;
    CollisionLayer layer2 = 0x0002;
    CollisionLayer result = layer1 | layer2;
    TEST_ASSERT_EQUAL_UINT16(0x0003, result);
}

/**
 * @test AND operation on layers
 * @expected Correctly intersects layers
 */
void test_collision_layer_and_operation(void) {
    CollisionLayer layer1 = 0x0003;
    CollisionLayer layer2 = 0x0001;
    CollisionLayer result = layer1 & layer2;
    TEST_ASSERT_EQUAL_UINT16(0x0001, result);
}

/**
 * @test NOT operation on layers
 * @expected Correctly inverts all bits
 */
void test_collision_layer_not_operation(void) {
    CollisionLayer layer = 0x0000;
    CollisionLayer result = ~layer;
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, result);
}

/**
 * @test XOR operation on layers
 * @expected Correctly toggles bits
 */
void test_collision_layer_xor_operation(void) {
    CollisionLayer layer1 = 0x0003;
    CollisionLayer layer2 = 0x0001;
    CollisionLayer result = layer1 ^ layer2;
    TEST_ASSERT_EQUAL_UINT16(0x0002, result);
}

// =============================================================================
// Tests for custom layer definitions
// =============================================================================

/**
 * @test Custom layer definitions (bit shifting)
 * @expected Each layer is a unique bit
 */
void test_custom_layers_unique_bits(void) {
    CollisionLayer layer1 = 1 << 0;  // Bit 0
    CollisionLayer layer2 = 1 << 1;  // Bit 1
    CollisionLayer layer3 = 1 << 2;  // Bit 2
    CollisionLayer layer4 = 1 << 3;  // Bit 3
    
    TEST_ASSERT_EQUAL_UINT16(0x0001, layer1);
    TEST_ASSERT_EQUAL_UINT16(0x0002, layer2);
    TEST_ASSERT_EQUAL_UINT16(0x0004, layer3);
    TEST_ASSERT_EQUAL_UINT16(0x0008, layer4);
}

/**
 * @test Layer mask with multiple bits
 * @expected Can represent multiple layers
 */
void test_layer_mask_multiple_bits(void) {
    CollisionLayer mask = (1 << 0) | (1 << 2) | (1 << 4);
    TEST_ASSERT_EQUAL_UINT16(0x0015, mask);
}

/**
 * @test Layer collision check with AND
 * @expected Correctly checks if layers overlap
 */
void test_layer_collision_check(void) {
    (void)(1 << 1);  // actorLayer: Layer 1 (not used directly)
    CollisionLayer actorMask = (1 << 2) | (1 << 3);  // Interested in layers 2 and 3
    CollisionLayer otherLayer = 1 << 2;  // Layer 2
    
    // Check if actor's mask includes other's layer
    bool canCollide = (actorMask & otherLayer) != 0;
    TEST_ASSERT_TRUE(canCollide);
}

/**
 * @test Layer collision check no overlap
 * @expected Returns false when no overlap
 */
void test_layer_collision_check_no_overlap(void) {
    (void)(1 << 1);  // actorLayer: Layer 1 (not used directly)
    CollisionLayer actorMask = (1 << 2) | (1 << 3);  // Interested in layers 2 and 3
    CollisionLayer otherLayer = 1 << 4;  // Layer 4
    
    // Check if actor's mask includes other's layer
    bool canCollide = (actorMask & otherLayer) != 0;
    TEST_ASSERT_FALSE(canCollide);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // Circle tests
    RUN_TEST(test_circle_initialization);
    RUN_TEST(test_circle_zero_initialization);
    RUN_TEST(test_circle_negative_position);
    RUN_TEST(test_circle_large_radius);
    RUN_TEST(test_circle_small_radius);
    
    // Segment tests
    RUN_TEST(test_segment_initialization);
    RUN_TEST(test_segment_zero_initialization);
    RUN_TEST(test_segment_negative_coordinates);
    RUN_TEST(test_segment_horizontal);
    RUN_TEST(test_segment_vertical);
    RUN_TEST(test_segment_point);
    
    // Default layers tests
    RUN_TEST(test_default_layers_none);
    RUN_TEST(test_default_layers_all);
    RUN_TEST(test_collision_layer_size);
    
    // Bitwise operations tests
    RUN_TEST(test_collision_layer_or_operation);
    RUN_TEST(test_collision_layer_and_operation);
    RUN_TEST(test_collision_layer_not_operation);
    RUN_TEST(test_collision_layer_xor_operation);
    
    // Custom layers tests
    RUN_TEST(test_custom_layers_unique_bits);
    RUN_TEST(test_layer_mask_multiple_bits);
    RUN_TEST(test_layer_collision_check);
    RUN_TEST(test_layer_collision_check_no_overlap);
    
    return UNITY_END();
}
