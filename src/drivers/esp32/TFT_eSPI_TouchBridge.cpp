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
    pixelroot32::core::logging::log("[TouchBridge] TFT registered: %s", tft ? "OK" : "NULL");
}

bool touchBridgeHasTft() {
    return gTftForTouch != nullptr;
}

void readTouchFromTftEspi(pixelroot32::input::TouchPoint* points, uint8_t& count) {
    count = 0;
    if (points == nullptr || gTftForTouch == nullptr) {
        return;
    }

    if (!gCalibrationApplied) {
        uint16_t calData[5] = { 300, 3600, 300, 3600, 7 };
        gTftForTouch->setTouch(calData);
        gCalibrationApplied = true;
        pixelroot32::core::logging::log("[TouchBridge] calibration applied");
    }

#ifdef ARDUINO_ARCH_ESP32
    const uint32_t ts = millis();
#else
    const uint32_t ts = 0;
#endif

    uint16_t x = 0;
    uint16_t y = 0;
    constexpr uint16_t kTouchThreshold = 300;
    const uint8_t pressed = gTftForTouch->getTouch(&x, &y, kTouchThreshold);

    static uint32_t sLastRawLog = 0;
    if (pressed == 0) {
#ifdef ARDUINO_ARCH_ESP32
        if (ts - sLastRawLog > 2000) {
            uint16_t rx = 0, ry = 0;
            gTftForTouch->getTouchRaw(&rx, &ry);
            uint16_t rz = gTftForTouch->getTouchRawZ();
            pixelroot32::core::logging::log("[TouchBridge] idle rx=%u ry=%u rz=%u", rx, ry, rz);
            sLastRawLog = ts;
        }
#endif
        return;
    }

    pixelroot32::core::logging::log("[TouchBridge] PRESS x=%u y=%u", x, y);

    points[0] = pixelroot32::input::TouchPoint(
        static_cast<int16_t>(x),
        static_cast<int16_t>(y),
        true,
        0,
        ts);
    count = 1;
}

} // namespace pixelroot32::drivers::esp32

#endif
