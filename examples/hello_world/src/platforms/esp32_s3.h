#ifdef PLATFORM_ESP32S3

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include  <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "HelloWorldScene.h"

namespace pr32 = pixelroot32;

// Button Mapping (Arduino ESP32)
// Common mapping for 5-directional pad and A button
const int BTN_UP = 15;
const int BTN_DOWN = 37;
const int BTN_LEFT = 36;
const int BTN_RIGHT = 35;
const int BTN_A = 1;
const int BTN_B = 2;

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::ST7735, 
    DISPLAY_ROTATION, 
    PHYSICAL_DISPLAY_WIDTH, 
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(6, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B); // 6 buttons: Up, Down, Left, Right, A, B

pr32::core::Engine engine(config, inputConfig);

helloworld::HelloWorldScene helloScene;

void setup() {
    engine.init();
    engine.setScene(&helloScene);
}

void loop() {
    engine.run();
}

#endif
