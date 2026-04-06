#ifdef PLATFORM_ESP32CYD

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <core/Engine.h>
#include <core/Log.h>
#include <platforms/EngineConfig.h>

#include "AnimatedTilemapScene.h"

namespace pr32 = pixelroot32;

// ESP32-2432S028 (CYD): ILI9341 240x320 + XPT2046 (TOUCH_CS=33, shared SPI — see TFT_eSPI User_Setup).

// Not using touch for this example.

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

animatedtilemap::AnimatedTilemapScene animatedTilemapScene;

void setup() {
    engine.init();
    engine.setScene(&animatedTilemapScene);
}

void loop() {
    engine.run();
}

#endif
