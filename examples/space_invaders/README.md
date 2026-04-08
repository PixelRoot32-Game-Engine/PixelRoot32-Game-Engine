# Space Invaders Example

Classic **Space Invaders** arcade game: alien formation with synchronized movement, player ship with shooting mechanics, bunkers for protection, enemy projectiles, score system, and **procedural audio** through the engine **`AudioEngine`**.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_AUDIO=1`** — set in [`lib/platformio.ini`](lib/platformio.ini) `base` template so all environments inherit it.
- **`PIXELROOT32_ENABLE_PHYSICS=1`** — required for projectile collision detection.

Display size is **240×240** in the project `platformio.ini` (see **`PHYSICAL_DISPLAY_*`**).

## Platforms

| Environment | Display | Audio backend |
|-------------|---------|----------------|
| **`native`** | SDL2, 240×240 | **`SDL2_AudioBackend`** in [`src/platforms/native.h`](src/platforms/native.h) |
| **`esp32dev`** | **ST7789** 240×240 | Default: **`ESP32_I2S_AudioBackend`** (comment in `esp32_dev.h` documents optional internal **DAC** backend instead) |

Pin choices for I2S / DAC are in **`src/platforms/esp32_dev.h`** (edit there if your wiring differs).

## Controls

- **Arrow keys** (or GPIO D-pad mapped in your platform input config) to move left/right.
- **Space** to fire (max 4 simultaneous bullets with 150ms cooldown).
- Destroy all aliens to win; if aliens reach the player's level, game over.

## How audio is triggered

[`SpaceInvadersScene.cpp`](src/SpaceInvadersScene.cpp) builds **`pixelroot32::audio::AudioEvent`** values (player shoot, enemy shoot, explosion, BGM tempo changes) and calls **`engine.getAudioEngine().playEvent(...)`**. Wave types (pulse, triangle) are lightweight beeps suited for embedded output.

## Features

- **Scene** with pooled **`ProjectileActor`**, **`AlienActor`**, **`BunkerActor`**, **`PlayerActor`**
- **4-player bullet limit** with fire rate cooldown (150ms)
- **Alien formation** with step-based movement and tempo-based BGM
- **Audio** subsystem integration (`AudioEngine`, platform backends)

## Documentation links

- [Audio API](../../docs/api/API_AUDIO.md)
- [Core API](../../docs/api/API_CORE.md)
- [Input API](../../docs/api/API_INPUT.md)

## Build

From **`examples/space_invaders`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```