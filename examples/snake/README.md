# Snake Game Example

A complete Snake game implementation using PixelRoot32.

## Features

- Snake movement with arrow keys
- Food spawning
- Collision detection (walls and self)
- Optional audio feedback (I2S or DAC)
- Score display
- Game over state

## Build

```bash
# Native (PC with SDL2)
pio run -e native

# ESP32
pio run -e esp32dev
```

## Controls

- Arrow keys to move
- Goal: Eat food to grow and score points
- Avoid: Walls and your own tail

---

**Note:** Looking for more examples? Check out the main samples repository:
[https://github.com/pixelroot32/PixelRoot32-Game-Samples](https://github.com/pixelroot32/PixelRoot32-Game-Samples)