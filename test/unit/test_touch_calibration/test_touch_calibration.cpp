/**
 * @file test_touch_calibration.cpp
 * @brief Unit tests for input/TouchCalibration module
 * @version 1.0
 * @date 2026-04-05
 * 
 * Tests for TouchCalibration - touch coordinate transformation and calibration.
 */

#include <unity.h>
#include "../test_config.h"
#include "input/TouchAdapter.h"
#include "input/TouchPoint.h"

using namespace pixelroot32::input;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_touch_calibration_default_constructor(void) {
    TouchCalibration calib;
    
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleX);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetY);
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation0), 
                            static_cast<uint8_t>(calib.rotation));
}

void test_touch_calibration_for_resolution_320x240(void) {
    TouchCalibration calib = TouchCalibration::forResolution(320, 240);
    
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(320.0f / 4096.0f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(240.0f / 4096.0f, calib.scaleY);
}

void test_touch_calibration_for_resolution_240x320(void) {
    TouchCalibration calib = TouchCalibration::forResolution(240, 320);
    
    TEST_ASSERT_EQUAL_INT16(240, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(320, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(240.0f / 4096.0f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(320.0f / 4096.0f, calib.scaleY);
}

void test_touch_calibration_for_resolution_zero_dimensions(void) {
    TouchCalibration calib = TouchCalibration::forResolution(0, 0);
    
    TEST_ASSERT_EQUAL_INT16(0, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(0, calib.displayHeight);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleX);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleY);
}

void test_touch_calibration_for_resolution_negative_dimensions(void) {
    TouchCalibration calib = TouchCalibration::forResolution(-100, -200);
    
    TEST_ASSERT_EQUAL_INT16(-100, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(-200, calib.displayHeight);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleX);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleY);
}

void test_touch_calibration_transform_no_scale_no_offset(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    calib.offsetX = 0;
    calib.offsetY = 0;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchPoint result = calib.transform(100, 50, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(100, result.x);
    TEST_ASSERT_EQUAL_INT16(50, result.y);
    TEST_ASSERT_TRUE(result.pressed);
    TEST_ASSERT_EQUAL_UINT8(0, result.id);
    TEST_ASSERT_EQUAL_UINT32(1000, result.ts);
}

void test_touch_calibration_transform_with_scale(void) {
    TouchCalibration calib;
    calib.scaleX = 0.5f;
    calib.scaleY = 0.5f;
    calib.offsetX = 0;
    calib.offsetY = 0;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchPoint result = calib.transform(200, 100, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(100, result.x);
    TEST_ASSERT_EQUAL_INT16(50, result.y);
}

void test_touch_calibration_transform_with_offset(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    calib.offsetX = 10;
    calib.offsetY = -5;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchPoint result = calib.transform(100, 50, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(110, result.x);
    TEST_ASSERT_EQUAL_INT16(45, result.y);
}

void test_touch_calibration_transform_combined_scale_and_offset(void) {
    TouchCalibration calib;
    calib.scaleX = 2.0f;
    calib.scaleY = 0.5f;
    calib.offsetX = -10;
    calib.offsetY = 20;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchPoint result = calib.transform(100, 100, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(190, result.x);  // (100 * 2.0) + (-10) = 190
    TEST_ASSERT_EQUAL_INT16(70, result.y);  // (100 * 0.5) + 20 = 70
}

void test_touch_calibration_transform_not_pressed(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    
    TouchPoint result = calib.transform(100, 50, false, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(100, result.x);
    TEST_ASSERT_EQUAL_INT16(50, result.y);
    TEST_ASSERT_FALSE(result.pressed);
}

void test_touch_calibration_transform_clamp_negative_to_zero(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    calib.offsetX = -50;
    calib.offsetY = -50;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchPoint result = calib.transform(10, 10, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(0, result.x);
    TEST_ASSERT_EQUAL_INT16(0, result.y);
}

void test_touch_calibration_transform_clamp_positive_to_max(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    calib.offsetX = 50;
    calib.offsetY = 50;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchPoint result = calib.transform(300, 200, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(320, result.x);
    TEST_ASSERT_EQUAL_INT16(240, result.y);
}

void test_touch_calibration_apply_rotation_0(void) {
    TouchCalibration calib;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation0;
    
    int16_t outX, outY;
    calib.applyRotation(100, 50, outX, outY);
    
    TEST_ASSERT_EQUAL_INT16(100, outX);
    TEST_ASSERT_EQUAL_INT16(50, outY);
}

void test_touch_calibration_apply_rotation_90(void) {
    TouchCalibration calib;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation90;
    
    int16_t outX, outY;
    calib.applyRotation(100, 50, outX, outY);
    
    TEST_ASSERT_EQUAL_INT16(240 - 50, outX);  // height - y
    TEST_ASSERT_EQUAL_INT16(100, outY);        // x
}

void test_touch_calibration_apply_rotation_180(void) {
    TouchCalibration calib;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation180;
    
    int16_t outX, outY;
    calib.applyRotation(100, 50, outX, outY);
    
    TEST_ASSERT_EQUAL_INT16(320 - 100, outX);  // width - x
    TEST_ASSERT_EQUAL_INT16(240 - 50, outY);   // height - y
}

void test_touch_calibration_apply_rotation_270(void) {
    TouchCalibration calib;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation270;
    
    int16_t outX, outY;
    calib.applyRotation(100, 50, outX, outY);
    
    TEST_ASSERT_EQUAL_INT16(50, outX);          // y
    TEST_ASSERT_EQUAL_INT16(320 - 100, outY);   // width - x
}

void test_touch_calibration_set_rotation(void) {
    TouchCalibration calib;
    
    calib.setRotation(TouchRotation::Rotation90);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation90),
                            static_cast<uint8_t>(calib.rotation));
    
    calib.setRotation(TouchRotation::Rotation180);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation180),
                            static_cast<uint8_t>(calib.rotation));
}

void test_touch_calibration_inverted_rotation_0(void) {
    TouchCalibration calib;
    calib.rotation = TouchRotation::Rotation0;
    
    TouchRotation inv = calib.inverted();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation0),
                            static_cast<uint8_t>(inv));
}

void test_touch_calibration_inverted_rotation_90(void) {
    TouchCalibration calib;
    calib.rotation = TouchRotation::Rotation90;
    
    TouchRotation inv = calib.inverted();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation270),
                            static_cast<uint8_t>(inv));
}

void test_touch_calibration_inverted_rotation_180(void) {
    TouchCalibration calib;
    calib.rotation = TouchRotation::Rotation180;
    
    TouchRotation inv = calib.inverted();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation180),
                            static_cast<uint8_t>(inv));
}

void test_touch_calibration_inverted_rotation_270(void) {
    TouchCalibration calib;
    calib.rotation = TouchRotation::Rotation270;
    
    TouchRotation inv = calib.inverted();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TouchRotation::Rotation90),
                            static_cast<uint8_t>(inv));
}

void test_touch_calibration_preset_ili9341_320x240(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ILI9341_320x240);
    
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(1.05f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(1.10f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(-15, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(-20, calib.offsetY);
}

void test_touch_calibration_preset_st7789_240x320(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7789_240x320);
    
    TEST_ASSERT_EQUAL_INT16(240, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(320, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(0.9f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(0.9f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetY);
}

void test_touch_calibration_preset_st7789_240x240(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7789_240x240);
    
    TEST_ASSERT_EQUAL_INT16(240, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(0.88f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(0.88f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(10, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(10, calib.offsetY);
}

void test_touch_calibration_preset_st7735_128x160(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7735_128x160);
    
    TEST_ASSERT_EQUAL_INT16(128, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(160, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(0.65f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(0.65f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(10, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(25, calib.offsetY);
}

void test_touch_calibration_preset_st7735_128x128(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7735_128x128);
    
    TEST_ASSERT_EQUAL_INT16(128, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(128, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(0.62f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(0.62f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetY);
}

void test_touch_calibration_preset_ili9488_320x480(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ILI9488_320x480);
    
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(480, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(1.08f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(1.08f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetY);
}

void test_touch_calibration_preset_gc9a01_240x240(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::GC9A01_240x240);
    
    TEST_ASSERT_EQUAL_INT16(240, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_FLOAT_EQUAL(0.90f, calib.scaleX);
    TEST_ASSERT_FLOAT_EQUAL(0.90f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetY);
}

void test_touch_calibration_preset_none(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::None);
    
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleX);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleY);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetX);
    TEST_ASSERT_EQUAL_INT16(0, calib.offsetY);
}

void test_touch_calibration_preset_custom(void) {
    TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::Custom);
    
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleX);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleY);
}

void test_touch_calibration_transform_with_rotation_90(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    calib.offsetX = 0;
    calib.offsetY = 0;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation90;
    
    TouchPoint result = calib.transform(50, 100, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(240 - 100, result.x);  // height - y = 140
    TEST_ASSERT_EQUAL_INT16(50, result.y);         // x = 50
}

void test_touch_calibration_transform_with_rotation_180(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    calib.offsetX = 0;
    calib.offsetY = 0;
    calib.displayWidth = 320;
    calib.displayHeight = 240;
    calib.rotation = TouchRotation::Rotation180;
    
    TouchPoint result = calib.transform(50, 100, true, 0, 1000);
    
    TEST_ASSERT_EQUAL_INT16(320 - 50, result.x);  // width - x = 270
    TEST_ASSERT_EQUAL_INT16(240 - 100, result.y); // height - y = 140
}

void test_touch_calibration_transform_touch_id_preserved(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    
    TouchPoint result = calib.transform(100, 50, true, 5, 1000);
    
    TEST_ASSERT_EQUAL_UINT8(5, result.id);
}

void test_touch_calibration_transform_timestamp_preserved(void) {
    TouchCalibration calib;
    calib.scaleX = 1.0f;
    calib.scaleY = 1.0f;
    
    TouchPoint result = calib.transform(100, 50, true, 0, 12345678);
    
    TEST_ASSERT_EQUAL_UINT32(12345678, result.ts);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_touch_calibration_default_constructor);
    RUN_TEST(test_touch_calibration_for_resolution_320x240);
    RUN_TEST(test_touch_calibration_for_resolution_240x320);
    RUN_TEST(test_touch_calibration_for_resolution_zero_dimensions);
    RUN_TEST(test_touch_calibration_for_resolution_negative_dimensions);
    
    RUN_TEST(test_touch_calibration_transform_no_scale_no_offset);
    RUN_TEST(test_touch_calibration_transform_with_scale);
    RUN_TEST(test_touch_calibration_transform_with_offset);
    RUN_TEST(test_touch_calibration_transform_combined_scale_and_offset);
    RUN_TEST(test_touch_calibration_transform_not_pressed);
    RUN_TEST(test_touch_calibration_transform_clamp_negative_to_zero);
    RUN_TEST(test_touch_calibration_transform_clamp_positive_to_max);
    RUN_TEST(test_touch_calibration_transform_with_rotation_90);
    RUN_TEST(test_touch_calibration_transform_with_rotation_180);
    RUN_TEST(test_touch_calibration_transform_touch_id_preserved);
    RUN_TEST(test_touch_calibration_transform_timestamp_preserved);
    
    RUN_TEST(test_touch_calibration_apply_rotation_0);
    RUN_TEST(test_touch_calibration_apply_rotation_90);
    RUN_TEST(test_touch_calibration_apply_rotation_180);
    RUN_TEST(test_touch_calibration_apply_rotation_270);
    
    RUN_TEST(test_touch_calibration_set_rotation);
    
    RUN_TEST(test_touch_calibration_inverted_rotation_0);
    RUN_TEST(test_touch_calibration_inverted_rotation_90);
    RUN_TEST(test_touch_calibration_inverted_rotation_180);
    RUN_TEST(test_touch_calibration_inverted_rotation_270);
    
    RUN_TEST(test_touch_calibration_preset_ili9341_320x240);
    RUN_TEST(test_touch_calibration_preset_st7789_240x320);
    RUN_TEST(test_touch_calibration_preset_st7789_240x240);
    RUN_TEST(test_touch_calibration_preset_st7735_128x160);
    RUN_TEST(test_touch_calibration_preset_st7735_128x128);
    RUN_TEST(test_touch_calibration_preset_ili9488_320x480);
    RUN_TEST(test_touch_calibration_preset_gc9a01_240x240);
    RUN_TEST(test_touch_calibration_preset_none);
    RUN_TEST(test_touch_calibration_preset_custom);
    
    return UNITY_END();
}