# Tic-Tac-Toe Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

A **tic-tac-toe** game against an **AI** (minimax-style move search with configurable error chance). The board is drawn with **primitive rendering** (grid, X/O, cursor). **UI** uses **`UILabel`** for status text and either **`UIButton`** or **`UITouchButton`** depending on touch support.

On **`esp32cyd`**, **`PIXELROOT32_ENABLE_TOUCH`** and **`onUnconsumedTouchEvent`** map taps to cells; **`native`** / **`esp32dev`** use cursor + confirm-style input per `GameConstants.h` button IDs.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_TOUCH=1`** on **`native`** and **`esp32cyd`** (see `platformio.ini`) so touch code paths compile where used.
- ESP32 Dev preset does **not** set touch in `platformio.ini` — use GPIO **DPAD + A** (or equivalent) as in `GameConstants.h` (`BTN_UP`, `BTN_DOWN`, `BTN_PREV`, `BTN_NEXT`, `BTN_SELECT`).
- **`PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=1`** — required, not optional. `GameConstants.h` declares `kBoardGrid` as a `gameplay::GridSpec`, and the whole `GridSpace.h` header lives behind this flag (default `0`), so the example does not compile without it.

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
- Cell/world conversion via **`gameplay::GridSpace`**: board centring, grid lines, cursor position, and mark drawing are all placed through `kBoardGrid` (`GameConstants.h`) instead of hand-rolled `* CELL_SIZE` / `/ CELL_SIZE` arithmetic. `touchToCell()` maps a raw touch position to a board cell with `worldToCellX/Y` + `containsCell`, which correctly rejects touches inside `kTouchHitSlop` that land outside the board (a truncating conversion would have silently accepted some of them).

## Documentation links

- [UI API](../../docs/api/ui.md)
- [Input API](../../docs/api/input.md)
- [Core API](../../docs/api/core.md)
- [Memory system — gameplay flags and byte budgets](../../docs/architecture/memory-system.md) — RAM cost of `PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE`
- [`gameplay/GridSpace.h`](../../include/gameplay/GridSpace.h) — the full grid conversion API

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
