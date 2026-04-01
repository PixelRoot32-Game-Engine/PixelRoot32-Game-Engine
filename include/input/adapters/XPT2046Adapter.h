/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * XPT2046 Touch Controller Adapter
 * SPI-based touch controller with heavy filtering
 *
 * NOTE: This adapter does significant preprocessing:
 * - Median filter for noise rejection
 * - Debounce logic
 * - Pressure threshold filtering
 * - Coordinate normalization
 */
#pragma once

#ifdef PLATFORM_NATIVE
    #include <platforms/mock/MockArduino.h>
#else
    #include <Arduino.h>
#endif

#include <input/TouchAdapter.h>
#include <input/TouchPoint.h>

namespace pixelroot32::input {

/**
 * @class XPT2046Adapter
 * @brief XPT2046 SPI touch controller driver
 *
 * Hardware: XPT2046 (common on TFT touchscreens)
 * Protocol: SPI — usually shares the TFT bus; ESP32-2432S028R uses a separate GPIO bit-bang bus
 *           (build with -D XPT2046_USE_GPIO_SPI; pins overridable via XPT2046_GPIO_* macros).
 *           Use -D XPT2046_GPIO_SWAP_AXES=1 when finger vertical/horizontal tracks the wrong screen axis.
 *           Use -D XPT2046_GPIO_MIRROR_X=1 when left/right are inverted (horizontal flip in screen space).
 *           GPIO path order: map → vendor swap → MIRROR_X → CAL_OFFSET_* → clamp (offsets = final nudge).
 *           -D XPT2046_DEBUG_RAW_TOUCH=1 logs raw ax/ay (~4 Hz) while pressed for RAW_LO/HI corner calibration.
 * Sampling: Up to 125Hz
 * Filtering: Heavy (median + debounce + pressure threshold)
 *
 * @note SPI bus MUST be coordinated with display DMA
 * @note Requires display-specific calibration
 */
struct XPT2046Adapter {
    // Compile-time constants
    static constexpr uint32_t SPI_CLOCK_HZ = 2500000;    // 2.5 MHz
    static constexpr uint8_t CHIP_SELECT_PIN = 5;       // Default CS pin
    static constexpr uint8_t IRQ_PIN = 4;                // Interrupt pin (optional)
    static constexpr bool USES_SPI = true;               // Flag for bus coordination

    // XPT2046 register addresses
    static constexpr uint8_t REG_X = 0x10;  // Y position (actually X on screen)
    static constexpr uint8_t REG_Y = 0x50;  // X position (actually Y on screen)
    static constexpr uint8_t REG_Z1 = 0x30; // Pressure Z1
    static constexpr uint8_t REG_Z2 = 0x70; // Pressure Z2

    // Filtering state
    static TouchCalibration calibration;
    static bool isInitialized;
    static bool wasPressed;
    static uint32_t lastPressTime;
    static uint32_t lastReadTime;

    // Median filter buffers (fixed-size, no heap)
    static int16_t medianX[filtering::MEDIAN_WINDOW];
    static int16_t medianY[filtering::MEDIAN_WINDOW];
    static uint8_t medianIndex;

    /**
     * @brief Initialize XPT2046 hardware
     * @return true if successful
     */
    static bool initImpl();

    /**
     * @brief Read touch data from XPT2046
     * @param points Output buffer
     * @param count Number of points read
     * @return true if successful
     */
    static bool readImpl(TouchPoint* points, uint8_t& count);

    /**
     * @brief Set calibration parameters
     * @param calib Calibration data
     */
    static void setCalibrationImpl(const TouchCalibration& calib);

    /**
     * @brief Read raw X coordinate from XPT2046
     * @return Raw ADC value (0-4095)
     */
    static int16_t readRawX();

    /**
     * @brief Read raw Y coordinate from XPT2046
     * @return Raw ADC value (0-4095)
     */
    static int16_t readRawY();

    /**
     * @brief Read pressure (Z) from XPT2046
     * @return Pressure value
     */
    static uint16_t readPressure();

    /**
     * @brief Median filter implementation
     * @param value New sample
     * @param isX True for X coordinate
     * @return Filtered value
     */
    static int16_t medianFilter(int16_t value, bool isX);

    /**
     * @brief Check if controller is connected
     * @return true if responding
     */
    static bool isConnectedImpl();

private:
    /**
     * @brief Wait for DMA operation to complete (SPI bus coordination)
     *
     * XPT2046 shares SPI bus with TFT display.
     * Must wait for display DMA to complete before reading touch.
     *
     * @return true if bus is available, false if DMA still active
     */
    static bool waitForDMADone();

    /**
     * @brief Platform-specific millis() function
     */
    static uint32_t millis();
};

} // namespace pixelroot32::input
