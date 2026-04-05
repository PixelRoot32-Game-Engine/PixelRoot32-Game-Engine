/**
 * @file test_xpt2046_adapter.cpp
 * @brief Unit tests for input/adapters/XPT2046Adapter module
 * @version 1.0
 * @date 2026-04-05
 * 
 * Tests for XPT2046Adapter - SPI touch controller driver.
 * 
 * NOTE: This adapter has heavy hardware dependencies (SPI, GPIO).
 * These tests verify static configuration and constants only.
 * Full functionality requires ESP32 hardware or extensive SPI mocks.
 */

#include <unity.h>
#include "../test_config.h"
#include "input/adapters/XPT2046Adapter.h"

using namespace pixelroot32::input;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_xpt2046_spi_clock_hz(void) {
    TEST_ASSERT_EQUAL(2500000U, XPT2046Adapter::SPI_CLOCK_HZ);
}

void test_xpt2046_chip_select_pin(void) {
    TEST_ASSERT_EQUAL(5, XPT2046Adapter::CHIP_SELECT_PIN);
}

void test_xpt2046_irq_pin(void) {
    TEST_ASSERT_EQUAL(4, XPT2046Adapter::IRQ_PIN);
}

void test_xpt2046_uses_spi(void) {
    TEST_ASSERT_TRUE(XPT2046Adapter::USES_SPI);
}

void test_xpt2046_register_x(void) {
    TEST_ASSERT_EQUAL(0x10, XPT2046Adapter::REG_X);
}

void test_xpt2046_register_y(void) {
    TEST_ASSERT_EQUAL(0x50, XPT2046Adapter::REG_Y);
}

void test_xpt2046_register_z1(void) {
    TEST_ASSERT_EQUAL(0x30, XPT2046Adapter::REG_Z1);
}

void test_xpt2046_register_z2(void) {
    TEST_ASSERT_EQUAL(0x70, XPT2046Adapter::REG_Z2);
}

void test_xpt2046_calibration_default(void) {
    TouchCalibration calib = XPT2046Adapter::calibration;
    
    TEST_ASSERT_EQUAL_INT16(320, calib.displayWidth);
    TEST_ASSERT_EQUAL_INT16(240, calib.displayHeight);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleX);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, calib.scaleY);
}

void test_xpt2046_is_initialized_default(void) {
    TEST_ASSERT_FALSE(XPT2046Adapter::isInitialized);
}

void test_xpt2046_was_pressed_default(void) {
    TEST_ASSERT_FALSE(XPT2046Adapter::wasPressed);
}

void test_xpt2046_last_press_time_default(void) {
    TEST_ASSERT_EQUAL(0U, XPT2046Adapter::lastPressTime);
}

void test_xpt2046_last_read_time_default(void) {
    TEST_ASSERT_EQUAL(0U, XPT2046Adapter::lastReadTime);
}

void test_xpt2046_median_index_default(void) {
    TEST_ASSERT_EQUAL(0, XPT2046Adapter::medianIndex);
}

void test_xpt2046_median_filter_array_size(void) {
    TEST_ASSERT_EQUAL(5U, sizeof(XPT2046Adapter::medianX) / sizeof(int16_t));
    TEST_ASSERT_EQUAL(5U, sizeof(XPT2046Adapter::medianY) / sizeof(int16_t));
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_xpt2046_spi_clock_hz);
    RUN_TEST(test_xpt2046_chip_select_pin);
    RUN_TEST(test_xpt2046_irq_pin);
    RUN_TEST(test_xpt2046_uses_spi);
    RUN_TEST(test_xpt2046_register_x);
    RUN_TEST(test_xpt2046_register_y);
    RUN_TEST(test_xpt2046_register_z1);
    RUN_TEST(test_xpt2046_register_z2);
    RUN_TEST(test_xpt2046_calibration_default);
    RUN_TEST(test_xpt2046_is_initialized_default);
    RUN_TEST(test_xpt2046_was_pressed_default);
    RUN_TEST(test_xpt2046_last_press_time_default);
    RUN_TEST(test_xpt2046_last_read_time_default);
    RUN_TEST(test_xpt2046_median_index_default);
    RUN_TEST(test_xpt2046_median_filter_array_size);
    
    return UNITY_END();
}