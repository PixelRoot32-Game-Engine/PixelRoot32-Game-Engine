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

The AI uses an **optimized Expectimax** algorithm with adaptive depth search and intelligent state caching:

### Algorithm

| Feature | Implementation |
|---------|---------------|
| **Search** | Expectimax (MAX nodes: player moves, CHANCE nodes: tile spawns) |
| **Depth** | Adaptive 2-5 based on empty cells (deeper when board is tight) |
| **Sampling** | Smart: evaluates all cells when ≤8 empty, samples 6 when >8 empty |
| **Spawn probability** | Spawn-2 (90%) + Spawn-4 (10%) with proper weighting |
| **Transposition Table** | 8192-entry cache with FNV-1a hashing to avoid recomputing states |
| **Probability cutoff** | Stops exploring paths with <0.01% cumulative probability |

### Heuristic Evaluation

The AI evaluates board states using 6 weighted factors:

| Factor | Weight | Description |
|--------|--------|-------------|
| **Empty cells** | 4K-100K (exponential) | Mobility bonus that increases as board fills up |
| **Monotonicity penalty** | -100× | Severe penalty for non-monotonic sequences (larger tiles matter more) |
| **Smoothness** | +100× | Rewards adjacent tiles with similar values (merge potential) |
| **Merge bonus** | +1000× per merge | Strong incentive for creating merge opportunities |
| **Corner bonus** | +100× maxTile | Rewards keeping max tile in any corner |
| **Corner penalty** | -1000× maxTile | **Catastrophic penalty** if max tile is NOT in a corner |
| **Adjacent tiles** | +0.5× value | Bonus for tiles adjacent to max tile (builds L-pattern) |

### Performance

- **Native (PC)**: Base depth 3, adaptive up to depth 5
- **ESP32**: Base depth 2, adaptive up to depth 4
- Average decision time: <100ms on native, <500ms on ESP32
- **Win rate**: Consistently reaches 2048 tile

### Controls

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
