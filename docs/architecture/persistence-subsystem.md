# Persistence Subsystem

The optional `pixelroot32::persistence` module lets a game keep small data
between runs: settings, high scores, unlocks and, where the medium allows, save
slots. It ships behind `PIXELROOT32_ENABLE_PERSISTENCE`, defaults to `0`, and
contributes no symbols, no `Engine` bytes and no `Scene` vtable change when it
is off.

The module has three adapters in v1: ESP-IDF NVS on the ESP32, one file per
record on native (PC) builds, and fixed in-RAM tables for tests. Above them,
`RecordCodec` wraps game payloads in a self-checking envelope and
`PersistenceService` binds a settings store and a saves store to two roles.

> **Rule, stated once:** the engine never writes in the background. Every set,
> erase and `commit()` is a call the game makes. On the ESP32 a flash write
> stalls flash-resident code on both cores for milliseconds, so save at a safe
> point - a menu, a pause screen, a level transition - never mid-gameplay.
>
> For setup and usage, start with the [Persistence guide](../guide/persistence.md).

---

## 1. Models and media

```text
Game code
   |  settings(), saves()                (roles, bound at setup)
PersistenceService  ------------------  RecordCodec (envelope + CRC-32)
   |
   |-- IKeyValueStore    typed ints, blobs, erase, commit
   `-- ISlotStore        N numbered records, replace-or-keep per slot
          ^
Adapters (one per medium)
   |-- Esp32NvsKeyValueStore / Esp32NvsSlotStore   drivers/esp32   (ESP32)
   |-- HostFileKeyValueStore / HostFileSlotStore   drivers/native  (PLATFORM_NATIVE)
   `-- MemoryKeyValueStore   / MemorySlotStore     persistence/    (any target)
```

A **storage model** is an interface describing the shape of the data:
`IKeyValueStore` for small named values, `ISlotStore` for a fixed number of
numbered records. A **medium** is an adapter that implements one or more models
over one kind of storage.

The split exists so a game never names a medium in its game logic. Code that
saves a settings record talks to an `IKeyValueStore&`; whether that is NVS on
the board or a file on the PC is decided once, where the stores are
constructed. Moving saves to another medium later - an SD card, an I2C EEPROM -
adds an adapter and leaves the interfaces and the game logic untouched.

| Layer | Owns | Knows nothing about |
|-------|------|---------------------|
| Game | What is stored, its schema version, migration, when to commit | Media, NVS, files |
| `PersistenceService` + `RecordCodec` | Roles, envelope, CRC, null-store fallback | Media |
| `IKeyValueStore`, `ISlotStore` | Shape of the data | Envelope, game schema |
| Adapters | One medium: mapping, atomicity, error translation | Game data, envelope |

Adapters move opaque bytes. The envelope is validated in one place, above
them, so every medium gets the same corruption checks.

### Ownership

`Engine` does not own any of it. The game constructs its stores and the
service, calls each adapter's `init()`, and binds the stores at setup. A game
may also skip the service and hold a store reference directly; the service
only adds the envelope helpers and the never-null role accessors.

---

## 2. The storage models

### `IKeyValueStore`

Named values: `int32_t`, `uint32_t` and blobs. Strings go through blobs;
floats are not stored as such (encode them in the game's record).

- **One value per key, last write wins.** A set of any type replaces whatever
  the key held, so a typed get on a key that now holds another type answers
  `NotFound`.
- **A missing key is `NotFound`** and leaves every output untouched.
- **Reads take a capacity.** `getBlob()` never writes past `capacity`; when the
  blob is larger it answers `TooLarge`, writes nothing and reports the stored
  size. `capacity == 0` is the size query.
- **`commit()` is the durability point**, and the game's decision.

### `ISlotStore`

A fixed number of numbered records, `0..slotCount()-1`, each up to
`maxRecordSize()` bytes.

- Out-of-range slots answer `InvalidArgument` before the medium is touched.
- An empty slot reads as `NotFound` with `stored = 0`. A zero-size record
  occupies its slot.
- **`write()` is replace-or-keep:** after a failed or interrupted write,
  `read()` returns the previous record or the new one, never a mix. Each
  adapter states how it guarantees this (section 5).

---

## 3. The record envelope

`RecordCodec` wraps a game payload in a 16-byte header, written byte by byte in
little-endian order. Records are never `sizeof(T)` struct dumps: padding and
layout differ between the host and the ESP32, and the envelope fixes byte
order explicitly.

| Bytes | Field | Value |
|-------|-------|-------|
| 0-3 | Magic | ASCII `P` `R` `S` `T` |
| 4 | Envelope version | `RecordCodec::kEnvelopeVersion` (`1`) |
| 5 | Reserved | `0` |
| 6-7 | Schema version | `uint16_t`, owned by the game |
| 8-11 | Payload length | `uint32_t` |
| 12-15 | CRC-32 | Over bytes 0..11, then the payload |
| 16.. | Payload | Game bytes |

**CRC coverage.** The checksum is CRC-32/IEEE (the zlib/PNG CRC, reflected
polynomial `0xEDB88320`, check value `0xCBF43926` for `"123456789"`). It covers
the first twelve header bytes as well as the payload, so a flipped schema
version or length field is caught even though neither is compared against a
fixed value. A 16-entry nibble table keeps its flash cost to 64 bytes.

**Decode** rejects a truncated record, a wrong magic, an unknown envelope
version, a non-zero reserved byte, a length field that disagrees with the
record size, and a CRC mismatch - all as `Corrupt`.

**The schema version belongs to the game.** The codec stores it and reports it
on decode; it never judges it. A game that changes its record layout bumps its
schema version, and on load it migrates an older version (or falls back to
defaults) itself. There is no automatic migration.

---

## 4. Error model

Every store, adapter and codec call returns a `StoreResult`. There are no bare
`bool` results and no exceptions, so a caller can always tell a missing record
from a full medium or a damaged one.

| `StoreResult` | Meaning | Typical cause |
|---------------|---------|---------------|
| `Ok` | The operation completed | - |
| `NotFound` | The key or slot holds no record; outputs untouched | First run, erased key, a typed get on a key of another type |
| `TooLarge` | The data does not fit the caller's buffer or the medium's limit | Read buffer too small, blob above the store's maximum |
| `NoSpace` | The medium is full | NVS partition full, disk full, memory table full |
| `Corrupt` | Stored bytes failed validation | Envelope, length or CRC check; an NVS partition that needs an erase |
| `IoError` | The medium failed in a way the module cannot classify further | File system error, malformed host file, other NVS errors |
| `Unavailable` | No store is bound, or the medium is not initialised | Unbound role, `init()` not called or failed |
| `InvalidKey` | The key or namespace breaks the key rules | Too long, empty, or a character outside `[A-Za-z0-9_-]` |
| `InvalidArgument` | A null pointer, an out-of-range slot or another bad parameter | Programming error |

**Unbound roles never dereference null.** `PersistenceService::settings()` and
`saves()` return a built-in null store (`NullKeyValueStore`, `NullSlotStore`)
when nothing is bound. Every call on it answers `Unavailable`; the null slot
store reports `slotCount() == 0` and `maxRecordSize() == 0`. A game built for a
target without storage keeps running with defaults.

---

## 5. Adapters and atomicity

| Adapter | Medium | Replace-or-keep (S7) | `commit()` |
|---------|--------|----------------------|------------|
| `MemoryKeyValueStore<Entries, BlobBytes>`, `MemorySlotStore<Slots, RecordBytes>` | Fixed in-RAM tables | A write applies in full or not at all | Counts calls, `Ok` |
| `HostFileKeyValueStore`, `HostFileSlotStore` | One file per key or slot | Temp file, flush and sync, then one replace-existing move | `Ok`: every write is already in place |
| `Esp32NvsKeyValueStore`, `Esp32NvsSlotStore` | One ESP-IDF NVS namespace | Same-type set: one NVS write; cross-type set: not atomic (below) | `nvs_commit` |

### Memory

Header-only templates with no heap: capacities are template parameters, and
contents are lost with the object. They exist for tests and volatile use, and
carry test hooks outside the interfaces: `setFailWrites()`, `corruptByte()`,
`writes()` and (KV only) `commits()`.

### Host files

Files live under `<rootDir>/<nameSpace>/` (KV) or `<rootDir>/<prefix>/`
(slots). A key's file name escapes each uppercase letter as `^` plus its
lowercase letter, then `.kv` (`Ab_C` -> `^ab_^c.kv`), because host file systems
may ignore case and `^` is not a key character. A KV file holds a one-byte type
tag (1 = i32, 2 = u32, 3 = blob) followed by the value; a slot file (`2.slot`)
holds the record as is.

Every write goes to `<file>.tmp`, is flushed and synced (`_commit` on Windows,
`fsync` on POSIX), closed, and moved over the final file in one step -
`MoveFileExA` with `MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH` on
Windows, `rename()` on POSIX. A crash leaves the old file or the new one, plus
at worst a stale `.tmp` that reads never open and the next write overwrites.
A malformed file (empty, truncated, unknown tag, oversized) reads as `IoError`,
never as a silent `NotFound`.

`init()` creates the directory with its parents and must succeed before use;
until then every operation answers `Unavailable`. Paths are built in fixed
buffers of `kHostFileMaxPathBytes` (260, the Windows `MAX_PATH`, applied on
every host).

> **Status:** the POSIX branch (`rename`, `mkdir`, `fsync`, `ENOSPC`) has not
> yet been run outside Windows; Linux CI is its first check.

### ESP32 NVS

`Esp32NvsKeyValueStore` stores integers as NVS `I32`/`U32` items and blobs as
NVS blobs, over one namespace on a configurable partition label (default
`nvs`). `Esp32NvsSlotStore` stores slot `i` as the blob under key
`<prefix><i>` (`save0`, `save1`, ...); `init()` checks that the longest such
key fits 15 characters.

**Same-type writes are replace-or-keep.** A set is a single NVS write, and NVS
writes the new entry (for a blob, its data chunks and then its index) before it
marks the old one erased. After a power loss a read returns the old value or
the new one. Slot writes are always same-type writes.

**Cross-type writes are not.** NVS can hold one item per type under the same
key (observed on IDF 4.4.7: after `nvs_set_i32` then `nvs_set_blob` on one key,
both items stay and `nvs_get_i32` still returns the old integer). To keep the
one-value-per-key rule, a set probes the key and, when an item of another type
is there, erases it before writing the new one. A power loss between that erase
and the write loses the key. Games that never change a key's type are not
affected.

**`init()` never erases.** It initialises the partition when nobody has yet
(`nvs_flash_init_partition`; a partition the Arduino core already initialised
is fine) and opens the namespace read-write. A partition NVS cannot use as it
is (`ESP_ERR_NVS_NO_FREE_PAGES`, `ESP_ERR_NVS_NEW_VERSION_FOUND`) is reported as
`Corrupt` and left alone: erasing it would wipe every namespace on it, which is
the game's decision.

**Error mapping:** `NOT_FOUND` and `TYPE_MISMATCH` -> `NotFound`;
`NOT_ENOUGH_SPACE` -> `NoSpace`; `KEY_TOO_LONG` and `INVALID_NAME` ->
`InvalidKey`; `INVALID_LENGTH` and `VALUE_TOO_LONG` -> `TooLarge`;
`NOT_INITIALIZED` and `INVALID_HANDLE` -> `Unavailable`; anything else ->
`IoError`.

**Compatibility with `Preferences`.** A blob written by Arduino
`Preferences::putBytes(key, ...)` in namespace N reads back byte-identically
through `getBlob()` over namespace N, and the other way round (verified on the
board). This is the migration path for records a game already stores with
`Preferences`.

---

## 6. Writes and the ESP32 flash stall

Writes are explicit. No adapter, the service included, writes on its own:
`PersistenceService` never calls `commit()`, and nothing is flushed from a
timer or a destructor.

On the ESP32 every NVS write or page erase disables the flash cache on **both
cores** for its duration. Anything executing from flash - the render loop,
audio code not placed in IRAM, memory-mapped flash reads such as a WAD or asset
container - stalls until it ends. NVS has no write-back cache, so the stall
happens in the set or erase, not in `commit()`.

Measured on the CYD (ESP32-2432S028, Arduino-ESP32 2.0.17 / IDF 4.4.7, 50 writes
of a changing blob per size, `tools/persist_nvs_smoke`):

| Operation | Min | Mean | Max |
|-----------|-----|------|-----|
| `setBlob`, 8 B | ~1.7 ms | ~4 ms | ~13 ms |
| `setBlob`, 64 B | ~3 ms | ~4.3 ms | ~13.6 ms |
| `commit()` | 6 us | 12 us | 21 us |

One set costs about a quarter of a 60 fps frame on average and most of one at
worst, and a save of several values adds up. **Save at a safe point** - a menu, a pause screen,
a level transition - and group the sets of one save together before one
`commit()`. Never save from an audio callback, a timer or the middle of a
gameplay frame.

### `boot_app0.bin` and custom partition tables

The Arduino PlatformIO build always flashes `boot_app0.bin` (the 8 KB
`otadata` image) at `0xE000`. In the default tables that address is the
`otadata` partition. In a custom table whose `nvs` partition covers `0xE000` -
for example an `nvs` at `0x9000` with size `0x6000` (it spans `0x9000`-`0xF000`) and no `otadata` - every
upload overwrites the last NVS page and can destroy stored settings.

Either end the `nvs` row before `0xE000` (or add an `otadata` row there), or
drop that image from the upload. `tools/persist_nvs_smoke/keep_nvs.py` is a
PlatformIO extra script that does the latter; the smoke tool's README explains
it.

---

## 7. Memory

No heap on the device path. Every buffer is caller-owned or a fixed array
sized at compile time.

| Item | Size |
|------|------|
| `PersistenceService` | Four pointers (two store pointers, two null-store vtables): 16 B on ESP32, 32 B on a 64-bit host |
| `Esp32NvsKeyValueStore` | 24 B (handle, two string pointers, limits) |
| `Esp32NvsSlotStore` | 28 B |
| CRC table | 64 B of flash |
| Measured on the CYD, two NVS KV stores linked | `.dram0.bss` +48 B, `.dram0.data` +0 B; flash +2,760 B text, +680 B rodata |
| `HostFileKeyValueStore`, `HostFileSlotStore` | One 260-byte path buffer plus the limits each (native only) |
| `MemoryKeyValueStore`, `MemorySlotStore` | Their template capacities |

With the flag off the module costs nothing: every header body sits behind
`#if PIXELROOT32_ENABLE_PERSISTENCE`.

The record helpers take a scratch buffer from the caller: it needs
`RecordCodec::kHeaderSize + payload size` bytes. A stack array in the function
that saves is the intended use.

---

## 8. Threading

Single-threaded contract: every call comes from one task, the game loop.
Adapters and the service are not thread-safe and take no locks. A game that
saves from another task must serialise all access itself.

---

## 9. Limits

| ID | Limit |
|----|-------|
| L1 | **Keys and namespaces** are 1..15 bytes, each in `[A-Za-z0-9_-]` (`kMaxKeyLength`, `isValidKey()`). Fifteen is the NVS limit, applied to every medium so a key written on the host works on the board; the character set is safe as a file name on Windows and POSIX. An invalid key answers `InvalidKey` before the medium is touched. |
| L2 | **NVS namespaces** follow the same rule. Use one namespace per store and never share one with unrelated data; a slot store's `<prefix><index>` keys must also fit 15 characters. |
| L3 | **NVS is for small records.** Usable space is the `nvs` partition minus NVS overhead (the CYD's 24 KB partition holds about 20 KB). Large save games need a raw-partition or SD adapter. |
| L4 | **Cross-type writes on NVS are not atomic** (section 5). |
| L5 | **Typed values are `int32_t`, `uint32_t` and blobs.** Strings and floats go through the game's own record. |
| L6 | **No automatic schema migration.** The codec reports the version; the game migrates. |
| L7 | **No wear accounting** beyond what NVS does internally. |
| L8 | **No `IFileStore` yet.** Streaming files arrive with the first backend that needs them (SD). |
| L9 | **Arduino core range.** The NVS adapter uses `nvs_*` calls common to IDF 4.4 and 5.x; it has been built and run on Arduino-ESP32 2.0.17. Arduino-ESP32 3.x has not yet been built locally. |

---

## 10. Future media

Each future medium is an adapter behind the existing interfaces. None of them
changes `IKeyValueStore`, `ISlotStore`, `RecordCodec` or game code that saves
through a store reference.

| Medium | Model(s) | Replace-or-keep | Notes |
|--------|----------|-----------------|-------|
| SD card (FAT) | Slots, and a future `IFileStore` | Temp file + rename; FAT is not power-safe, so slot reads rely on the envelope to reject a torn record | Several KB of RAM for FATFS; removable media answers `Unavailable` |
| I2C EEPROM (AT24Cxx-class chip) | Slots, small KV | Two copies per slot (A/B with a sequence number); the envelope picks the valid one | Page-write limits and ~5 ms write cycles live in the adapter |
| Raw flash partition | Slots | A/B sectors per slot with a sequence number | The route for large save games on boards with 16 MB of flash |
| Mirrored slot store | Slots | Per underlying store | A `MirroredSlotStore` implementing `ISlotStore` over two others, reporting each result - a "save to two media" policy without a new interface |

`IFileStore` (open, read, write, close, remove, exists, size, replace) is
deliberately not shipped yet: an interface no backend validates would freeze an
untested design. It is added with the SD adapter.

---

## 11. Testing

| Suite | What it covers |
|-------|----------------|
| `test/unit/test_persist_flags/` | The flag and its mirrors match the build (runs flag on and flag off) |
| `test/unit/test_persist_codec/` | Envelope round trip, each corruption class, CRC check value, key rules |
| `test/unit/test_persist_contract/` | One adapter contract suite, run unchanged against every adapter in its table |
| `test/unit/test_persist_service/` | Roles, null stores, record helpers |
| `test/unit/test_persist_hostfile/` | Host file specifics: `init()`, file layout, case-distinct keys, interrupted writes, stale temp files, malformed files |

The contract suite is the heart of it. It runs on the host against the memory
and host-file adapters under `native_test_persistence`, and on the board
against the NVS adapters through `tools/persist_nvs_smoke` (`pio test -e
smoke`, which compiles the same test file from the engine's `test/` folder).
The smoke firmware adds what only hardware can show: `Preferences`
compatibility, raw IDF type behaviour, write timing and `nvs_get_stats`.

```bash
pio test -e native_test_persistence   # host: codec, contract, service, host files
cd tools/persist_nvs_smoke
pio test -e smoke                      # board: contract suite over NVS
pio run  -e smoke -t upload            # board: compatibility and timing report
```

---

## 12. Where to look

| What | Where |
|------|-------|
| Results, key rules | [`include/persistence/StoreTypes.h`](../../include/persistence/StoreTypes.h) |
| Key-value model | [`include/persistence/KeyValueStore.h`](../../include/persistence/KeyValueStore.h) |
| Slot model | [`include/persistence/SlotStore.h`](../../include/persistence/SlotStore.h) |
| Envelope and CRC | [`include/persistence/RecordCodec.h`](../../include/persistence/RecordCodec.h) |
| Service and null stores | [`include/persistence/PersistenceService.h`](../../include/persistence/PersistenceService.h) |
| Memory adapters | [`include/persistence/MemoryStores.h`](../../include/persistence/MemoryStores.h) |
| Host file adapters | [`include/drivers/native/HostFileStore.h`](../../include/drivers/native/HostFileStore.h) |
| NVS adapters | [`include/drivers/esp32/Esp32NvsStore.h`](../../include/drivers/esp32/Esp32NvsStore.h) |
| Contract suite | [`test/unit/test_persist_contract/`](../../test/unit/test_persist_contract/) |
| Board smoke tool | [`tools/persist_nvs_smoke/README.md`](../../tools/persist_nvs_smoke/README.md) |
| Task-oriented guide | [Persistence guide](../guide/persistence.md) |
| Conceptual API | [Persistence API](../api/persistence.md) |
| Build flag | [Configuration](../api/config.md) |
