# Platform support: desktop and Web limits against ESP32

> **Source of truth:** the code, not this page. The section that covers a row cites the file that decides it. If this page and the source disagree, the source is right and the page is a bug.
>
> **Scope:** what each target can actually do. ESP32 variant details (DAC, FPU, memory per chip) live in the [Platform compatibility guide](../guide/platform-compatibility.md).

## The rule this page exists to state

**ESP32 is the primary target.** Desktop (macOS, Windows, Linux through SDL2) and Web (Emscripten) are secondary: they run the same engine, scenes and `PIXELROOT32_ENABLE_*` flags, but they are *hosts*, not hardware. Several capabilities are **simulated** there rather than implemented, and a few are absent.

"Secondary" is not a statement about code quality. It is a statement about semantics: when a game needs a capability that only exists on a board, that dependency must be visible in the build, not discovered at runtime.

## Legend

| Mark | Meaning | What a player/developer sees |
|---|---|---|
| ✅ | Supported | Real implementation for that target |
| ⚠️ | Degraded | Runs, but with different behaviour, resources or guarantees |
| 🟡 | Simulated | Works against a stub or an in-process substitute; useful for development, not the real thing |
| ❌ | Absent | Not implemented, or the build fails on purpose |

Failure modes matter as much as the mark:

- **Build error** — `#error` with an explanation. Safe.
- **Runtime degradation** — runs, behaves differently. Must be documented (this page).
- **Silent no-op** — runs, does nothing, no diagnostic. The dangerous one; each case is called out below.

## Capability contract

Identity and capability are separate questions and both are answered in [`include/platforms/PlatformSelect.h`](../../include/platforms/PlatformSelect.h):

| Macro | Question it answers | ESP32 | Desktop | Web |
|---|---|---|---|---|
| `PIXELROOT32_PLATFORM_ESP32` | identity | 1 | 0 | 0 |
| `PIXELROOT32_PLATFORM_DESKTOP` | identity | 0 | 1 | 0 |
| `PIXELROOT32_PLATFORM_WEB` | identity | 0 | 0 | 1 |
| `PIXELROOT32_HAS_SDL` | "is SDL2 the display/input backend?" | 0 | 1 | 1 |
| `PIXELROOT32_HAS_ARDUINO` | "does `<Arduino.h>` exist?" | 1 | 0 | 0 |
| `PIXELROOT32_HAS_HOST_FS` | "is there a host filesystem?" | 0 | 1 | 1 |

All six are **always defined**, with value `0` or `1`. Use `#if`, never `#ifdef` — `#ifdef` is true on every target.

`PLATFORM_NATIVE` is a **legacy alias** kept in `[base_native]` (`platformio.ini:51`) and in the CMake interface (`CMakeLists.txt:190-191`). It also reaches the browser build, because `tools/web/build_web.py` rewrites native's compile database. **New code must not branch on it**; that inheritance is exactly how the browser ends up claiming desktop capabilities (see [Simulated, not implemented](#simulated-not-implemented)).

## Master matrix

| Capability | ESP32 | Desktop | Web |
|---|---|---|---|
| Networking transport | ✅ ESP-NOW | 🟡 in-process loopback | 🟡 in-process loopback |
| Persistence: key-value / slots | ✅ NVS | ✅ host files | ✅ IndexedDB (IDBFS) |
| Persistence: microSD | ✅ opt-in | ❌ build error | ❌ build error |
| Buttons | ✅ GPIO pins | ⚠️ keyboard scancodes | ⚠️ keyboard scancodes |
| Touch panel (XPT2046) | ✅ real over SPI | 🟡 pointer→touch | 🟡 pointer + fingers |
| Touch panel (GT911) | ❌ placeholder | ❌ placeholder | ❌ placeholder |
| Gamepad | ❌ | ❌ | ❌ |
| Colour TFT display | ✅ TFT_eSPI + DMA | ⚠️ SDL2 texture | ⚠️ WebGL via SDL2 |
| Monochrome OLED (U8G2) | ✅ | ❌ not compiled | ❌ not compiled |
| 8bpp framebuffer fast path | ✅ | ❌ `nullptr` | ❌ `nullptr` |
| Audio | ✅ I2S / DAC + FreeRTOS task | ✅ SDL2 + audio thread | ⚠️ synthesis inside the callback |
| Core count / chip info | ✅ real | 🟡 simulated 4 cores | 🟡 simulated 4 cores |
| Free heap in debug overlay | ✅ | ❌ `N/A` | ❌ `N/A` |
| GPIO / ADC / I²C / SPI | ✅ real | ❌ no-op / mock | ❌ no-op / mock |
| Watchdog / OTA / deep sleep | ❌ no engine API | ❌ | ❌ |

## Networking

| Target | Transport | Evidence |
|---|---|---|
| ESP32 | ESP-NOW over the WiFi radio; no router or access point needed | [`ESP32EspNowTransport.h`](../../include/drivers/esp32/ESP32EspNowTransport.h) |
| Desktop | `LoopbackTransport`: two simulated nodes in one process, with an injectable fault profile. No radio, no sockets | [`LoopbackTransport.h`](../../include/drivers/native/LoopbackTransport.h) |
| Web | The same loopback, compiled because the browser build inherits native's compile database | [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h) (`ENABLE_NETWORKING` guard) |

- Enabling `PIXELROOT32_ENABLE_NETWORKING` on any target that is neither ESP32 nor `PLATFORM_NATIVE` is a **build error**. The browser is not in that "any" because it defines the alias.
- The `encrypted` flag of the ESP-NOW transport is accepted and **ignored**: there is no encryption.
- Practical consequence: a multiplayer game is an ESP32 game. Desktop and Web sessions are single-process demos of the protocol, which is still useful — the two `esp_now` examples run in a browser exactly that way.

## Persistence

| Medium | Target | Notes |
|---|---|---|
| NVS | ESP32 | Byte-compatible with Arduino `Preferences` blobs; every write stalls flash for ~4 ms |
| microSD (FAT) | ESP32, opt-in | `PIXELROOT32_ENABLE_SD_STORAGE=1` requires persistence **and** ESP32, otherwise `#error` |
| Host files | Desktop, Web | One file per key or slot under a root directory; atomic `rename()` on desktop |
| IndexedDB (IDBFS) | Web | Mounted at `/pr32-persist`; explicit `FS.syncfs()` |
| Memory | All | For tests |

Web specifics that change how a game is written:

- **Loading is asynchronous.** There is no blocking wait without Asyncify, so the API exposes `isStorageReady()` and the game polls it. Reading before it is ready can overwrite a save. After `kStorageLoadTimeoutMs` (5 s) the store logs and lets reads through: losing a save beats not booting.
- **`fsync()` is a no-op** on the web (`HostFileStore.cpp`): durability comes from `FS.syncfs()`, already hooked to `visibilitychange` (hidden) and `beforeunload`.
- **Without `-lidbfs.js` there is no persistence at all.** The build still succeeds and the game still runs; a single console warning is the only signal. `tools/web/build_web.py` only adds the flag for examples that declare it.

## Input

| Source | ESP32 | Desktop | Web |
|---|---|---|---|
| Physical buttons | ✅ `digitalRead()` per pin | ⚠️ SDL scancodes | ⚠️ SDL scancodes |
| Mouse | ❌ | ✅ | ✅ |
| Touch panel | ✅ XPT2046 (SPI) | 🟡 pointer becomes a touch point | 🟡 pointer and real fingers |
| Gamepad | ❌ | ❌ | ❌ |

- `PIXELROOT32_ENABLE_TOUCH` defaults to **1 on the web and 0 everywhere else**. It does not enable a panel on desktop/web; it routes the pointer through the touch pipeline (gestures, dispatch) instead of the raw mouse state.
- On the web, `SDL_HINT_TOUCH_MOUSE_EVENTS=0` suppresses the browser's emulated mouse events for the same tap, so a press is not delivered twice.
- **GT911 is a placeholder on every target.** `readRegister()`/`readCoordinate()` return `0`, and the flags compile: a capacitive panel wired to the board reads nothing. Do not treat `-D TOUCH_DRIVER_GT911` as support.
- The Arduino mock is honest about its limits: `analogRead()` returns `0`, `digitalWrite()` is a no-op, `micros()` has millisecond resolution (it is `millis() * 1000`).
- **No gamepad exists on any target.** SDL2 offers `SDL_GameController` on desktop and web; the engine does not use it.

## Display

| Target | Driver | Notes |
|---|---|---|
| ESP32 | `TFT_eSPI_Drawer`, `U8G2_Drawer` | DMA blocks, optional RGB444 (`PIXELROOT32_TFT_12BIT_COLOR`), monochrome path |
| Desktop / Web | `SDL2_Drawer` | RGB565 streaming texture |

The important degradation is not the driver, it is the framebuffer:

- `getSpriteBuffer()` returns `nullptr` on `SDL2_Drawer` and `U8G2_Drawer`. Without a logical 8bpp buffer:
  - the direct 1bpp/2bpp/4bpp raster path is unavailable and drawing goes through `drawPixel`;
  - dirty regions have nothing to clear selectively and are skipped;
  - `StaticTilemapLayerCache` and `StaticLayerSnapshot` cannot snapshot, so every layer is drawn every frame.
- `examples/animated_tilemap` documents the behaviour for consumers: with a `nullptr` sprite buffer, "all layers drawn every frame; snapshot unused".
- U8G2 is **not compiled outside ESP32**. A monochrome game is an ESP32 game: `DisplayConfig` fails with `#error` when no driver is available, rather than handing out a null surface.

## Audio

| Target | Scheduler | Where synthesis runs |
|---|---|---|
| ESP32 | `ESP32AudioScheduler` | FreeRTOS audio task created by the I2S/DAC backend |
| Desktop | `NativeAudioScheduler` | Dedicated `std::thread`, PCM through a lock-free ring |
| Web | `DefaultAudioScheduler` | The SDL2 audio callback — no extra thread |

- The web path cannot use `std::thread` without `SharedArrayBuffer` and COOP/COEP headers, which would break "open a link and play". Synthesis therefore runs inside the callback, with a per-block budget (~2.9 ms at 44100/128).
- Browser autoplay is solved by Emscripten's SDL2 port (`autoResumeAudioContext()` resumes on the first `keydown`/`mousedown`/`touchstart`), not by the engine. A page with no user gesture is silent by browser policy, not by bug.
- The quality of the web scheduler has been measured structurally (`underruns`, non-zero peak, block cadence) and the audio has been listened to. What is **not** measured is the p99 synthesis time per callback on a slow machine.
- With `PIXELROOT32_ENABLE_AUDIO=0` the subsystem is compiled out on every target.

## Platform services

| Service | ESP32 | Desktop | Web | Evidence |
|---|---|---|---|---|
| Core count | ✅ from `sdkconfig` | 🟡 reports 4 | 🟡 reports 4 | `PlatformCapabilities.cpp` |
| WiFi / Bluetooth flags | ✅ from `sdkconfig` | ❌ | ❌ | `PlatformCapabilities.cpp` |
| Free heap | ✅ `ESP.getFreeHeap()` | ❌ `RAM: N/A` | ❌ `RAM: N/A` | `Engine.cpp` debug overlay |
| RNG | ✅ `esp_random()` | ⚠️ clock + counter + MAC | ⚠️ same | `NetSession.cpp` |
| `millis` / `delay` | ✅ | ⚠️ SDL ticks; `delay()` really sleeps | ⚠️ SDL ticks; `delay()` **busy-waits** | `MockArduino.h`, SDL2 port |
| GPIO, ADC, I²C, SPI | ✅ | ❌ no-op / loopback | ❌ no-op / loopback | `MockArduino.h`, `MockSPI.h` |

- **`PlatformCapabilities` is inferred, not measured, outside ESP32.** Desktop is hardcoded to "4 cores, dual-core true"; the browser lands in that same branch through `PLATFORM_NATIVE`. Treat `coreCount` as advisory on secondary targets: the browser is single-threaded.
- The engine has **no I²C (`Wire`) support on any target** today: the capacitive path is the GT911 placeholder above, and that is where the bus transactions would live. Outside the Arduino core, `MockSPI` supplies a loopback `SPIClass` that defines a global `SPI` so SPI-dependent code compiles; it talks to no bus.
- No watchdog, OTA, deep-sleep or battery API is exposed by the engine on any target.

## Loop and timing

- One frame is one `runFrame()`: platform events → up to `MAX_STEPS_PER_FRAME` steps of `FIXED_STEP_MS` (16 ms) → at most one draw + present. Identical on all targets, so simulation does not depend on refresh rate.
- The host loop differs: `emscripten_set_main_loop_arg` on the web (requestAnimationFrame, ~60 Hz), `while (runFrame())` on desktop (vsync), one call per Arduino `loop()` on ESP32.
- The web loop does not sleep at the end of a frame: `requestAnimationFrame` already returns control to the browser, and `SDL_Delay()` would not sleep here anyway (without Asyncify, SDL spins on the clock). `runFrame()` calls `SDL_Delay(1)` on desktop and `yield()` on ESP32 only. Measured on `examples/sprites` over 300 frames with the same harness (macOS, Chromium): the frame callback went from p50/min 1000 µs to p50 100 µs, mean 88 µs.

## Build and tooling

| Piece | Reality |
|---|---|
| PlatformIO envs | ESP32 boards and `native_*`. **There is no `web` env.** |
| Web build | `tools/web/build_web.py` rewrites native's `compile_commands.json` to `em++` and links with `-sUSE_SDL=2` |
| CMake | Desktop only: it excludes `src/drivers/esp32/`, defines `PLATFORM_NATIVE`, links SDL2 |
| Windows (CMake) | Not verified; no installer. The decision and what it would take are in the [packaging guide](../guide/packaging.md) |
| Physics / grid limits | `[base_esp32]` uses tighter limits than `native` through build flags, so the same game can hit different ceilings per target |
| Tests | Web has no dedicated test environment; `test/test_config.h` classifies a non-native build as an ESP32 test build |

The web build inheriting native's compile database is deliberate (same defines, same flags) and is also the root of the main simulation issue below.

## Simulated, not implemented

These are the cases where a secondary target *appears* to have a capability and does not. They are the reason this page exists.

| # | What happens | Where | Failure mode |
|---|---|---|---|
| 1 | The browser inherits `-DPLATFORM_NATIVE` and therefore desktop branches, including `PlatformCapabilities` (4 cores) and the networking guard | `tools/web/build_web.py`, `platformio.ini`, `PlatformCapabilities.cpp`, `PlatformDefaults.h` | Runtime, no diagnostic |
| 2 | Networking on the web compiles and links the in-process loopback: "network" that never leaves the tab | as above | Runtime |
| 3 | XPT2046 on desktop/web is a stub (`readRawX/Y` = 2048, DMA requests "succeed") | `XPT2046Adapter.cpp` | No-op |
| 4 | GT911 answers `0` on every target | `GT911Adapter.h` | No-op |
| 5 | `analogRead()` returns 0, `digitalWrite()` does nothing, `micros()` is milliseconds | `MockArduino.h` | No-op |
| 6 | A web build without `-lidbfs.js` runs with persistence disabled after one warning | `WebStorage.h` | No-op |
| 7 | `delay()` on the web does not sleep; `SDL_Delay` spins. Game code that calls `delay()` pays it — the engine's own frame loop no longer does | SDL2 port / `Engine.cpp` | Runtime (CPU) |
| 8 | `PR32_HAS_FPU_MACRO` is `1` for any non-IDF target through a fallback | `EngineConfig.h` | Runtime (arithmetic assumption) |

Two of these are latent, not observed in a build: the browser's 4-core claim is inferred from the compile database it reuses (no web binary has been inspected for it), and no example enables networking on the web out of the box.

## Rules for game code

1. **Ask for a capability, never for a platform.** `PIXELROOT32_HAS_HOST_FS`, `PIXELROOT32_HAS_SDL`, `PIXELROOT32_HAS_ARDUINO`. Only use `PIXELROOT32_PLATFORM_*` when the question is genuinely "which board is this".
2. **Never branch on `PLATFORM_NATIVE` in new code.** It means "not embedded" to one reader and "desktop" to another, and it is true on the web.
3. **Fail loud or log.** A capability that is missing must produce `#error` at build time or a one-time log at runtime. Never a silent no-op.
4. **Do not treat a stub as support.** Today that means GT911 and gamepads; both compile and do nothing.
5. **A web game is a single-process game.** If the design needs the radio, the design is ESP32-first and the browser demo is a protocol simulation.

## Related

- [Platform compatibility guide](../guide/platform-compatibility.md) — ESP32 variants, memory, audio hardware per chip
- [Configuration flags](../api/config.md) — every `PIXELROOT32_ENABLE_*` and its default
- [Platform API](../api/platform.md) — macros that exist, per target
- [Networking subsystem](./networking-subsystem.md) — transport, reliability, convergent counter
- [Persistence subsystem](./persistence-subsystem.md) — record envelope, adapters, flash stalls
- [Touch input](./touch-input.md) — XPT2046 pipeline and calibration
- [Driver layer](./layer-drivers.md) — TFT_eSPI, U8G2, SDL2
- [The examples in the browser](../guide/web-demos.md) — build recipe, gallery, what the web build proves
