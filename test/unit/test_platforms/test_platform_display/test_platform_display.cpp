/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Unit tests for platform display configuration and operations
 */

#include <unity.h>
#include <graphics/DisplayConfig.h>

using namespace pixelroot32::graphics;

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// DisplayConfig Construction Tests
// ============================================================================

void test_display_config_none_type(void) {
    // NONE type should not throw or fail in native_test
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    TEST_ASSERT_EQUAL(DisplayType::NONE, config.type);
    TEST_ASSERT_EQUAL(0, config.rotation);
    TEST_ASSERT_EQUAL(240, config.physicalWidth);
    TEST_ASSERT_EQUAL(240, config.physicalHeight);
}

void test_display_config_with_scaling(void) {
    // Physical 240x240, Logical 120x120 (2x scaling)
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 120, 120);
    TEST_ASSERT_EQUAL(240, config.physicalWidth);
    TEST_ASSERT_EQUAL(240, config.physicalHeight);
    TEST_ASSERT_EQUAL(120, config.logicalWidth);
    TEST_ASSERT_EQUAL(120, config.logicalHeight);
}

void test_display_config_auto_logical_resolution(void) {
    // When logical is 0, it should default to physical
    DisplayConfig config(DisplayType::NONE, 0, 128, 64, 0, 0);
    TEST_ASSERT_EQUAL(128, config.logicalWidth);
    TEST_ASSERT_EQUAL(64, config.logicalHeight);
}

void test_display_config_with_offsets(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 240, 240, 10, 20);
    TEST_ASSERT_EQUAL(10, config.xOffset);
    TEST_ASSERT_EQUAL(20, config.yOffset);
}

void test_display_config_with_rotation(void) {
    DisplayConfig config(DisplayType::NONE, 90, 240, 240);
    TEST_ASSERT_EQUAL(90, config.rotation);
}

void test_display_config_st7789_type(void) {
    DisplayConfig config(DisplayType::ST7789, 0, 240, 240);
    TEST_ASSERT_EQUAL(DisplayType::ST7789, config.type);
}

void test_display_config_st7735_type(void) {
    DisplayConfig config(DisplayType::ST7735, 0, 128, 128);
    TEST_ASSERT_EQUAL(DisplayType::ST7735, config.type);
}

// ============================================================================
// DisplayConfig Pin Configuration Tests
// ============================================================================

void test_display_config_pin_configuration(void) {
    DisplayConfig config(DisplayType::OLED_SSD1306, 0, 
                         22,  // clk
                         21,  // data
                         255, // cs (not used)
                         255, // dc (not used)
                         16,  // rst
                         128, 64);
    
    TEST_ASSERT_EQUAL(22, config.clockPin);
    TEST_ASSERT_EQUAL(21, config.dataPin);
    TEST_ASSERT_EQUAL(255, config.csPin);
    TEST_ASSERT_EQUAL(255, config.dcPin);
    TEST_ASSERT_EQUAL(16, config.resetPin);
}

void test_display_config_hardware_i2c_flag(void) {
    DisplayConfig config(DisplayType::OLED_SSD1306, 0,
                         22, 21, 255, 255, 16,
                         128, 64, 0, 0, 0, 0, true);
    TEST_ASSERT_TRUE(config.useHardwareI2C);
}

// ============================================================================
// Scaling Helper Tests
// ============================================================================

void test_display_config_needs_scaling_true(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 120, 120);
    TEST_ASSERT_TRUE(config.needsScaling());
}

void test_display_config_needs_scaling_false(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 240, 240);
    TEST_ASSERT_FALSE(config.needsScaling());
}

void test_display_config_scale_x(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 120, 120);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, config.getScaleX());
}

void test_display_config_scale_y(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 120, 60);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, config.getScaleY());
}

void test_display_config_no_scaling_factor(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 240, 240);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, config.getScaleX());
    TEST_ASSERT_EQUAL_FLOAT(1.0f, config.getScaleY());
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

void test_display_config_width_alias(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 120, 120);
    TEST_ASSERT_EQUAL(120, config.width());
}

void test_display_config_height_alias(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240, 120, 80);
    TEST_ASSERT_EQUAL(80, config.height());
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

void test_display_config_move_constructor(void) {
    DisplayConfig original(DisplayType::ST7789, 0, 240, 240);
    DisplayConfig moved(std::move(original));
    
    TEST_ASSERT_EQUAL(DisplayType::ST7789, moved.type);
    TEST_ASSERT_EQUAL(240, moved.physicalWidth);
}

void test_display_config_move_assignment(void) {
    DisplayConfig original(DisplayType::ST7789, 0, 240, 240);
    DisplayConfig target(DisplayType::NONE, 0, 0, 0);
    
    target = std::move(original);
    
    TEST_ASSERT_EQUAL(DisplayType::ST7789, target.type);
    TEST_ASSERT_EQUAL(240, target.physicalWidth);
}

// ============================================================================
// Copy Semantics Tests
// ============================================================================

void test_display_config_copy_constructor(void) {
    DisplayConfig original(DisplayType::ST7735, 90, 128, 128, 64, 64, 5, 10);
    DisplayConfig copy(original);
    
    TEST_ASSERT_EQUAL(original.type, copy.type);
    TEST_ASSERT_EQUAL(original.rotation, copy.rotation);
    TEST_ASSERT_EQUAL(original.physicalWidth, copy.physicalWidth);
    TEST_ASSERT_EQUAL(original.physicalHeight, copy.physicalHeight);
    TEST_ASSERT_EQUAL(original.logicalWidth, copy.logicalWidth);
    TEST_ASSERT_EQUAL(original.logicalHeight, copy.logicalHeight);
    TEST_ASSERT_EQUAL(original.xOffset, copy.xOffset);
    TEST_ASSERT_EQUAL(original.yOffset, copy.yOffset);
    TEST_ASSERT_EQUAL(original.clockPin, copy.clockPin);
    TEST_ASSERT_EQUAL(original.useHardwareI2C, copy.useHardwareI2C);
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_display_config_zero_resolution(void) {
    // Zero resolution should be handled gracefully
    DisplayConfig config(DisplayType::NONE, 0, 0, 0, 0, 0);
    // Logical should default to physical (0)
    TEST_ASSERT_EQUAL(0, config.logicalWidth);
    TEST_ASSERT_EQUAL(0, config.logicalHeight);
}

void test_display_config_unusual_rotation(void) {
    // Rotation values other than standard 0/90/180/270
    DisplayConfig config(DisplayType::NONE, 45, 240, 240);
    TEST_ASSERT_EQUAL(45, config.rotation);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Construction Tests
    RUN_TEST(test_display_config_none_type);
    RUN_TEST(test_display_config_with_scaling);
    RUN_TEST(test_display_config_auto_logical_resolution);
    RUN_TEST(test_display_config_with_offsets);
    RUN_TEST(test_display_config_with_rotation);
    RUN_TEST(test_display_config_st7789_type);
    RUN_TEST(test_display_config_st7735_type);
    
    // Pin Configuration
    RUN_TEST(test_display_config_pin_configuration);
    RUN_TEST(test_display_config_hardware_i2c_flag);
    
    // Scaling Helpers
    RUN_TEST(test_display_config_needs_scaling_true);
    RUN_TEST(test_display_config_needs_scaling_false);
    RUN_TEST(test_display_config_scale_x);
    RUN_TEST(test_display_config_scale_y);
    RUN_TEST(test_display_config_no_scaling_factor);
    
    // Backward Compatibility
    RUN_TEST(test_display_config_width_alias);
    RUN_TEST(test_display_config_height_alias);
    
    // Move Semantics
    RUN_TEST(test_display_config_move_constructor);
    RUN_TEST(test_display_config_move_assignment);
    
    // Copy Semantics
    RUN_TEST(test_display_config_copy_constructor);
    
    // Edge Cases
    RUN_TEST(test_display_config_zero_resolution);
    RUN_TEST(test_display_config_unusual_rotation);
    
    return UNITY_END();
}
