#ifdef PLATFORM_ESP32C3

#include <Arduino.h>
#include <drivers/esp32/U8G2_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "FlappyBirdScene.h"

namespace pr32 = pixelroot32;

// Button Mapping (Arduino ESP32)
const int BTN_PIN_UP = 3;

// OLED Pin Configuration for Beetle ESP32-C3
// SCL = 6, SDA = 5
const uint8_t OLED_SDA = 5;
const uint8_t OLED_SCL = 6;
const uint8_t OLED_RST = 255; // No reset pin

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::OLED_SSD1306, 
    DISPLAY_ROTATION, 
    OLED_SCL, OLED_SDA, 255, 255, OLED_RST,
    PHYSICAL_DISPLAY_WIDTH, 
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(1, BTN_PIN_UP); // 1 button: Up

pr32::core::Engine engine(config, inputConfig);

// MenuScene menuScene;
flappy::FlappyBirdScene flappyBirdScene;

void setup() {
  engine.init();
  engine.setScene(&flappyBirdScene);
}

void loop() {
    engine.run();
}

#endif