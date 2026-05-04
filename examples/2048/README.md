# 2048 Example

A **2048** puzzle game where you slide tiles to combine them and reach **2048**. The game features **grid rendering**, **tile values**, **score tracking**, and **game over / win states**.

On **`esp32cyd`**, swipes are detected via **`onUnconsumedTouchEvent`** to slide tiles in the swipe direction. On **`native`**, use D-pad controls per `Game2048Constants.h` button IDs.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_TOUCH=1`** on **`native`** and **`esp32cyd`** (see `platformio.ini`) so touch code paths compile.

`PIXELROOT32_ENABLE_UI_SYSTEM` defaults to **on** in the engine ([`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h)).

See **`platformio.ini`** for **`native`** and **`esp32cyd`** (CYD uses **ILI9341** 240×320 + **XPT2046** touch — many calibration defines are in the INI).

## Platforms

| Environment | Display / input |
|-------------|----------------|
| **`native`** | SDL2, 240×320, simulated touch + keyboard enabled |
| **`esp32cyd`** | **ILI9341** 240×320 + resistive touch (**XPT2046** GPIO SPI) |

## Controls (GPIO / keyboard)

- **Arrow keys / D-pad** to slide all tiles in the selected direction.
- **Play Again**: reset button wired in `Game2048Scene::createResetButton()` (GPIO or touch widget depending on build).

## Touch (CYD)

Swipe gestures are detected in **`onUnconsumedTouchEvent`**:

- **DragStart / DragMove / DragEnd** events track the swipe direction.
- Minimum swipe distance of **30 pixels** triggers a move.
- Swipe direction is determined by the major axis (horizontal or vertical).

## Features

- **Scene** lifecycle + **UI labels** and conditional **touch / GPIO buttons**
- **TouchEvent** pipeline for swipe detection
- **Game logic**: tile sliding, merging, spawning, win/lose detection
- **Custom palette** in `color_palette.h` (PR32-compatible tile colors)
- **Score tracking** displayed in label
- **Audio SFX** (no background music): spawn (Triangle), merge (double Triangle), move (simple Triangle)
- **Fixed merge behavior**: correct merge in all 4 directions (left, right, up, down)
- **AI Auto-play**: expectimax algorithm with depth-3 search and heuristic evaluation

## Audio

Audio SFX are defined in **`src/assets/sfx.h`**:

- `playSpawnSound` - Triangle wave (A4) - plays when new tile spawns
- `playMergeSound` - Double Triangle (C5 + E5) - plays when tiles merge
- `playMoveSound` - Simple Triangle (A3) - plays on move without merge
- `playWinSound` / `playGameOverSound` - win/lose feedback

No background music (removed to reduce memory footprint).

## AI Auto-play

The AI uses an **Expectimax** algorithm with depth-3 search:

| Heuristic | Weight | Description |
|-----------|--------|-------------|
| Empty cells | 1.0 | More empty = better options |
| Monotonicity | 0.5 | Prefer monotonic rows/columns |
| Smoothness | 0.3 | Adjacent tiles should have similar values |
| Max tile | 0.8 | Higher max = closer to win |
| Corner bonus | 1.2 | Keep max tile in corner |

**Controls**:
- AI is enabled by compile flag `-D GAME2048_AI_MODE=1` in `platformio.ini`
- When enabled, AI automatically makes optimal moves every frame
- AI respects game rules from `game_rules.md`

To disable AI, remove `-D GAME2048_AI_MODE=1` from build_flags in platformio.ini.

## Documentation links

- [UI API](../../docs/api/ui.md)
- [Input API](../../docs/api/input.md)
- [Core API](../../docs/api/core.md)

## Build

From **`examples/2048`**:

```bash
pio run -e native
pio run -e esp32cyd
```

## Upload (ESP32)

```bash
pio run -e esp32cyd --target upload
```
