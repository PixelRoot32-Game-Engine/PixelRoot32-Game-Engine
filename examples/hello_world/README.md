# Hello World Example

A minimal example that displays "Hello PixelRoot32!" on the screen with button input detection.

## Features

- Text rendering with UILabel
- Button press detection (U/D/L/R/A/B)
- Background color cycling

## Hardware

- Display: 128x128 ST7735 TFT
- ESP32 pins: MOSI=23, SCLK=18, DC=2, RST=4

## Build

```bash
# Native (PC with SDL2)
pio run -e native

# ESP32
pio run -e esp32dev
```

## Upload to ESP32

```bash
pio run -e esp32dev --target upload
```
