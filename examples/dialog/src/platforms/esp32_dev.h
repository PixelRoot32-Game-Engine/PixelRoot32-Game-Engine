#ifdef PLATFORM_ESP32DEV

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "DialogExampleScene.h"

namespace pr32 = pixelroot32;

// Button mapping (Arduino ESP32): 5-directional pad plus A/B.
const int BTN_UP_PIN = 32;
const int BTN_DOWN_PIN = 27;
const int BTN_LEFT_PIN = 33;
const int BTN_RIGHT_PIN = 14;
const int BTN_A_PIN = 13;
const int BTN_B_PIN = 12;

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::ST7789,
    DISPLAY_ROTATION,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN,
                                      BTN_A_PIN, BTN_B_PIN);

pr32::core::Engine engine(config, inputConfig);

dialogexample::DialogExampleScene scene;

void setup() {
    engine.init();
    engine.setScene(&scene);
}

void loop() {
    engine.run();
}

#endif
