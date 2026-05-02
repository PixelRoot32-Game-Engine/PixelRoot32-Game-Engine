# Flappy Bird Example

A **Flappy Bird**–style game: bird is a **`RigidActor`** (gravity + flap impulse), pipes are **`KinematicActor`** pairs that scroll and **recycle** when off-screen. Score and game states (**waiting / playing / game over**) are handled in [`FlappyBirdScene`](src/FlappyBirdScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_PHYSICS=1`** — required on **`esp32c3`** in `platformio.ini`.
- **`PIXELROOT32_ENABLE_PROFILING`** — enabled on the **`esp32c3`** environment in this project (optional for learning builds).
- **U8g2 path (hardware)**: **`PIXELROOT32_USE_U8G2`**, **`PIXELROOT32_NO_TFT_ESPI`** on **`esp32c3`**.

The **logical framebuffer** is **72×40** pixels, centered in a **128×64** physical OLED via **`X_OFF_SET`**, **`Y_OFF_SET`**, **`LOGICAL_WIDTH`**, **`LOGICAL_HEIGHT`** in `platformio.ini`.

## Platforms

| Environment | Target |
|-------------|--------|
| **`native`** | SDL2 window sized for the same logical resolution (offsets in `platformio.ini`) |
| **`esp32c3`** | **DFRobot Beetle ESP32-C3** (`board = dfrobot_beetle_esp32c3`) with **U8g2** display (no TFT_eSPI on this preset) |

This example does **not** ship an `esp32dev` TFT environment — only **`native`** + **`esp32c3`**.

## Controls

- **Action / Jump** — button **0** (`FlappyBirdConstants` / scene input) to flap when running.
- Avoid pipes and the top/bottom bounds; pass gaps to increase score.

## Features

- **Physics** actors for bird and pipes
- **Object pool–style** pipe reuse
- **Small-resolution** rendering path suited for **128×64 OLED** via U8g2

## Documentation links

- [Physics API](../../docs/api/physics.md)
- [Core API](../../docs/api/core.md)
- [Platform / drivers](../../docs/api/platform.md)

## Build

From **`examples/flappy_bird`**:

```bash
pio run -e native
pio run -e esp32c3
```

## Upload (ESP32-C3)

```bash
pio run -e esp32c3 --target upload
```

Wire your OLED according to the U8g2 configuration used in this project’s platform header / driver setup.
