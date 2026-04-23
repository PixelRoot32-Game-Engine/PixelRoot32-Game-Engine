# Music Demo Example

Interactive **audio** demo showcasing the **PixelRoot32 audio subsystem** with **instrument presets** and **pre-composed melodies**. Features a **UI-based interface** for testing different sound presets and playing melodic sequences.

When **`PIXELROOT32_ENABLE_UI_SYSTEM`** is on (default in [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h)), the demo exposes **touch UI**: **`UIButton`** for instrument preset selection and melody playback, **`UILabel`** for navigation hints, and **`UIVerticalLayout`** for organized button placement — see [`MusicDemoScene.h`](src/MusicDemoScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_AUDIO`** — required for audio subsystem and music playback
- **`PIXELROOT32_ENABLE_UI_SYSTEM`** — required for UI widgets (buttons, labels, layouts)
- **`PIXELROOT32_ENABLE_TOUCH=1`** — set for **`native`** in `platformio.ini` so touch APIs compile and mouse/touch can drive the demo

See **`platformio.ini`** for **`native`** and **`esp32dev`**.

## Platforms

| Environment | Display / input |
|-------------|-----------------|
| **`native`** | SDL2, 240×240; touch flag enables the same code paths with **simulated** touch |
| **`esp32dev`** | **ST7789** 240×240 (use keyboard/GPIO per your `platforms/esp32_dev.h`) |

## Controls / interaction

- **Navigate menus** — use your **`InputManager`** layout to move between UI buttons (see scene `update` / button handling)
- **Select/Play** — trigger button callbacks to play instrument sounds or melodies
- **Back navigation** — return to previous menu level

## Features

- **10 instrument presets** — Lead Square, Harmony Square, Bass Triangle, Kick, Snare, Hi-hat, Triangle Lead, Triangle Pad, Pulse Pad, Pulse Bass
- **3 pre-composed melodies** — including a structured NES-style Overworld theme
- **UI-based sound testing** — play individual instrument sounds on demand
- **Melody playback** — play full melodic sequences with multiple voices
- **Modular audio architecture** — demonstrates `InstrumentPreset`, melody sequencing, and audio scheduling

## Documentation links

- [Audio API](../../docs/api/API_AUDIO.md)
- [Input API](../../docs/api/API_INPUT.md)
- [UI API](../../docs/api/API_UI.md)
- [Core — Scene](../../docs/api/API_CORE.md)

## Build

From **`examples/music_demo`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
