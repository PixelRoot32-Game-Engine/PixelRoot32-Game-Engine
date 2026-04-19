/**
 * @file test_color_depth_manager.cpp
 * @brief Unit tests for graphics/ColorDepthManager module
 * @version 1.0
 * @date 2026-04-19
 *
 * Tests for ColorDepthManager including:
 * - setDepth validation (valid returns true, invalid returns false)
 * - Default depth fallback behavior
 * - Depth enum operations
 */

#include <unity.h>
#include "graphics/ColorDepthManager.h"
#include "../../test_config.h"

using namespace pixelroot32::graphics;

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
// Tests for ColorDepthManager::setDepth(int) - Valid Values
// =============================================================================

/**
 * @test setDepth(24) returns true
 * @expected true
 */
void test_setDepth_24_returns_true(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(24);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(24, manager.getDepthBits());
}

/**
 * @test setDepth(16) returns true
 * @expected true
 */
void test_setDepth_16_returns_true(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(16);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(8) returns true
 * @expected true
 */
void test_setDepth_8_returns_true(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(8);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(8, manager.getDepthBits());
}

/**
 * @test setDepth(4) returns true
 * @expected true
 */
void test_setDepth_4_returns_true(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(4);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, manager.getDepthBits());
}

// =============================================================================
// Tests for ColorDepthManager::setDepth(int) - Invalid Values
// =============================================================================

/**
 * @test setDepth(0) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_0_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(0);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(1) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_1_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(1);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(2) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_2_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(2);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(3) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_3_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(3);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(5) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_5_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(5);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(32) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_32_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(32);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(100) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_100_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(100);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(-1) returns false and defaults to 16-bit
 * @expected false, depth defaults to 16
 */
void test_setDepth_negative_returns_false(void) {
    ColorDepthManager manager;
    bool result = manager.setDepth(-1);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

// =============================================================================
// Tests for Depth Enum
// =============================================================================

/**
 * @test setDepth(Depth::Depth16) works correctly
 * @expected depth set to 16-bit
 */
void test_setDepth_enum_depth16(void) {
    ColorDepthManager manager;
    manager.setDepth(ColorDepthManager::Depth::Depth16);
    TEST_ASSERT_EQUAL_INT(16, manager.getDepthBits());
}

/**
 * @test setDepth(Depth::Depth8) works correctly
 * @expected depth set to 8-bit
 */
void test_setDepth_enum_depth8(void) {
    ColorDepthManager manager;
    manager.setDepth(ColorDepthManager::Depth::Depth8);
    TEST_ASSERT_EQUAL_INT(8, manager.getDepthBits());
}

/**
 * @test getBytesPerPixel returns correct values
 * @expected 3 for 24-bit, 2 for 16-bit, 1 for 8-bit/4-bit
 */
void test_getBytesPerPixel(void) {
    ColorDepthManager manager;

    manager.setDepth(24);
    TEST_ASSERT_EQUAL_INT(3, manager.getBytesPerPixel());

    manager.setDepth(16);
    TEST_ASSERT_EQUAL_INT(2, manager.getBytesPerPixel());

    manager.setDepth(8);
    TEST_ASSERT_EQUAL_INT(1, manager.getBytesPerPixel());

    manager.setDepth(4);
    TEST_ASSERT_EQUAL_INT(1, manager.getBytesPerPixel());
}

/**
 * @test needsPaletteConversion returns correct values
 * @expected true for 8-bit and 4-bit, false for 16-bit and 24-bit
 */
void test_needsPaletteConversion(void) {
    ColorDepthManager manager;

    manager.setDepth(24);
    TEST_ASSERT_FALSE(manager.needsPaletteConversion());

    manager.setDepth(16);
    TEST_ASSERT_FALSE(manager.needsPaletteConversion());

    manager.setDepth(8);
    TEST_ASSERT_TRUE(manager.needsPaletteConversion());

    manager.setDepth(4);
    TEST_ASSERT_TRUE(manager.needsPaletteConversion());
}

// =============================================================================
// Tests for Statistics
// =============================================================================

/**
 * @test Initial statistics are zero
 * @expected all counters start at 0
 */
void test_initial_statistics(void) {
    ColorDepthManager manager;
    TEST_ASSERT_EQUAL_UINT32(0, manager.getTotalBytesTransferred());
    TEST_ASSERT_EQUAL_UINT32(0, manager.getFrameCount());
}

/**
 * @test addBytesTransferred accumulates correctly
 * @expected total increases
 */
void test_addBytesTransferred(void) {
    ColorDepthManager manager;
    manager.addBytesTransferred(100);
    manager.addBytesTransferred(200);
    TEST_ASSERT_EQUAL_UINT32(300, manager.getTotalBytesTransferred());
}

/**
 * @test incrementFrameCount increments correctly
 * @expected frame count increases
 */
void test_incrementFrameCount(void) {
    ColorDepthManager manager;
    manager.incrementFrameCount();
    manager.incrementFrameCount();
    manager.incrementFrameCount();
    TEST_ASSERT_EQUAL_UINT32(3, manager.getFrameCount());
}

/**
 * @test getAverageBytesPerFrame calculates correctly
 * @expected average = total / frameCount
 */
void test_getAverageBytesPerFrame(void) {
    ColorDepthManager manager;
    manager.addBytesTransferred(100);
    manager.incrementFrameCount();
    manager.addBytesTransferred(200);
    manager.incrementFrameCount();
    TEST_ASSERT_EQUAL_UINT32(150, manager.getAverageBytesPerFrame());
}

/**
 * @test resetStatistics clears counters
 * @expected all counters reset to 0
 */
void test_resetStatistics(void) {
    ColorDepthManager manager;
    manager.addBytesTransferred(100);
    manager.incrementFrameCount();
    manager.resetStatistics();
    TEST_ASSERT_EQUAL_UINT32(0, manager.getTotalBytesTransferred());
    TEST_ASSERT_EQUAL_UINT32(0, manager.getFrameCount());
}

// =============================================================================
// Tests for Transfer Ratio
// =============================================================================

/**
 * @test getTransferRatio returns correct ratios
 * @expected 1.0 for 24-bit, 0.667 for 16-bit, 0.333 for 8-bit
 */
void test_getTransferRatio(void) {
    ColorDepthManager manager;

    manager.setDepth(24);
    TEST_ASSERT_FLOAT_EQUAL(1.0f, manager.getTransferRatio());

    manager.setDepth(16);
    TEST_ASSERT_FLOAT_EQUAL(0.666667f, manager.getTransferRatio());

    manager.setDepth(8);
    TEST_ASSERT_FLOAT_EQUAL(0.333333f, manager.getTransferRatio());
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    // Valid depth values (should return true)
    RUN_TEST(test_setDepth_24_returns_true);
    RUN_TEST(test_setDepth_16_returns_true);
    RUN_TEST(test_setDepth_8_returns_true);
    RUN_TEST(test_setDepth_4_returns_true);

    // Invalid depth values (should return false)
    RUN_TEST(test_setDepth_0_returns_false);
    RUN_TEST(test_setDepth_1_returns_false);
    RUN_TEST(test_setDepth_2_returns_false);
    RUN_TEST(test_setDepth_3_returns_false);
    RUN_TEST(test_setDepth_5_returns_false);
    RUN_TEST(test_setDepth_32_returns_false);
    RUN_TEST(test_setDepth_100_returns_false);
    RUN_TEST(test_setDepth_negative_returns_false);

    // Depth enum tests
    RUN_TEST(test_setDepth_enum_depth16);
    RUN_TEST(test_setDepth_enum_depth8);
    RUN_TEST(test_getBytesPerPixel);
    RUN_TEST(test_needsPaletteConversion);

    // Statistics tests
    RUN_TEST(test_initial_statistics);
    RUN_TEST(test_addBytesTransferred);
    RUN_TEST(test_incrementFrameCount);
    RUN_TEST(test_getAverageBytesPerFrame);
    RUN_TEST(test_resetStatistics);

    // Transfer ratio tests
    RUN_TEST(test_getTransferRatio);

    return UNITY_END();
}