#ifdef PLATFORM_ESP32CYD

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <core/Engine.h>
#include <core/Log.h>
#include <platforms/EngineConfig.h>
#include <input/TouchManager.h>
#include <input/TouchAdapter.h>
#include <input/TouchEvent.h>
#include <input/TouchPoint.h>

#include "PhysicsDemoScene.h"

namespace pr32 = pixelroot32;

// ESP32-2432S028 (CYD): ILI9341 240x320 + XPT2046 (TOUCH_CS=33, shared SPI — see TFT_eSPI User_Setup).

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::ILI9341_2,
    DISPLAY_ROTATION,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::core::Engine engine(config);

physicsdemo::PhysicsDemoScene physicsScene;

// TouchManager para leer hardware touch (hardware polling)
pr32::input::TouchManager touchManager(PHYSICAL_DISPLAY_WIDTH, PHYSICAL_DISPLAY_HEIGHT);

static unsigned long touchPrevFrameMs = 0;

void setup() {
    engine.init();
    engine.setScene(&physicsScene);

    // Set 240×320 cal before touchManager.init(): default preset is 320x240 and breaks mapping on first use.
    {
        pr32::input::TouchCalibration cal =
            pr32::input::TouchCalibration::forResolution(PHYSICAL_DISPLAY_WIDTH, PHYSICAL_DISPLAY_HEIGHT);
        touchManager.setCalibration(cal);
    }
    touchManager.init();

    // Register TouchManager with Engine for automatic touch processing
    #if PIXELROOT32_ENABLE_TOUCH
    engine.setTouchManager(&touchManager);
    #endif
}

void loop() {
    unsigned long nowMs = millis();
    unsigned long frameDt = (touchPrevFrameMs == 0) ? 1u : (nowMs - touchPrevFrameMs);
    touchPrevFrameMs = nowMs;

    // Poll hardware touch
    touchManager.update(frameDt);

    // Engine handles touch injection automatically via setTouchManager()
    // No manual touch point injection needed - Engine processes getTouchPoints()
    // and detects releases internally

    engine.run();
}

#endif
