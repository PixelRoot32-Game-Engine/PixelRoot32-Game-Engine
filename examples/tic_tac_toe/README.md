# Tic-Tac-Toe Example

A **tic-tac-toe** game against an **AI** (minimax-style move search with configurable error chance). The board is drawn with **primitive rendering** (grid, X/O, cursor). **UI** uses **`UILabel`** for status text and either **`UIButton`** or **`UITouchButton`** depending on touch support.

On **`esp32cyd`**, **`PIXELROOT32_ENABLE_TOUCH`** and **`onUnconsumedTouchEvent`** map taps to cells; **`native`** / **`esp32dev`** use cursor + confirm-style input per `GameConstants.h` button IDs.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_TOUCH=1`** on **`native`** and **`esp32cyd`** (see `platformio.ini`) so touch code paths compile where used.
- ESP32 Dev preset does **not** set touch in `platformio.ini` — use GPIO **DPAD + A** (or equivalent) as in `GameConstants.h` (`BTN_UP`, `BTN_DOWN`, `BTN_PREV`, `BTN_NEXT`, `BTN_SELECT`).

`PIXELROOT32_ENABLE_UI_SYSTEM` defaults to **on** in the engine ([`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h)).

See **`platformio.ini`** for **`native`**, **`esp32dev`**, and **`esp32cyd`** (CYD uses **ILI9341** 240×320 + **XPT2046** touch — many calibration defines are in the INI).

## Platforms

| Environment | Display / input |
|-------------|----------------|
| **`native`** | SDL2, 240×240, simulated touch enabled |
| **`esp32dev`** | **ST7789** 240×240, no touch flags in INI |
| **`esp32cyd`** | **ILI9341** 240×320 + resistive touch (**XPT2046** GPIO SPI) |

## Controls (GPIO / keyboard)

- Navigate / change cell / confirm per `src/GameConstants.h` (`BTN_*` indices).
- **Play Again**: reset control wired in `TicTacToeScene::createResetButton()` (GPIO or touch widget depending on build).

## Touch (CYD)

Touches that the UI does not consume are handled in **`onUnconsumedTouchEvent`**, with hit slop around each cell (`kTouchHitSlop` in [`TicTacToeScene.h`](src/TicTacToeScene.h)).

## Features

- **Scene** lifecycle + **UI labels** and conditional **touch / GPIO buttons**
- **`TouchEvent`** pipeline for board placement
- **AI**: `computeAIMove`, win detection, draw state
- **Custom palette** and vector draw for marks (no tilemap required for the board)

## Documentation links

- [UI API](../../docs/api/API_UI.md)
- [Input API](../../docs/api/API_INPUT.md)
- [Core API](../../docs/api/API_CORE.md)

## Build

From **`examples/tic_tac_toe`**:

```bash
pio run -e native
pio run -e esp32dev
pio run -e esp32cyd
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
pio run -e esp32cyd --target upload
```
