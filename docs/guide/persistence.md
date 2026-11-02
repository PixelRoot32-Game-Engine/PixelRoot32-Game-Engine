# Saving data in your game (persistence)

The `pixelroot32::persistence` module keeps game data between runs: settings,
high scores, unlocks and small save slots. On the ESP32 it stores them in NVS
(the same flash area Arduino `Preferences` uses); on native (PC/SDL2) builds it
writes one small file per record; in tests it keeps them in RAM.

This page is task-oriented. For the envelope layout, atomicity per medium,
memory and limits, see the
[Persistence subsystem](../architecture/persistence-subsystem.md)
(authoritative). For every signature, see the
[Persistence API](../api/persistence.md) and the headers.

> **The one rule:** the engine never saves on its own. Every write is a call
> your game makes, and on the ESP32 a flash write freezes flash-resident code
> on both cores for a few milliseconds. Save at a safe point: a menu, a pause
> screen, a level transition.

**Implementation path:** [enable](#1-enable-the-module) ->
[set up the stores](#2-set-up-the-stores) ->
[save and load settings](#3-settings-records) ->
[save slots](#4-save-slots) ->
[change the record later](#5-schema-versions-and-migration) ->
[commit at a safe point](#6-when-to-save) ->
[test on native](#7-testing) ->
[ship checklist](#9-ship-checklist).

---

## 1. Enable the module

```ini
build_flags =
    -D PIXELROOT32_ENABLE_PERSISTENCE=1
```

The flag defaults to `0`. With it off, the module contributes no symbols and
no `Engine` bytes, and the headers compile to nothing. There are no other
macros: capacities are constructor arguments or template parameters, so you
pay only for the stores you create.

---

## 2. Set up the stores

The game owns everything: it constructs the stores, initialises them, and
binds them to the two roles of a `PersistenceService` - **settings** (an
`IKeyValueStore`) and **saves** (an `ISlotStore`). `Engine` is not involved.

### ESP32: NVS

```cpp
#include <persistence/PersistenceService.h>
#include <drivers/esp32/Esp32NvsStore.h>

using namespace pixelroot32::persistence;

// Namespace "mygame" (<= 15 chars, [A-Za-z0-9_-]), blobs up to 64 bytes.
Esp32NvsKeyValueStore settingsStore("mygame", 64);
// Three slots of up to 256 bytes, stored as keys save0, save1, save2.
Esp32NvsSlotStore saveStore("mygame", "save", 3, 256);
PersistenceService persistence;

void setupPersistence() {
    if (settingsStore.init() == StoreResult::Ok) {
        persistence.bindSettings(&settingsStore);
    }
    if (saveStore.init() == StoreResult::Ok) {
        persistence.bindSaves(&saveStore);
    }
}
```

- `init()` initialises the `nvs` partition if nobody has yet and opens the
  namespace. It **never erases** anything. A partition NVS cannot use as it is
  answers `Corrupt`; erasing it (`nvs_flash_erase_partition`) is your decision,
  because it wipes every namespace on it.
- Pick a namespace of your own and do not share it with unrelated data. A
  fourth constructor argument selects another NVS partition label.
- NVS suits small records: settings, scores, a few hundred bytes of save. Large
  save games need another medium (see the
  [subsystem's future media](../architecture/persistence-subsystem.md#10-future-media)).

If your game already stores a blob with Arduino `Preferences::putBytes()`,
`getBlob()` over the same namespace and key reads it back byte for byte - read
the old record once, then rewrite it in the enveloped format of section 3.

### Native (PC/SDL2): host files

```cpp
#include <persistence/PersistenceService.h>
#include <drivers/native/HostFileStore.h>

using namespace pixelroot32::persistence;

HostFileKeyValueStore settingsStore("save_data", "mygame", 64);  // save_data/mygame/
HostFileSlotStore     saveStore("save_data", "save", 3, 256);    // save_data/save/
PersistenceService    persistence;
```

`init()` and binding are the same as on the ESP32; `init()` creates the
directories. Each key or slot is one file, written to a temp file and moved
into place, so a crash keeps the old value or the new one. Data survives
between runs of the simulator.

### One game, both platforms

Only the store definitions differ. Keep the rest of the game on the service:

```cpp
#if defined(ESP32)
Esp32NvsKeyValueStore settingsStore("mygame", 64);
#elif defined(PLATFORM_NATIVE)
HostFileKeyValueStore settingsStore("save_data", "mygame", 64);
#endif
```

### Nothing bound

If a store fails to initialise, leave its role unbound. `persistence.settings()`
and `persistence.saves()` never return null: an unbound role answers
`Unavailable` to every call, and the game runs on its defaults.
`hasSettings()` / `hasSaves()` tell you which roles are live.

---

## 3. Settings records

Store a settings record as one blob wrapped in the engine's envelope. The
envelope adds 16 bytes: a magic, the envelope version, **your schema version**,
the payload length and a CRC-32 over all of it. On load, a damaged or truncated
record answers `Corrupt` instead of handing you garbage.

```cpp
constexpr uint16_t kSettingsSchema = 1;

struct Settings {
    uint8_t volume = 8;
    uint8_t brightness = 12;
    bool invertY = false;
};

StoreResult saveSettings(const Settings& s) {
    // Write fields one by one: never store a struct with memcpy, its padding
    // and layout differ between the PC and the ESP32.
    const uint8_t payload[3] = {s.volume, s.brightness, uint8_t(s.invertY ? 1 : 0)};
    uint8_t scratch[RecordCodec::kHeaderSize + sizeof(payload)];
    const StoreResult r = persistence.saveSettingRecord(
        "cfg", kSettingsSchema, payload, sizeof(payload), scratch, sizeof(scratch));
    if (r != StoreResult::Ok) {
        return r;
    }
    return persistence.settings().commit();
}

Settings loadSettings() {
    Settings s;  // defaults
    uint8_t scratch[RecordCodec::kHeaderSize + 32];
    const uint8_t* p = nullptr;
    RecordInfo info{};
    const StoreResult r =
        persistence.loadSettingRecord("cfg", scratch, sizeof(scratch), p, info);
    if (r == StoreResult::Ok && info.schemaVersion == kSettingsSchema &&
        info.payloadSize == 3) {
        s.volume = p[0];
        s.brightness = p[1];
        s.invertY = p[2] != 0;
    }
    return s;
}
```

- The scratch buffer is yours: the save needs `kHeaderSize + payload size`
  bytes; the load needs at least the stored record size, or it answers
  `TooLarge`. A stack array is the intended use - the module allocates nothing.
- `payload` points inside `scratch`; copy the fields out before `scratch` goes
  out of scope.
- What each result means on load:

| Result | What to do |
|--------|------------|
| `Ok` | Check `info.schemaVersion`, then read the payload |
| `NotFound` | First run: use defaults |
| `Corrupt` | Damaged record: use defaults (and save them at the next safe point) |
| `TooLarge` | Your scratch buffer is smaller than the stored record |
| `Unavailable` | No store bound: use defaults |
| `IoError`, `NoSpace` | Medium problem: report it, keep running |

For a single number you do not need a record: `settings().setI32("hiscore",
value)` and `getI32()` work directly (`setU32`/`getU32` for unsigned values).
They have no envelope, so prefer a record for anything with several fields.

---

## 4. Save slots

Slots are numbered records, `0..slotCount()-1`. The calls mirror the settings
ones:

```cpp
StoreResult saveSlot(uint16_t slot, const uint8_t* data, size_t size) {
    uint8_t scratch[RecordCodec::kHeaderSize + 256];
    return persistence.saveRecord(slot, kSaveSchema, data, size, scratch, sizeof(scratch));
}

bool loadSlot(uint16_t slot, uint8_t* out, size_t outSize) {
    uint8_t scratch[RecordCodec::kHeaderSize + 256];
    const uint8_t* p = nullptr;
    RecordInfo info{};
    if (persistence.loadRecord(slot, scratch, sizeof(scratch), p, info) != StoreResult::Ok ||
        info.schemaVersion != kSaveSchema || info.payloadSize != outSize) {
        return false;
    }
    memcpy(out, p, outSize);
    return true;
}
```

- `saves().query(slot, size)` tells you whether a slot is used (and its size)
  without reading it - handy for a "Continue / Empty" slot menu.
- `saves().erase(slot)` empties a slot.
- A slot write is **replace-or-keep**: if the power fails mid-write, the slot
  holds the old save or the new one, never a mix.
- On the ESP32 slot writes reach flash immediately; there is no separate
  commit for slots.

---

## 5. Schema versions and migration

The schema version is yours. The engine stores it and reports it on load; it
never rejects a record for its version. When you change the layout of a
record:

1. Bump the constant (`kSettingsSchema = 2`).
2. In the loader, handle every version you still accept:

```cpp
if (r == StoreResult::Ok) {
    if (info.schemaVersion == 2 && info.payloadSize == 4) {
        // current layout
    } else if (info.schemaVersion == 1 && info.payloadSize == 3) {
        // old layout: read it, fill the new field with its default
    }
    // anything else: defaults
}
```

3. Save in the new version at the next safe point.

---

## 6. When to save

On the ESP32, NVS writes are expensive and block more than the caller: while
flash is written, the flash cache is off on **both cores**, so the render loop
and any audio or asset code running from flash stall. Measured on the CYD
(ESP32-2432S028, Arduino-ESP32 2.0.17):

| Operation | Typical | Worst seen |
|-----------|---------|------------|
| `setBlob` of 8-64 bytes | ~4 ms | ~13 ms |
| `commit()` | ~12 us | 21 us |

The cost is in the set (NVS has no write-back cache); `commit()` is nearly
free. One set costs about a quarter of a 60 fps frame on average and most of
one at worst, and a save of several values adds up.

- Save from a menu, a pause screen, a "Saving..." screen or a level
  transition - never from inside a gameplay frame, an audio callback or a
  timer.
- Group the sets of one save, then call `settings().commit()` once.
- Do not save every frame or on every value change; save when the player
  leaves the options screen.
- The service never calls `commit()` for you.

On native builds every write is a small file write; the rule still keeps
behaviour the same on both platforms.

---

## 7. Testing

Use the memory stores in unit tests: same interfaces, no hardware, no files.

```cpp
#include <persistence/MemoryStores.h>

MemoryKeyValueStore<8, 64> settings;   // 8 keys, blobs up to 64 bytes
MemorySlotStore<3, 256> saves;         // 3 slots of up to 256 bytes
persistence.bindSettings(&settings);
persistence.bindSaves(&saves);

settings.setFailWrites(true);          // every write answers IoError
saves.corruptByte(0, 5);               // damage a stored byte -> load is Corrupt
```

They have no `init()`, allocate nothing, and carry test hooks:
`setFailWrites()`, `corruptByte()`, and counters `writes()` (both) and
`commits()` (key-value) to check that your game saves - and commits - exactly
when it should.

Run the engine's own persistence suites with:

```bash
pio test -e native_test_persistence
```

---

## 8. Custom partition tables: the `0xE000` trap

The Arduino PlatformIO build flashes `boot_app0.bin` (the `otadata` image) at
`0xE000` on **every upload**. With the default partition tables that is the
`otadata` partition. If your custom table has **no `otadata`** and its `nvs`
partition covers `0xE000` - for example `nvs` at `0x9000` with size `0x6000` (it spans `0x9000`-`0xF000`) -
every upload overwrites the last NVS page, and saved settings can disappear
after a reflash.

Fix it in one of two ways:

- Make the `nvs` row end before `0xE000`, or put an `otadata` row at `0xE000`.
- Or drop that image from the upload with an extra script. The engine's
  `tools/persist_nvs_smoke/keep_nvs.py` does exactly that and its README
  explains how.

The default Arduino partition tables are not affected.

---

## 9. Ship checklist

- [ ] `PIXELROOT32_ENABLE_PERSISTENCE=1` only in the environments that save.
- [ ] Namespaces and keys are 1..15 characters from `[A-Za-z0-9_-]`, and the
      namespace is yours alone.
- [ ] Every record goes through `saveSettingRecord`/`saveRecord` with a schema
      version, and fields are written one by one (no struct `memcpy`).
- [ ] Loads handle `NotFound`, `Corrupt` and an unknown schema version with
      defaults.
- [ ] Saves and `commit()` happen only at safe points; nothing saves per
      frame.
- [ ] The game still runs when `init()` fails (roles left unbound).
- [ ] A custom partition table keeps `0xE000` out of `nvs`, or the upload drops
      `boot_app0.bin`.
- [ ] Settings survive a reflash of the app on the real board.

---

## 10. Next steps

- [Persistence subsystem](../architecture/persistence-subsystem.md) - envelope,
  atomicity per adapter, error model, memory, limits and future media.
- [Persistence API](../api/persistence.md) - every type and signature.
- [Configuration](../api/config.md) - the build flag.
