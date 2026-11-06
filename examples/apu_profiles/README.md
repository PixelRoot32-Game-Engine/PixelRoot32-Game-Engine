# apu_profiles

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

Nine 12-second blocks — `simple`, `medium` and `complex`, each played under
`Legacy`, `Nes` and `Famicom` — loop forever on a classic ESP32 with a MAX98357A
DAC and an ILI9341 panel, or on a PC through SDL2 with the terminal as the record.
**The speaker is the judge; the serial console is the record; the panel is the
sign on the door.**

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
| `Famicom` | The `Nes` character — same envelope, duty, noise and mix — through the Famicom's output stage: a 37 Hz high-pass into the low-pass, so bass and kick keep their fundamentals. `Nes` runs the NES stage instead (high-pass 90 Hz → 440 Hz → low-pass). | Cyan |

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
- **`complex`** — the Tool Suite sample song **"Crystal Cascade"**
  (`PixelRoot32-Tool-Suite/toolsuite_app/samples/music/Crystal_Cascade.pr32music`)
  at its own 150 BPM: a 12.5 % `PULSE` lead in 16th notes, a `TRIANGLE` bass, a
  25 % `PULSE` harmony and a `NOISE` drum kit (kick on 1 and 3, snare on 2 and 4,
  hi-hat on every eighth, a crash and an open hat closing the pattern). Three
  melodic voices; the drum hits play on the SFX slots, up to three overlapping at
  the closing crash, so six voices at the peak.

The complex piece follows the Tool Suite's sequencing model, which is what keeps
it in time: every timed voice is a sequencer track, none is fired from
`millis()`. The first pattern of each of the song's four tracks is written out
the way the Tool Suite's preview builds it — notes on one 16th-note step grid by
start step and length, gaps merged into one rest, drum hits on the same step
stacked with length 0 — and every track spans exactly 64 steps (16 beats), which
a `static_assert` checks, so the four tracks loop together forever. The song's
library instruments (`LIB_PULSE_SPARK`, `LIB_PULSE_THIN`, `LIB_PULSE_PLUCK`,
`LIB_PULSE_MARIMBA`, `LIB_TRIANGLE_SUB_BASS`, `LIB_DRUM_CRASH`,
`LIB_DRUM_OPEN_HAT`) are defined in `main.cpp` with the Tool Suite catalogue's
values; kick, snare and hi-hat are the APU's `INSTR_*` presets. Percussion is
where the envelope and noise differences between profiles are easiest to hear.

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
| 3 | `simple` | `Famicom` | 12 s |
| 4 | `medium` | `Legacy` | 12 s |
| 5 | `medium` | `Nes` | 12 s |
| 6 | `medium` | `Famicom` | 12 s |
| 7 | `complex` | `Legacy` | 12 s |
| 8 | `complex` | `Nes` | 12 s |
| 9 | `complex` | `Famicom` | 12 s |

After block 9 the programme restarts at block 1, forever: one full cycle is
1 min 48 s. `simple` and `medium` are in A minor at 120 BPM; `complex` is
"Crystal Cascade", in C major at 150 BPM (one 64-step pattern lasts 6.4 s).

## Wiring (fixed)

Classic ESP32 (`board = esp32dev`), the `pong` game's `PLATFORM_ESP32DEV_ILI9341`
board: a MAX98357A DAC and an ILI9341 240×320 panel mounted in portrait and used
in landscape. There is one wiring on purpose — a listening comparison is only
meaningful on the board the profiles ship on — but the programme also runs on a PC,
where the panel is simply absent.

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

Two environments, the same programme: `esp32dev` is the listening session on the
board, `native` is the same nine blocks on a PC with SDL2 audio. The transcript is
identical in shape, so a log from either build can be compared with the other.

From **`examples/apu_profiles`**:

```powershell
pio run -e esp32dev                                    # build
pio run -e esp32dev --target upload --upload-port COM4 # flash
pio device monitor --port COM4 --baud 115200           # listen + capture
```

And on a PC (needs SDL2 — `libsdl2-dev` on Linux, `pacman -S mingw-w64-x86_64-SDL2`
on MSYS2; the Windows include/lib paths and `-mconsole` are already in
`platformio.ini`, and CI deletes those three lines to build against the system
SDL2):

```powershell
pio run -e native
.pio/build/native/program
```

The shared `[base]` / `[base_esp32]` / `[base_native]` sections live in
`lib/platformio.ini`, like every other example in this repository.

Measured on the build made against the published APU **2.3.0**, no local override:

| Environment | Flash | RAM |
|---|---|---|
| `esp32dev` | **345 532 B (26.4 %)** | **24 852 B (7.6 %)** |
| `native` | — | — |

The PC build has no panel and no backend counters: `initDisplay()` reports failure
so the example says so once, and `underruns` / `worst_render_us` print as `0` in
every `REPORT` because `SDL2_AudioBackend` exposes neither. `peak` and `hash` are
the numbers that mean something there — they are what prove the block rendered.

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
# order piece=simple,medium,complex x profile=Legacy,Nes,Famicom; repeats forever
# profiles: Legacy=flat reference, Nes=stepped duty+unipolar noise, Famicom=Nes character+37 Hz HPF output stage
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
 y=96   LEGACY / NES / FAMICOM                     26 px letters x3, profile colour
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
the APU dependency — so the **published** APU is what a reader of this example
gets. The example needs APU **2.3.0** or later, the release that adds
`SoundProfile::Famicom` (the profile API itself, `SoundProfile` and
`AudioCommandType::SET_SOUND_PROFILE`, has been there since 2.1.0); 2.3.0 is on
the registry, so nothing has to be overridden. The override below is only for
working against an APU checkout that has not been released yet.

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

This example **is** the listening harness. It started as the APU repository's
`tools/apu_listen` and replaced it when the harness moved here (APU commit
`71758cb`, "move the listening harness to the engine examples"); the APU no
longer ships its own copy. The serial lines keep the `# apu_listen` prefix and
the panel keeps its layout so older transcripts stay readable, but this
example's transcripts are now the reference for comparing APU releases by ear.

`peak` and `hash` figures recorded before APU 2.3.0 — including the `complex`
peaks quoted in the APU's `docs/ABI.md` — belong to the previous `complex`
piece, which layered a `millis()`-driven SFX grid over the music. The current
piece is the "Crystal Cascade" transcription, so its figures are a new baseline
and are not comparable with those. Nothing in this example claims a profile
sounds better — that verdict is the listening test's output, not its input.

Both environments were built for this revision against the published APU
2.3.0: `esp32dev` (**345 532 B** flash) and `native`, which was also run
headless through the first eight blocks to confirm every piece renders — the
complex piece reports `peak=7671` under `Legacy` and `peak=6435` under `Nes`.
The `esp32dev` build was flashed and listened to on the board described above
(2026-11-06): the three pieces play in time under `Legacy`, `Nes` and
`Famicom`.

## Documentation links

- [Audio API](../../docs/api/audio.md) — backend, scheduler and the counters the report uses
- [Audio configuration](../../include/audio/AudioConfig.h) — sample rate, block size, post-mix hook
- [Engine configuration](../../include/platforms/EngineConfig.h) — display and platform defaults
