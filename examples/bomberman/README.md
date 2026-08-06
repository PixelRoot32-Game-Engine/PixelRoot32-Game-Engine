# Bomberman Example

A classic **Bomberman**-style game on a **13×11** grid: place bombs, chain
explosions, destroy soft walls, dodge four PRNG-driven enemies, collect
power-ups, and reach the exit once every enemy is dead. Movement is
**interpolated** between cells (not a teleport-per-cell like some of the
other grid examples), and every timed rule — movement, bomb fuses,
explosions, the level clock — runs on a **fixed 20 ms logic step**, decoupled
from the rendered frame rate.

## Requirements (build flags)

Set in [`lib/platformio.ini`](lib/platformio.ini)'s `[base]` template, so
every environment inherits them:

- **`PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=1`** — required, not optional.
  `BombermanConstants.h` declares `kBoardGrid` as a `gameplay::GridSpec`, and
  the whole `GridSpace.h` header lives behind this flag (default `0`), so the
  example does not compile without it.
- **`PIXELROOT32_ENABLE_AUDIO=1`** — needed for the four sound events below.
- **`PIXELROOT32_ENABLE_PHYSICS=0`** — the physics system is never used;
  every blocking/collision check here is a board lookup, not a physics query.
- **`PIXELROOT32_ENABLE_PARTICLES=0`** — not used.
- **`PIXELROOT32_ENABLE_UI_SYSTEM=0`** — HUD and overlays are drawn directly
  with `Renderer::drawText`/`drawTextCentered`, not the UI widget system.

Display size is **240×240** in the project `platformio.ini` (see
`PHYSICAL_DISPLAY_*`).

## Platforms

| Environment | Display | Audio backend |
|-------------|---------|----------------|
| **`native`** | SDL2, 240×240 | `SDL2_AudioBackend` in [`src/platforms/native.h`](src/platforms/native.h) |
| **`esp32dev`** | **ST7789** 240×240 | `ESP32_I2S_AudioBackend` in [`src/platforms/esp32_dev.h`](src/platforms/esp32_dev.h) |

Pin choices (ST7789 SPI, I2S audio, D-pad + two buttons) are in
`src/platforms/esp32_dev.h` — edit there if your wiring differs.

## Controls

| Action | `native` (keyboard) | `esp32dev` (GPIO) |
|--------|----------------------|--------------------|
| Move | Arrow keys | D-pad |
| Place bomb | Space | Button A |
| Restart (from Game Over / Stage Clear) | Enter | Button B |

Movement has a fixed priority — **Up > Down > Left > Right** — when more than
one direction is held at once, so input is never ambiguous and never
diagonal. Holding a direction into a wall, a bomb, or the board edge simply
leaves the player in place; it is never a death.

## How audio is triggered

`BombermanScene::logicStep()` builds `pixelroot32::audio::AudioEvent` values
and calls `engine.getAudioEngine().playEvent(...)`, the same pattern the
`snake` example uses. Four events, each tied to one exact pipeline moment:

- **Bomb placed** — the step a bomb placement actually succeeds (not every
  press of the bomb button; a press rejected by the bomb limit or an
  already-bombed cell stays silent).
- **Explosion** — the step any bomb(s) actually detonate, whether from a
  timer or a chain reaction (one event per step, even if several bombs chain
  together on that step).
- **Player death** — one shared event for every death cause: explosion
  contact, enemy contact, or the countdown reaching zero.
- **Stage clear** — the step victory is reached (last enemy already dead,
  player steps onto the revealed exit).

## Features

- **Deterministic board generation.** `BombermanBoard.cpp` builds the board
  from a seed using the engine's seeded PRNG (`math::set_seed` /
  `math::rand_int`) — never `std::rand`. Hard walls sit on the border and at
  every even-x/even-y interior cell; every other cell is on a connected
  corridor lattice by construction (no reachability check needed, since soft
  walls are all destructible and never permanently block a corridor cell).
  The player's start cell and its two adjacent cells are guaranteed free of
  soft walls.
- **Interpolated grid movement.** The player moves at 12 logic steps per
  cell, enemies at 20. Both use the same small `GridMove` struct
  (`src/GridMove.h`) but each actor writes its own advance loop — the
  player stays put when blocked and reads held input; an enemy re-picks a
  direction and reads the seeded PRNG. The two loops are deliberately not
  merged into a shared controller; they disagree on enough policy (blocking
  behavior, direction source, and the pass-through rule below) that sharing
  more than the struct would need extra flags for every difference.
- **Own-bomb pass-through.** A player can always leave the cell they just
  bombed; that bomb becomes solid again the instant they arrive in a new
  cell. Enemies have no such exemption — every bomb is solid to them.
- **Chain reactions.** An explosion reaching another bomb detonates it
  immediately, in the same logic step, via a small bounded queue (at most
  8 entries — the size of the bomb pool) instead of recursion, so a ring of
  mutually-adjacent bombs still resolves in a fixed number of iterations.
- **Hidden content lives in the tile type, not a side table.** The three
  soft-wall variants (plain, hiding the exit, hiding a power-up) render and
  block identically until destroyed — there is only one array that could
  disagree with what is drawn.
- **Cell/world conversion via `gameplay::GridSpace`.** The board's
  `constexpr GridSpec` places the playfield below a reserved status band
  (`containsCell` rejects that band automatically, so no gameplay rule needs
  to know the HUD exists) — no hand-rolled `* 16` / `/ 16` arithmetic
  anywhere in the game logic.
- **Plain-enum state, not the engine's generic state-machine primitive.**
  The level's five states (Loading, Playing, ExitUnlocked, StageClear,
  GameOver) are one plain `enum class`, driven by a fixed nine-stage update
  pipeline that runs the same conditions in the same order every logic step.
  Player, bomb, and tile "state" are likewise derived from a couple of
  existing fields rather than stored as separate enums.
- **Entity budget guardrail.** A `static_assert` (compile-time) plus a
  post-init `assert` (development builds only — it compiles out under
  `NDEBUG` on `esp32dev` release builds) both guard against the entity pool
  silently dropping an actor once its fixed array is full.
- **HUD and overlays.** Lives, enemies remaining, current fire power / max
  bombs (`F n B n`), and the countdown are drawn every frame straight from
  the live game state — there is no cached copy that could go stale. Game
  Over and Stage Clear are a **text overlay drawn on top of the board**, not
  a separate screen or `Scene`.
- **Level countdown.** A `TIME` value counts down once per logic step
  (never per rendered frame); reaching zero costs one life through the exact
  same path as an explosion or an enemy contact. The 180-second starting
  value is a first estimate, not a value tuned against a timed play session.

## Documentation links

- [Audio API](../../docs/api/audio.md)
- [Core API](../../docs/api/core.md)
- [Input API](../../docs/api/input.md)
- [Memory system — gameplay flags and byte budgets](../../docs/architecture/memory-system.md)
- [Grid cell occupancy pattern](../../docs/patterns/grid-cell-occupancy.md) — the status-band-above-the-grid layout this example uses
- [`gameplay/GridSpace.h`](../../include/gameplay/GridSpace.h) — the full grid conversion API

## Build

From **`examples/bomberman`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
