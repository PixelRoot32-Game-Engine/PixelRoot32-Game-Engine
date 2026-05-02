# Music Demo Example

Interactive **audio** demo showcasing the **PixelRoot32 audio subsystem** with **instrument presets** and **pre-composed multi-part music**. Features a **UI-based interface** for testing sound presets and playing layered `MusicTrack` arrangements.

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
- **Select / Play** — trigger button callbacks to play instrument sounds or melodies
- **Same melody again** — pressing the **currently playing** melody button **stops** playback (toggle)
- **Back** — **`B`** stops music if playing, then returns to the previous menu level

## Melody assets (`src/assets/`)

Tracks are split per theme; shared beat constants and demo-only **`InstrumentPreset`** overrides live in **`common_melodies.h`**:

| File | Role |
|------|------|
| [`common_melodies.h`](src/assets/common_melodies.h) | Beat fractions (`S`/`E`/`Q`/…), `kDemoArcadeLeadWave` / `kDemoAdventureLeadWave`, **`DEMO_SNES_LEAD_TIGHT`** / **`DEMO_SNES_BASS_STAC`** (tighter ADSR for SNES-style arranging), **`ARP_STEP`** |
| [`classic_arcade_melody.h`](src/assets/classic_arcade_melody.h) | **Melody 1** — Classic Arcade (`sClassicArcadeTrack`) |
| [`adventure_melody.h`](src/assets/adventure_melody.h) | **Melody 2** — Adventure (`sAdventureTrack`) |
| [`action_melody.h`](src/assets/action_melody.h) | **Melody 3** — Action (`sActionTrack`) |
| [`arpeggio_melody.h`](src/assets/arpeggio_melody.h) | **Melody 4** — Em arpeggio demo (`sArpDemoTrack`) |

Each full arrangement uses **`MusicTrack`** layering: **main** + optional **`secondVoice`**, **`thirdVoice`**, and **`percussion`**, flattened by `MusicPlayer` into the global voice pool (**`ApuCore::MAX_VOICES`** = 8). The headers comment on keeping harmony/percussion notes relatively short so **SFX** can share the pool without constant stealing.

## Melodies (UI labels vs. engine)

| UI button | BPM (see `MusicDemoScene::playMelody`) | Loop length (beats) | Layers (summary) |
|-----------|----------------------------------------|---------------------|------------------|
| **Melody 1** | 140 | 32 | **SAW** lead (`kDemoArcadeLeadWave`), bass (`DEMO_SNES_BASS_STAC`), pulse harmony stabs, noise drums (two groove blocks + fill) |
| **Melody 2** | 125 | 64 | **SINE** lead (`kDemoAdventureLeadWave`), bass, harmony, drums (extended **A \| B \| A′ \| C**-style material) |
| **Melody 3** | 160 | 32 | **PULSE** lead via **`DEMO_SNES_LEAD_TIGHT`** (16th-style arpeggio macros), matching bass, sparse harmony hits, dense 16th-hat drums + break |
| **Melody 4 + ARP voice** | 145 | 32 | **SAW** lead (`kDemoArcadeLeadWave`), **`secondVoice`**: fast Em arpeggio (`INSTR_TRIANGLE_LEAD`, **`WaveType::SINE`** on the sub-track), **`thirdVoice`**: bass, **same drum grid as Melody 1** for a stable loop |

## Features

- **10 engine instrument presets** (one-shot tests) — Lead Square, Harmony Square, Bass Triangle, Kick, Snare, Hi-hat, Triangle Lead, Triangle Pad, Pulse Pad, Pulse Bass
- **4 multi-part demo tracks** — layered loops with distinct BPM and form (see table above)
- **Melody 4** — full **four-part** demo: lead + arpeggiated **`secondVoice`** + bass + percussion (not just lead + arp)
- **Audio Lab menu** — **pulse frequency sweep** (Phase A), **SINE / SAW chord** one-shots (Phase B), and **master bitcrush** cycling via `AudioEngine::setMasterBitcrush`
- **UI-based sound testing** — play individual instrument sounds on demand
- **Modular audio architecture** — demonstrates `InstrumentPreset`, per-demo preset tweaks, melody sequencing, `AudioEvent` sweep fields, and audio scheduling

## Documentation links

- [Audio API](../../docs/api/audio.md)
- [Music player guide](../../docs/guide/music-player-guide.md)
- [Input API](../../docs/api/input.md)
- [UI API](../../docs/api/ui.md)
- [Core — Scene](../../docs/api/core.md)

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
