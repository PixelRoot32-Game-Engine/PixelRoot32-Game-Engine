/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * XPT2046Adapter.cpp - SPI implementation
 * Hardware-specific touch reading for XPT2046 controller
 */
#include "input/adapters/XPT2046Adapter.h"
#include "platforms/PlatformDefaults.h"
#include "core/Log.h"

#if defined(TOUCH_DRIVER_XPT2046) && defined(ARDUINO_ARCH_ESP32) && defined(PIXELROOT32_USE_TFT_ESPI_DRIVER) && \
    !defined(XPT2046_USE_GPIO_SPI)
#include "drivers/esp32/TFT_eSPI_TouchBridge.h"
#endif

#if defined(PLATFORM_ESP32) || defined(ARDUINO_ARCH_ESP32)
    #include <driver/spi_master.h>
    #include <driver/gpio.h>
#endif

#if defined(XPT2046_USE_GPIO_SPI) && defined(ARDUINO_ARCH_ESP32)
// ESP32-2432S028R (Sunton): XPT2046 on a separate bit-banged bus — see vendor Demo 5_2_XPT2046_Touch_Test (xpt2046.h).
#ifndef XPT2046_GPIO_IRQ
#define XPT2046_GPIO_IRQ 36
#endif
#ifndef XPT2046_GPIO_MOSI
#define XPT2046_GPIO_MOSI 32
#endif
#ifndef XPT2046_GPIO_MISO
#define XPT2046_GPIO_MISO 39
#endif
#ifndef XPT2046_GPIO_CLK
#define XPT2046_GPIO_CLK 25
#endif
#ifndef XPT2046_GPIO_CS
#define XPT2046_GPIO_CS 33
#endif
#ifndef XPT2046_GPIO_MEDIAN_SAMPLES
#define XPT2046_GPIO_MEDIAN_SAMPLES 9
#endif
#ifndef XPT2046_GPIO_VENDOR_COORDS
#define XPT2046_GPIO_VENDOR_COORDS 0
#endif
#ifndef XPT2046_GPIO_SWAP_AXES
#define XPT2046_GPIO_SWAP_AXES 0
#endif
#ifndef XPT2046_GPIO_MIRROR_X
#define XPT2046_GPIO_MIRROR_X 0
#endif
#endif

namespace pixelroot32::input {

#if defined(XPT2046_USE_GPIO_SPI) && defined(ARDUINO_ARCH_ESP32)

namespace {

void gpioSpiWriteByte(uint8_t num) {
    for (uint8_t count = 0; count < 8; count++) {
        digitalWrite(XPT2046_GPIO_MOSI, (num & 0x80) ? HIGH : LOW);
        num <<= 1;
        digitalWrite(XPT2046_GPIO_CLK, LOW);
        digitalWrite(XPT2046_GPIO_CLK, HIGH);
    }
}

uint16_t gpioSpiReadReg(uint8_t reg) {
    uint16_t adValue = 0;
    digitalWrite(XPT2046_GPIO_CLK, LOW);
    digitalWrite(XPT2046_GPIO_MOSI, LOW);
    digitalWrite(XPT2046_GPIO_CS, LOW);
    gpioSpiWriteByte(reg);
    delayMicroseconds(6);
    digitalWrite(XPT2046_GPIO_CLK, LOW);
    delayMicroseconds(1);
    digitalWrite(XPT2046_GPIO_CLK, HIGH);
    digitalWrite(XPT2046_GPIO_CLK, LOW);
    for (uint8_t count = 0; count < 16; count++) {
        adValue <<= 1;
        digitalWrite(XPT2046_GPIO_CLK, LOW);
        digitalWrite(XPT2046_GPIO_CLK, HIGH);
        if (digitalRead(XPT2046_GPIO_MISO) != 0) {
            adValue++;
        }
    }
    adValue >>= 4;
    digitalWrite(XPT2046_GPIO_CS, HIGH);
    return adValue;
}

#if defined(XPT2046_GPIO_USE_RAW_RANGE) && (XPT2046_GPIO_USE_RAW_RANGE != 0)
#ifndef XPT2046_RAW_X_LO
#define XPT2046_RAW_X_LO 300
#endif
#ifndef XPT2046_RAW_X_HI
#define XPT2046_RAW_X_HI 3800
#endif
#ifndef XPT2046_RAW_Y_LO
#define XPT2046_RAW_Y_LO 300
#endif
#ifndef XPT2046_RAW_Y_HI
#define XPT2046_RAW_Y_HI 3800
#endif

static int16_t mapRawToScreen16(uint16_t v, int32_t lo, int32_t hi, int16_t outMin, int16_t outMax) {
    if (hi <= lo) {
        return outMin;
    }
    const int64_t num =
        (static_cast<int64_t>(v) - lo) * static_cast<int64_t>(outMax - outMin);
    int64_t x = num / (hi - lo) + outMin;
    if (x < outMin) {
        return outMin;
    }
    if (x > outMax) {
        return outMax;
    }
    return static_cast<int16_t>(x);
}
#endif

uint16_t readMedianAxis(uint8_t cmd) {
    constexpr uint8_t kN = XPT2046_GPIO_MEDIAN_SAMPLES;
    uint16_t buf[kN];
    for (uint8_t i = 0; i < kN; i++) {
        buf[i] = gpioSpiReadReg(cmd);
    }
    for (uint8_t i = 0; i < kN - 1; i++) {
        for (uint8_t j = i + 1; j < kN; j++) {
            if (buf[i] > buf[j]) {
                uint16_t t = buf[i];
                buf[i] = buf[j];
                buf[j] = t;
            }
        }
    }
    return buf[kN / 2];
}

} // namespace

static bool readImplGpioSpi(TouchPoint* points, uint8_t& count) {
    count = 0;
    if (!XPT2046Adapter::isInitialized || points == nullptr) {
        return false;
    }
    if (digitalRead(XPT2046_GPIO_IRQ) != 0) {
        return true;
    }

    constexpr uint8_t kCmdXRead = 0x90;
    constexpr uint8_t kCmdYRead = 0xD0;
    uint16_t ax = readMedianAxis(kCmdXRead);
    uint16_t ay = readMedianAxis(kCmdYRead);
#if defined(XPT2046_GPIO_SWAP_AXES) && (XPT2046_GPIO_SWAP_AXES != 0)
    const uint16_t tmpAxis = ax;
    ax = ay;
    ay = tmpAxis;
#endif

#if defined(PIXELROOT32_DEBUG_MODE) && (PIXELROOT32_DEBUG_MODE != 0)
    {
        static uint32_t s_gpioRawLogMs = 0;
        const uint32_t tlog = ::millis();
        if (tlog - s_gpioRawLogMs >= 350u) {
            s_gpioRawLogMs = tlog;
            pixelroot32::core::logging::log("[XPT2046] raw after swap ax=%u ay=%u", static_cast<unsigned>(ax),
                static_cast<unsigned>(ay));
        }
    }
#endif

    const uint32_t currentTime = ::millis();
    TouchPoint tp;
#if defined(XPT2046_GPIO_USE_RAW_RANGE) && (XPT2046_GPIO_USE_RAW_RANGE != 0)
    const uint16_t ru = ax;
    const uint16_t rv = ay;
    const auto& cal0 = XPT2046Adapter::calibration;
    const int16_t sx =
        mapRawToScreen16(ru, XPT2046_RAW_X_LO, XPT2046_RAW_X_HI, 0, cal0.displayWidth);
    const int16_t sy =
        mapRawToScreen16(rv, XPT2046_RAW_Y_LO, XPT2046_RAW_Y_HI, 0, cal0.displayHeight);
    int16_t rx = 0;
    int16_t ry = 0;
    cal0.applyRotation(sx, sy, rx, ry);
    tp = TouchPoint(rx, ry, true, 0, currentTime);
#else
    tp = XPT2046Adapter::calibration.transform(
        static_cast<int16_t>(ax),
        static_cast<int16_t>(ay),
        true,
        0,
        currentTime);
#endif

#if XPT2046_GPIO_VENDOR_COORDS
    {
        const int16_t fx = tp.x;
        const int16_t gy = tp.y;
        tp.x = static_cast<int16_t>(XPT2046Adapter::calibration.displayWidth - gy);
        tp.y = fx;
    }
#endif

#if defined(XPT2046_GPIO_MIRROR_X) && (XPT2046_GPIO_MIRROR_X != 0)
    {
        const auto& c = XPT2046Adapter::calibration;
        tp.x = static_cast<int16_t>(c.displayWidth - tp.x);
    }
#endif
#if defined(XPT2046_CAL_OFFSET_X)
    tp.x = static_cast<int16_t>(tp.x + XPT2046_CAL_OFFSET_X);
#endif
#if defined(XPT2046_CAL_OFFSET_Y)
    tp.y = static_cast<int16_t>(tp.y + XPT2046_CAL_OFFSET_Y);
#endif
    {
        const auto& c = XPT2046Adapter::calibration;
        if (tp.x < 0) {
            tp.x = 0;
        }
        if (tp.x > c.displayWidth) {
            tp.x = c.displayWidth;
        }
        if (tp.y < 0) {
            tp.y = 0;
        }
        if (tp.y > c.displayHeight) {
            tp.y = c.displayHeight;
        }
    }

    points[0] = tp;
    count = 1;
    return true;
}

#endif // XPT2046_USE_GPIO_SPI && ARDUINO_ARCH_ESP32

// Static member initialization
TouchCalibration XPT2046Adapter::calibration;
bool XPT2046Adapter::isInitialized = false;
bool XPT2046Adapter::wasPressed = false;
uint32_t XPT2046Adapter::lastPressTime = 0;
uint32_t XPT2046Adapter::lastReadTime = 0;
int16_t XPT2046Adapter::medianX[filtering::MEDIAN_WINDOW];
int16_t XPT2046Adapter::medianY[filtering::MEDIAN_WINDOW];
uint8_t XPT2046Adapter::medianIndex = 0;

// Platform-specific SPI read implementation
#if defined(PLATFORM_ESP32) || defined(ARDUINO_ARCH_ESP32)

static spi_device_handle_t xpt2046_spi_handle = nullptr;

bool XPT2046Adapter::initImpl() {
#if defined(XPT2046_USE_GPIO_SPI) && defined(ARDUINO_ARCH_ESP32)
    pinMode(XPT2046_GPIO_MOSI, OUTPUT);
    pinMode(XPT2046_GPIO_CLK, OUTPUT);
    pinMode(XPT2046_GPIO_CS, OUTPUT);
    pinMode(XPT2046_GPIO_MISO, INPUT);
    pinMode(XPT2046_GPIO_IRQ, INPUT);
    digitalWrite(XPT2046_GPIO_CS, HIGH);
    digitalWrite(XPT2046_GPIO_CLK, LOW);
    pixelroot32::core::logging::log(
        "[XPT] GPIO-SPI XPT2046 (2432S028R): MOSI=%d MISO=%d CLK=%d CS=%d IRQ=%d",
        XPT2046_GPIO_MOSI, XPT2046_GPIO_MISO, XPT2046_GPIO_CLK, XPT2046_GPIO_CS, XPT2046_GPIO_IRQ);
#else
    spi_bus_config_t bus_config = {
        .mosi_io_num = -1,
        .sclk_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0,
        .intr_flags = 0
    };
    (void)bus_config;
#endif

    isInitialized = true;
    wasPressed = false;
    lastPressTime = 0;
    lastReadTime = 0;
    medianIndex = 0;
    
    // Initialize median buffers
    for (uint8_t i = 0; i < filtering::MEDIAN_WINDOW; i++) {
        medianX[i] = 2048;  // Middle of ADC range
        medianY[i] = 2048;
    }
    
    return true;
}

int16_t XPT2046Adapter::readRawX() {
    if (!isInitialized) return 0;
    
    // Read X position - command: 0x90 (start + X axis)
    // 12-bit ADC, msb first
    uint8_t tx_buf[2] = { REG_X | 0x80, 0x00 };
    uint8_t rx_buf[2] = { 0, 0 };
    
    // In shared SPI scenario, this would need display DMA coordination
    // Using simplified blocking read for now
    // Actual implementation: spi_device_transmit() or equivalent
    
    // Placeholder: return middle of range
    return 2048;
}

int16_t XPT2046Adapter::readRawY() {
    if (!isInitialized) return 0;
    
    // Read Y position - command: 0xD0 (start + Y axis)
    // Placeholder
    return 2048;
}

uint16_t XPT2046Adapter::readPressure() {
    if (!isInitialized) return 0;
    
    // Read pressure Z1 and Z2, calculate resistance
    // For simplicity, return a value indicating touch
    return 100;  // Above threshold
}

#elif defined(PLATFORM_NATIVE)

// Native platform - use SDL mouse events as touch input
// This allows testing without hardware

bool XPT2046Adapter::initImpl() {
    isInitialized = true;
    wasPressed = false;
    lastPressTime = 0;
    lastReadTime = 0;
    medianIndex = 0;
    
    for (uint8_t i = 0; i < filtering::MEDIAN_WINDOW; i++) {
        medianX[i] = 2048;
        medianY[i] = 2048;
    }
    
    return true;
}

int16_t XPT2046Adapter::readRawX() {
    // On native, this would be mapped from SDL mouse position
    // Placeholder returning scaled display coordinate
    return 2048;
}

int16_t XPT2046Adapter::readRawY() {
    return 2048;
}

uint16_t XPT2046Adapter::readPressure() {
    return 100;
}

#else

// Other platforms - stub
bool XPT2046Adapter::initImpl() {
    isInitialized = true;
    return true;
}

int16_t XPT2046Adapter::readRawX() { return 0; }
int16_t XPT2046Adapter::readRawY() { return 0; }
uint16_t XPT2046Adapter::readPressure() { return 0; }

#endif

bool XPT2046Adapter::readImpl(TouchPoint* points, uint8_t& count) {
    count = 0;

#if defined(XPT2046_USE_GPIO_SPI) && defined(ARDUINO_ARCH_ESP32) && defined(TOUCH_DRIVER_XPT2046)
    static bool sLoggedGpio = false;
    if (!sLoggedGpio) {
        pixelroot32::core::logging::log("[XPT] using GPIO bit-bang XPT2046 (not TFT_eSPI bus)");
        sLoggedGpio = true;
    }
    return readImplGpioSpi(points, count);
#endif

#if defined(TOUCH_DRIVER_XPT2046) && defined(ARDUINO_ARCH_ESP32) && defined(PIXELROOT32_USE_TFT_ESPI_DRIVER) && \
    !defined(XPT2046_USE_GPIO_SPI)
    static bool sLoggedBridgePath = false;
    if (!sLoggedBridgePath) {
        pixelroot32::core::logging::log("[XPT] bridge path COMPILED, hasTft=%d",
            pixelroot32::drivers::esp32::touchBridgeHasTft() ? 1 : 0);
        sLoggedBridgePath = true;
    }
    if (pixelroot32::drivers::esp32::touchBridgeHasTft()) {
        pixelroot32::drivers::esp32::readTouchFromTftEspi(points, count);
        return true;
    }
#elif !defined(XPT2046_USE_GPIO_SPI)
    static bool sLoggedFallback = false;
    if (!sLoggedFallback) {
        pixelroot32::core::logging::log("[XPT] bridge path NOT compiled — using raw SPI stubs");
        sLoggedFallback = true;
    }
#endif

    if (!isInitialized) {
        return false;
    }

    // SPI bus coordination - wait for DMA if display is using SPI
    if (!waitForDMADone()) {
        // DMA active - skip this read, return last state
        return true;
    }

    // Read raw ADC values from XPT2046
    int16_t rawX = readRawX();
    int16_t rawY = readRawY();
    uint16_t pressure = readPressure();

    uint32_t currentTime = millis();

    // Filter: Pressure threshold
    if (pressure < filtering::PRESSURE_THRESHOLD) {
        // No touch detected
        if (wasPressed) {
            // Touch released - apply debounce
            if ((currentTime - lastPressTime) >= filtering::DEBOUNCE_MS) {
                wasPressed = false;
            }
        }
        return true;
    }

    // Apply median filter
    rawX = medianFilter(rawX, true);
    rawY = medianFilter(rawY, false);

    // Filter: Debounce - ignore rapid state changes
    if (!wasPressed) {
        // New press - check debounce
        if ((currentTime - lastPressTime) < filtering::DEBOUNCE_MS) {
            return true;  // Ignore - debouncing
        }
        wasPressed = true;
    }

    lastPressTime = currentTime;

    // Apply calibration and normalization
    TouchPoint tp = calibration.transform(rawX, rawY, true, 0, currentTime);

    points[0] = tp;
    count = 1;

    return true;
}

void XPT2046Adapter::setCalibrationImpl(const TouchCalibration& calib) {
    calibration = calib;
}

bool XPT2046Adapter::isConnectedImpl() {
    // Read XPT2046 ID register - should return 0x94
    // For now, assume connected if init succeeded
    return isInitialized;
}

int16_t XPT2046Adapter::medianFilter(int16_t value, bool isX) {
    int16_t* buffer = isX ? medianX : medianY;

    // Add new value to buffer
    buffer[medianIndex] = value;
    medianIndex = (medianIndex + 1) % filtering::MEDIAN_WINDOW;

    // Copy buffer for sorting
    int16_t sorted[filtering::MEDIAN_WINDOW];
    for (uint8_t i = 0; i < filtering::MEDIAN_WINDOW; i++) {
        sorted[i] = buffer[i];
    }

    // Bubble sort (small array, acceptable)
    for (uint8_t i = 0; i < filtering::MEDIAN_WINDOW - 1; i++) {
        for (uint8_t j = 0; j < filtering::MEDIAN_WINDOW - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                int16_t temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }

    // Return median (middle element)
    return sorted[filtering::MEDIAN_WINDOW / 2];
}

bool XPT2046Adapter::waitForDMADone() {
    // Platform-specific implementation
    // ESP32: Check display DMA status
    // Native: Always return true (no DMA)
#if defined(PLATFORM_ESP32) || defined(ARDUINO_ARCH_ESP32)
    // TODO: Check display DMA flag
    // For now, assume bus is available
    return true;
#else
    return true;
#endif
}

uint32_t XPT2046Adapter::millis() {
#if defined(PLATFORM_ESP32) || defined(ARDUINO_ARCH_ESP32)
    return (uint32_t)esp_timer_get_time() / 1000;
#else
    // Native: SDL_GetTicks() or similar
    return 0;
#endif
}

} // namespace pixelroot32::input