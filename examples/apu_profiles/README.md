# apu_profiles

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

Nine 12-second blocks — `simple`, `medium` and `complex`, each played under
`Legacy`, `Nes` and `Hybrid` — loop forever on a classic ESP32 with a MAX98357A
DAC and an ILI9341 panel. **The speaker is the judge; the serial console is the
record; the panel is the sign on the door.**

Only a human ear can answer "which profile sounds better", so this example fixes
everything else: the programme, the order, the length and the hardware. Two
profiles are always compared on the same notes, and every block leaves a line in
the log that can be checked against the next build.

## What it demonstrates

### The three profiles own the character, not the notes

A `SoundProfile` is *configuration*, not state: it decides the default envelope
curve, the mix balance, the output filters, the default pulse duty, the noise
period LUT, unipolar noise and whether pan/echo are enabled. The same note tables
therefore sound different under each profile without the example changing a
single note.

| Profile | What it adds on top of `Legacy` | Panel colour |
|---|---|---|
| `Legacy` | Nothing — the flat reference. Continuous duty, preset noise period, linear envelope. | White |
| `Nes` | Integer core with the published NES balance, unipolar noise on the NES period LUT, and duty snapped to the four hardware patterns (12.5 / 25 / 50 / 75 %). | Green |
| `Hybrid` | Everything `Nes` has, plus an exponential decay/release curve. | Cyan |

### The profile is switched with a command, not a call

`setSoundProfile` is an audio-thread call. From the main thread the documented
crossing is the command queue, so each block submits
`AudioCommandType::SET_SOUND_PROFILE` **before** `MUSIC_PLAY` — the first note of
a block already sounds under the profile that block is labelled with.

### The pieces are cumulative on purpose

A difference heard at one level can then be attributed to the number of voices:

- **`simple`** — one 50 % `PULSE` melody, sixteen eighth notes, looping. One voice.
- **`medium`** — the melody plus a `TRIANGLE` bass on the root of each bar
  (i–VI–III–VII) plus a `NOISE` hi-hat on every beat. Three voices.
- **`complex`** — four music tracks (melody, a diatonic third above on a 25 %
  duty pulse, bass, kick/hat percussion) plus a four-voice `PLAY_EVENT` layer on
  the SFX slots: a sustained `SAW` pad, a `SINE` counter-melody on the half notes,
  a `NOISE` snare on beats 2 and 4, and a `PULSE` arpeggio on the eighth notes.
  Eight voices at the peak.

A looping `PLAY_EVENT` is a sustained tone and cannot retrigger, so only the pad
is `loop = true`; the snare, the counter-melody and the arpeggio are one-shot
events re-submitted on the eighth-note grid. That is also where the envelope
difference is easiest to hear: on percussion, not on a held note.

### The post-mix hook proves the block happened

After `init()`, the example attaches a peak/hash meter to the APU instance the
audio task actually renders with:

```cpp
gEngine.getScheduler()->getApuCore().setPostMixMono(&postMixMono, nullptr);
```

It has to be attached there and not through `AudioConfig::postMixMono`:
`AudioEngine` **copies** its `AudioConfig` in the constructor, so setting the
field afterwards never reaches the copy `init()` reads. `peak` proves the block
was not silent, and the FNV-1a hash proves two profiles did not render
byte-identical audio — `underruns=0` alone proves neither.

## The programme

| Block | Piece | Profile | Length |
|---|---|---|---|
| 1 | `simple` | `Legacy` | 12 s |
| 2 | `simple` | `Nes` | 12 s |
| 3 | `simple` | `Hybrid` | 12 s |
| 4 | `medium` | `Legacy` | 12 s |
| 5 | `medium` | `Nes` | 12 s |
| 6 | `medium` | `Hybrid` | 12 s |
| 7 | `complex` | `Legacy` | 12 s |
| 8 | `complex` | `Nes` | 12 s |
| 9 | `complex` | `Hybrid` | 12 s |

After block 9 the programme restarts at block 1, forever: one full cycle is
1 min 48 s. Every piece is in A minor / C major at 120 BPM.

## Wiring (fixed)

Classic ESP32 (`board = esp32dev`), the `pong` game's `PLATFORM_ESP32DEV_ILI9341`
board: a MAX98357A DAC and an ILI9341 240×320 panel mounted in portrait and used
in landscape. There is one wiring and one environment on purpose — a listening
comparison is only meaningful on the board the profiles ship on.

| Signal | GPIO |
|---|---|
| I2S BCLK | 5 |
| I2S LRCK | 21 |
| I2S DOUT | 19 |
| TFT MISO | 22 |
| TFT MOSI | 23 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | 4 |
| TFT backlight | 16 |

Sample rate: **22 050 Hz**, mono. No buttons, no touch. The TFT_eSPI `-D` block
in [`platformio.ini`](platformio.ini) is the panel configuration; I2S and SPI
pins do not collide.

## Build and flash

From **`examples/apu_profiles`**:

```powershell
pio run -e esp32dev                                   # build
pio run -e esp32dev --target upload --upload-port COM4 # flash
pio device monitor --port COM4 --baud 115200           # listen + capture
```

`platformio.ini` keeps the shared `[base]` / `[base_esp32]` sections inline
instead of in `lib/platformio.ini`, because this example has no `native`
environment to share them with. There is no simulator target: I2S output and a
real panel are the point.

Measured on the build performed while porting the example (local APU override,
see below): **334,633 bytes flash (25.5%)**, **24,040 bytes RAM (7.3%)**.

Two warnings show up in this build, and neither comes from this example's own
code:

- `cc1.exe: warning: command line option '-fno-rtti' is valid for C/ObjC++ but
  not for C` — the shared `[base_esp32]` flags apply to the framework's C sources
  as well, so the C compiler reports the flag it ignores.
- `TFT_eSPI.h:973: warning: #warning >>>>------>> TOUCH_CS pin not defined` —
  TFT_eSPI's own header, because this panel has no touch controller. The `-D`
  block is copied verbatim from the board that was verified, so it is left alone.

## Serial output

Once at start-up:

```
# apu_listen rate=22050 pins=5/21/19 block_ms=12000
# order piece=simple,medium,complex x profile=Legacy,Nes,Hybrid; repeats forever
# profiles: Legacy=flat reference, Nes=stepped duty+unipolar noise, Hybrid=exponential envelope
# sizes MusicNote=... MusicTrack=... AudioEvent=... AudioCommand=...
```

One banner before each block, one report when it ends:

```
# LISTEN piece=simple profile=Legacy block=1 of 9
REPORT piece=simple profile=Legacy underruns=0 worst_render_us=0 peak=18432 hash=1A2B3C4D
```

- `underruns` — `ESP32_I2S_AudioBackend::getUnderrunCount()` **delta for this
  block** (the backend counter is monotonic, so the block value is a difference).
- `worst_render_us` — delta of `getWorstRenderUs()` for this block. That counter
  is a running maximum, so this is the increase: `0` means the block set no new
  record, not that it rendered instantly.
- `peak` — largest absolute mixed sample in the block, read through the post-mix
  hook. `peak=0` means the block was digital silence.
- `hash` — FNV-1a over the block's mixed samples. Two profiles reporting the same
  hash for the same piece did not reach the renderer, so a difference the ear
  claims to hear cannot be confirmed and a difference it misses cannot be
  dismissed.

`sizeof` of `MusicNote` / `MusicTrack` / `AudioEvent` / `AudioCommand` is printed
so a report can be tied to the exact build that produced it.

## Status panel

TFT_eSPI is used directly — no `Renderer`, no `Scene`, no engine input config.
This is one static screen, not a game, and the scene stack would only cost flash.
The panel is laid out for a 320×240 logical canvas (the physical 240×320 panel
after `DISPLAY_ROTATION=3`) and is repainted once per block, after the block's
audio commands are queued and before its timer starts.

```
 y=4    APU LISTEN  22050 Hz  I2S 5/21/19          dark grey, 8 px
 y=18   PIECE                                      light grey, 8 px
 y=28   SIMPLE / MEDIUM / COMPLEX                  white, 26 px letters x2
 y=86   PROFILE                                    light grey, 8 px
 y=96   LEGACY / NES / HYBRID                      26 px letters x3, profile colour
 y=178  full-width colour bar (320x6)              profile colour
 y=190  BLOCK 3/9                                  white, 16 px
 y=210  PREV UNDERRUNS <n>   PEAK <n>              light grey, 8 px
 y=221  PREV HASH <8 hex digits>                   light grey, 8 px
 y=230  prev = the block that just ended; ...      dark grey, 8 px
```

The `PREV` numbers are the ones the previous `REPORT` line carried, so screen and
log always agree; on block 1 they read `PREV none yet - first block`. The top
line is drawn on every repaint, so a black panel is a display problem and not a
dead board.

The panel is optional by construction. It is initialised only after the first
block is already playing, and every draw returns immediately until that init
completed: `TFT_eSPI::init()` returns `void` and shifts its commands out without
waiting for an answer, so an absent screen cannot block the audio. A missing
panel costs the screen and nothing else.

If the I2S peripheral never starts, the firmware says so three seconds in
(`# WARN backend never iterated: i2s init failed? pins=5/21/19`) instead of
pretending the run is healthy.

## Trying unreleased APU changes

`lib_deps` ships as:

```ini
lib_deps =
	symlink://../../
	TFT_eSPI=https://github.com/Gperez88/TFT_eSPI.git#master
```

`symlink://../../` pulls the engine, and the engine's `library.json` declares
`gperez88/PixelRoot32-APU: ^2.0.0` — so the **published** APU is what a reader of
this example gets. This example needs the profile API (`SoundProfile`,
`AudioCommandType::SET_SOUND_PROFILE`), which the published registry package did
not declare yet when the example was written; that is what the override below is
for.

To compile against an unreleased APU checkout, add a second local entry, and
PlatformIO compiles that checkout instead of the registry copy:

```ini
lib_deps =
	symlink://../../
	symlink://C:/path/to/your/PixelRoot32-APU
	TFT_eSPI=https://github.com/Gperez88/TFT_eSPI.git#master
```

Then put it back before committing: an absolute path on one machine is not
something the repository can ship. `symlink://` is not an operating-system
symlink — PlatformIO records the path in a `.pio-link` JSON and compiles the
library in place, so edits in that checkout are picked up by the next build.

## Status

The programme, the serial lines and the panel layout come from the APU
repository's `tools/apu_listen` harness and are carried here unchanged, so a
transcript from either build can be compared line for line. That harness was run
on the hardware described above; this example was verified here by compiling it
against a local APU checkout, and the firmware it produces is the same program.
Nothing in this example claims a profile sounds better — that verdict is the
listening test's output, not its input.

## Documentation links

- [Audio API](../../docs/api/audio.md) — backend, scheduler and the counters the report uses
- [Audio configuration](../../include/audio/AudioConfig.h) — sample rate, block size, post-mix hook
- [Engine configuration](../../include/platforms/EngineConfig.h) — display and platform defaults
