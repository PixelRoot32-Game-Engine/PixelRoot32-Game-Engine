# Persistence Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the
> capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be
> 100% functional or finished**; some features can be incomplete, experimental, or work in
> progress.

One scene, one subject: a settings record and a save slot behind
`PIXELROOT32_ENABLE_PERSISTENCE`, with every call's `StoreResult` printed.

The scene owns both stores and the `PersistenceService`. At boot it loads the
records, reports the result of each call, and falls back to defaults when a
record is missing or from another schema. It writes only when the player
presses **A**, so one press is a complete save and the next launch proves the
write reached the medium.

## What it shows

- **Two stores, one service.** `Esp32NvsKeyValueStore` +
  `Esp32NvsSlotStore` on the board; `HostFileKeyValueStore` +
  `HostFileSlotStore` on native. Both are bound to the same
  `PersistenceService`.
- **A settings record** (`cfg`, schema 1, three bytes: volume, brightness,
  invert Y) through `saveSettingRecord()` / `loadSettingRecord()`.
- **A save-slot record** (slot 0, schema 1, three bytes: level, presses) through
  `saveRecord()` / `loadRecord()`.
- **`StoreResult` on every step.** No call returns a bare boolean; the
  transcript prints the result's own name (`Ok`, `NotFound`, `Corrupt`, and the
  rest).
- **The record envelope.** A direct `RecordCodec::encode()` / `decode()` pair
  reports the stored schema version and payload length, and a deliberately
  truncated record reports `Corrupt`.
- **A safe save point.** Nothing writes per frame; `settings().commit()` runs
  once after the settings set, on the A press.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_PERSISTENCE=1`** — the persistence headers and the two
  stores. It defaults to `0`, and without it this example fails the build with
  an explicit `#error`.
- **Audio, particles, physics and the UI system are `0`** — the example does
  not use them, so their code stays out of the image. Every other engine flag
  keeps its default; see
  [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h).

## Platforms

| Environment | Notes |
|-------------|-------|
| **`native`** | SDL2 window, 240×240 logical size. Records become files under `save_data/`. |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. Records go to NVS. A is **GPIO 13**. |

## Controls

- **A** (`Space` on native, GPIO 13 on the board) — mutate both records, save
  them, and read both back.
- Nothing else. The scene has no other input, and no input means no write.

One press makes these changes, so the next boot has something different to
show:

| Record | Field | Change per press |
|--------|-------|------------------|
| settings | `volume` | `(volume + 1) % 11` |
| settings | `brightness` | `(brightness + 1) % 16` |
| settings | `invertY` | toggles |
| slot 0 | `presses` | `+1` |
| slot 0 | `level` | `1 + presses / 5` |

## What to look for in the output

Every step prints one stable, greppable line. `PERSIST,` starts every line, so
a transcript can be filtered with `grep '^PERSIST,'` (or `Select-String` on
Windows).

The first launch on a clean install has no records yet:

```text
PERSIST,platform,native
PERSIST,boot,settings_store_init,Ok
PERSIST,boot,saves_store_init,Ok
PERSIST,boot,bound,settings=1,saves=1,slots=3,max_record=256
PERSIST,boot,settings_load,NotFound
PERSIST,boot,settings_defaults,reason=NotFound
PERSIST,boot,slot_load,NotFound
PERSIST,boot,slot_defaults,reason=NotFound
PERSIST,codec,encode,Ok,bytes=19
PERSIST,codec,decode,Ok,schema=1,payload=3
PERSIST,codec,truncated,Corrupt
PERSIST,ready,input=A
```

Press A once and the same run shows the write, the commit and the read-back:

```text
PERSIST,input,A,action=save
PERSIST,save,settings_write,Ok
PERSIST,save,settings_commit,Ok
PERSIST,save,slot_write,Ok,slot=0,bytes=19
PERSIST,save,settings_reload,Ok,schema=1,payload=3
PERSIST,save,settings_now,volume=9,brightness=13,invert_y=1
PERSIST,save,slot_reload,Ok,schema=1,payload=3
PERSIST,save,slot_now,level=1,presses=1
```

Restart (or stop and rerun the native binary) and the boot lines change from
`NotFound` to the stored values:

```text
PERSIST,boot,settings_load,Ok,schema=1,payload=3
PERSIST,boot,settings_values,volume=9,brightness=13,invert_y=1
PERSIST,boot,slot_load,Ok,schema=1,payload=3
PERSIST,boot,slot_values,level=1,presses=1
```

That is the whole demonstration: the value the second run reads is the value
the first run wrote.

## Where the data lives

### Native: host files

`HostFileKeyValueStore("save_data", "mygame", 64)` and
`HostFileSlotStore("save_data", "save", 3, 256)` create:

```text
save_data/
├── mygame/
│   └── cfg.kv       the settings record, one type tag + the 19-byte record
└── save/
    └── 0.slot       the save-slot record, 19 bytes
```

The paths are relative to the working directory the binary runs from. From
`examples/persistence`, running `.pio/build/native/program` puts them in
`examples/persistence/save_data/`. Every write goes to a temporary file and is
moved into place, so a crash leaves the old file or the new one, never a mix.
`commit()` returns `Ok` here because each set is already flushed and in place.

### ESP32: NVS

`Esp32NvsKeyValueStore("mygame", 64)` and
`Esp32NvsSlotStore("mygame", "save", 3, 256)` use the **`mygame`** namespace on
the default **`nvs`** partition:

| Key | Holds |
|-----|-------|
| `cfg` | the settings record |
| `save0` | the save-slot record (slot 0) |
| `save1`, `save2` | the two unused slots |

`init()` initialises the partition if nobody has yet and never erases it.
Settings reach flash on the set and again on `commit()`; slot writes reach
flash immediately, with no separate commit. While flash is written the cache is
off on both cores, which is why both happen on the A press and nowhere else.

## Schema versions and what `RecordCodec` reports

The schema version is the game's. `RecordCodec` stores it in the 16-byte
envelope and reports it back on load; it **never rejects a record for its
version**. The scene does the judging:

- load result `Ok` and `schemaVersion == 1` and `payloadSize == 3` → use the
  fields;
- load result `Ok` with any other version or size → `settings_defaults` (or
  `slot_defaults`) and keep running;
- `NotFound` → first run, defaults;
- `Corrupt` → damage, defaults; the next save rewrites it.

The direct codec pair in the boot log makes that visible without a medium:
`encode` writes 16 + 3 = 19 bytes, `decode` reports `schema=1,payload=3`, and
decoding the same buffer one byte short reports `Corrupt` because the length
field disagrees and the CRC cannot match. Note that `RecordCodec` reports the
version rather than checking it against a fixed value, which is what lets you
bump `kSettingsSchema` and migrate old records in your own loader.

## The `0xE000` partition trap

The default `esp32dev` partition table is not affected, so this example saves
and reloads without extra setup. A **custom** table can still lose NVS data on
every upload, and saving settings is exactly when that hurts.

The Arduino PlatformIO build flashes `boot_app0.bin` (the `otadata` image) at
`0xE000` on **every upload**. With the default partition tables that is the
`otadata` partition. If your custom table has **no `otadata`** and its `nvs`
partition covers `0xE000` — for example `nvs` at `0x9000` with size `0x6000`,
spanning `0x9000`–`0xF000` — each upload overwrites the last NVS page and
saved settings can disappear after a reflash.

Fix it by making the `nvs` row end before `0xE000`, by putting an `otadata` row
at `0xE000`, or by dropping that image from the upload with
[the engine's `keep_nvs.py`](../../tools/persist_nvs_smoke/keep_nvs.py). The
full explanation is in
[section 8 of the persistence guide](../../docs/guide/persistence.md#8-custom-partition-tables-the-0xe000-trap).

## File structure

```text
src/
├── PersistenceExampleScene.h/.cpp  — stores, service, records, transcript
├── main.cpp                        — selects the platform wiring
└── platforms/
    ├── native.h                    — SDL2 engine wiring
    └── esp32_dev.h                 — ESP32 engine wiring
```

## Documentation links

- [Saving data in your game (persistence)](../../docs/guide/persistence.md)
- [Persistence API](../../docs/api/persistence.md)
- [Persistence subsystem](../../docs/architecture/persistence-subsystem.md)
- [Architecture](../../docs/architecture/architecture-index.md)

## Build

Run from **`examples/persistence`**:

```bash
pio run -e native
pio run -e esp32dev
```

Run the PC build from the same folder so `save_data/` lands there:

```bash
.pio/build/native/program
```

> **Windows note.** The `native` environment compiles with MSYS2's MinGW `g++`.
> If `pio run -e native` fails with a bare `*** [...] Error 1` and no compiler
> diagnostic, `g++` is not on `PATH`. Add it before building:
> `export PATH="/c/msys64/mingw64/bin:$PATH"`.

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
pio device monitor --baud 115200
```

The serial monitor shows the same `PERSIST,` lines the PC build prints to the
terminal.
