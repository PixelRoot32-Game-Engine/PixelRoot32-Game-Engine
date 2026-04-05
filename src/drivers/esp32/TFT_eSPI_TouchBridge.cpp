/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

#include "drivers/esp32/TFT_eSPI_TouchBridge.h"
#include "core/Log.h"
#include <TFT_eSPI.h>

#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#endif

namespace pixelroot32::drivers::esp32 {

static TFT_eSPI* gTftForTouch = nullptr;
static bool gCalibrationApplied = false;

void registerTftForXpt2046Touch(TFT_eSPI* tft) {
    gTftForTouch = tft;
    gCalibrationApplied = false;
    if (tft == nullptr) {
        pixelroot32::core::logging::log(pixelroot32::core::logging::LogLevel::Warning,
            "[TouchBridge] registerTft: TFT pointer is NULL (touch will not work)");
    }
}

bool touchBridgeHasTft() {
    return gTftForTouch != nullptr;
}

void readTouchFromTftEspi(pixelroot32::input::TouchPoint* points, uint8_t& count) {
    count = 0;
    if (points == nullptr || gTftForTouch == nullptr) {
        return;
    }

#if PIXELROOT32_TOUCH_ENABLED
    // TFT_eSPI touch requires TOUCH_CS to be defined, which sets up setTouch/getTouch methods
    if (!gCalibrationApplied) {
        uint16_t calData[5] = { 300, 3600, 300, 3600, 7 };
        gTftForTouch->setTouch(calData);
        gCalibrationApplied = true;
        pixelroot32::core::logging::log("[TouchBridge] default TFT_eSPI touch cal applied");
    }
#endif

#ifdef ARDUINO_ARCH_ESP32
    const uint32_t ts = millis();
#else
    const uint32_t ts = 0;
#endif

#if PIXELROOT32_TOUCH_ENABLED
    uint16_t x = 0;
    uint16_t y = 0;
    constexpr uint16_t kTouchThreshold = 300;
    const uint8_t pressed = gTftForTouch->getTouch(&x, &y, kTouchThreshold);
    if (pressed == 0) {
        return;
    }

    points[0] = pixelroot32::input::TouchPoint(
        static_cast<int16_t>(x),
        static_cast<int16_t>(y),
        true,
        0,
        ts);
    count = 1;
#endif
}

} // namespace pixelroot32::drivers::esp32

#endif
