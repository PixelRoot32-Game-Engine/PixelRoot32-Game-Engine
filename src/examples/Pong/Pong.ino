#include <Arduino.h>
#include "EDGE.h"
#include "PongScene.h"

#define I2C_SDA SDA
#define I2C_SCL SCL
#define SCREEN_WIDTH 72
#define SCREEN_HEIGHT 40

// Examples ESP32-C3 with OLED Display
DisplayConfig config(SSD1306, U8G2_R0, I2C_SCL, I2C_SDA, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE, SCREEN_WIDTH, SCREEN_HEIGHT, true);
InputConfig inputConfig(2, 0, 1); // Two button connected to pin 0 and 1

EDGE engine(config, inputConfig);
PongScene scene;

unsigned long currentTime = 0;

void printTask(ulong time);

void setup() {
    Serial.begin(115200);
    engine.init();
    engine.setScene(&scene);

    Serial.println("Setup completed");
}

void loop() {
    currentTime = millis();
    engine.update();
    engine.draw();
}
