#ifdef PLATFORM_ESP32DEV

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <drivers/esp32/ESP32_I2S_AudioBackend.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "RoomScreenScene.h"

namespace pr32 = pixelroot32;

// Audio Pin Configuration (I2S)
const int I2S_BCLK = 26;
const int I2S_LRCK = 25;
const int I2S_DOUT = 22;

// Button Mapping (Arduino ESP32)
const int BTN_UP = 32;
const int BTN_DOWN = 27;
const int BTN_LEFT = 33;
const int BTN_RIGHT = 14;
const int BTN_A = 13;
const int BTN_B = 12;

pr32::drivers::esp32::ESP32_I2S_AudioBackend audioBackend(I2S_BCLK, I2S_LRCK, I2S_DOUT, 22050);

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

pr32::input::InputConfig inputConfig(BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B);

pr32::audio::AudioConfig audioConfig(&audioBackend, audioBackend.getSampleRate());

pr32::core::Engine engine(config, inputConfig, audioConfig);

room_screen::RoomScreenScene roomScreenScene;

void setup() {
    engine.init();
    engine.setScene(&roomScreenScene);
}

void loop() {
    engine.run();
}

#endif
