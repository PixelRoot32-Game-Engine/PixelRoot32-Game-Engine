#ifdef PLATFORM_ESP32C3

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <drivers/esp32/ESP32_I2S_AudioBackend.h> // Uncomment for I2S (MAX98357A)
#include  <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "MusicDemoSceneC3.h"

namespace pr32 = pixelroot32;

// Audio Pin Configuration (I2S)
// Common mapping for MAX98357A or similar I2S DACs
const int I2S_BCLK = 9; //26
const int I2S_LRCK = 8; //25
const int I2S_DOUT = 10; //22

// Button Mapping (Arduino ESP32)
const int BTN_PIN_UP = 3;

// OLED Pin Configuration for Beetle ESP32-C3
// SCL = 6, SDA = 5
const uint8_t OLED_SDA = 5;
const uint8_t OLED_SCL = 6;
const uint8_t OLED_RST = 255; // No reset pin

// Select Audio Backend
// Option: I2S (High quality, external DAC like MAX98357A)
pr32::drivers::esp32::ESP32_I2S_AudioBackend audioBackend(I2S_BCLK, I2S_LRCK, I2S_DOUT, 11025);

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

pr32::audio::AudioConfig audioConfig(&audioBackend, audioBackend.getSampleRate());

pr32::core::Engine engine(config, inputConfig, audioConfig);

// MenuScene menuScene;
musicdemo::MusicDemoSceneC3 musicDemoScene;

void setup() {
  engine.init();
  engine.setScene(&musicDemoScene);
}

void loop() {
    engine.run();
}

#endif