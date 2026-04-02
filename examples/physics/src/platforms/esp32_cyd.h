#ifdef PLATFORM_ESP32CYD

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <core/Engine.h>
#include <core/Log.h>
#include <platforms/EngineConfig.h>
#include <input/TouchManager.h>
#include <input/TouchAdapter.h>
#include <input/TouchEvent.h>

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

pr32::input::TouchManager touchManager(PHYSICAL_DISPLAY_WIDTH, PHYSICAL_DISPLAY_HEIGHT);

static unsigned long touchPrevFrameMs = 0;

void setup() {
    engine.init();
    engine.setScene(&physicsScene);

    // Set 240×320 cal before touchManager.init(): default preset is 320×240 and breaks mapping on first use.
    {
        pr32::input::TouchCalibration cal =
            pr32::input::TouchCalibration::forResolution(PHYSICAL_DISPLAY_WIDTH, PHYSICAL_DISPLAY_HEIGHT);
        touchManager.setCalibration(cal);
    }
    touchManager.init();
    pr32::core::logging::log("[CYD] setup done (engine + touch %ux%u)", PHYSICAL_DISPLAY_WIDTH,
        PHYSICAL_DISPLAY_HEIGHT);
}

void loop() {
    unsigned long nowMs = millis();
    unsigned long frameDt = (touchPrevFrameMs == 0) ? 1u : (nowMs - touchPrevFrameMs);
    touchPrevFrameMs = nowMs;

    touchManager.update(frameDt);

    pr32::input::TouchEvent touchEvents[pr32::input::TOUCH_EVENT_QUEUE_SIZE];
    const uint8_t evtCount = touchManager.getEvents(touchEvents, pr32::input::TOUCH_EVENT_QUEUE_SIZE);
    if (evtCount > 0) {
#ifdef PIXELROOT32_DEBUG_MODE
        const auto& e0 = touchEvents[0];
        pr32::core::logging::log("[CYD] touch batch n=%u first:type=%u x=%d y=%d c=%d", evtCount,
            static_cast<uint8_t>(e0.type), e0.x, e0.y, e0.isConsumed() ? 1 : 0);
#endif
        auto sceneOpt = engine.getCurrentScene();
        if (sceneOpt.has_value() && sceneOpt.value() != nullptr) {
            sceneOpt.value()->processTouchEvents(touchEvents, evtCount);
        }
    }

    engine.run();
}

#endif
