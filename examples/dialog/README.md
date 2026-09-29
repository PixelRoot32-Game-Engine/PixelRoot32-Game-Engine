# Dialog Demo Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

One scene, one subject: `DialogRunner` + `DialogBox` behind
`PIXELROOT32_ENABLE_DIALOG`.

The runner is headless — no `Font`, no `Renderer`, only states — so the scene's
only job is to translate button presses into semantic `DialogAction` values
and feed them to it. `DialogBox` reads the runner and draws whatever it is
currently showing; the scene reacts to `ChoiceConfirmed` to report which
branch the player took.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_DIALOG=1`** — the only flag this example turns on;
  everything else keeps the engine's default build flags.

See [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h) for
every other default.

## Platforms

| Environment | Notes |
|-------------|-------|
| **`native`** | SDL2 window, 240×240 logical size. |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. |

## Controls

- **Up / Down** — move the highlighted option on a choice line.
- **A** — advance a text line, confirm a choice, or replay the script once it
  ends.
- **B** — cancel (a no-op here: none of this script's lines set
  `kLineFlagAllowCancel`).

## Script

The script (`DialogExampleScene.cpp`) exercises the three MVP behaviours in
one pass:

1. **An auto-advance line** — no input needed, moves on after 1800ms.
2. **A 3-line linear chain** — three `DialogLine::next` hops, advanced by the
   player.
3. **A 3-option `Choice` line** — `Up`/`Down` move the selection, `Confirm`
   fires `ChoiceConfirmed` and follows the chosen `DialogChoice::next`; all
   three branches rejoin at one `LineKind::End` line. A `ChoiceFilterFn`
   hides "Ask a riddle" once asked — replay the script to see the
   two-option menu. "Ask a riddle" also carries a right-aligned `detail`
   column ("once") drawn in its own colour slots — the multi-column row
   demo. Narrator speaks with a right-side portrait, Guide with a
   left-side one — speaker portraits facing each other.

## Features

- `DialogRunner` — headless five-state machine, driven by `feed()`/`update()`
- `DialogBox` — default panel, sized once via `measureHeightPx()`
- A `DialogEventFn` callback reacting to `ChoiceConfirmed`
- A `ChoiceFilterFn` hiding a one-time option, renormalized via
  `refreshChoices()`
- A multi-column option row: `DialogChoice::detail` right-aligned with
  per-column colour (`inkDetail`/`inkDetailSelected`)
- Speaker portraits in 4bpp at the `Size32` box: a Dragon Quest-style slime
  (Narrator, right) and a green-cap hero face (Guide, left) — simple NES
  references showing the multi-color portrait path (`portrait4bpp` +
  per-line `portraitPaletteSlot`)

## File Structure

```
src/
├── DialogExampleScene.h/.cpp  — the scene: script data, runner, box, input
└── platforms/
    ├── native.h               — SDL2 engine wiring
    └── esp32_dev.h            — ESP32 engine wiring
```

## Documentation links

- [Gameplay — DialogRunner](../../docs/api/generated/gameplay/DialogRunner.md)
- [Graphics — DialogBox](../../docs/api/generated/graphics/DialogBox.md)
- [Architecture](../../docs/architecture/architecture-index.md)

## Build

Run from **`examples/dialog`**:

```bash
pio run -e native
pio run -e esp32dev
```

> **Windows note.** The `native` environment compiles with MSYS2's MinGW `g++`.
> If `pio run -e native` fails with a bare `*** [...] Error 1` and no compiler
> diagnostic, `g++` is not on `PATH`. Add it before building:
> `export PATH="/c/msys64/mingw64/bin:$PATH"`.

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
