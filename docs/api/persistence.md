# API Reference: Persistence Module

> **Source of truth:**
> - `include/persistence/StoreTypes.h`
> - `include/persistence/KeyValueStore.h`
> - `include/persistence/SlotStore.h`
> - `include/persistence/RecordCodec.h`
> - `include/persistence/PersistenceService.h`
> - `include/persistence/MemoryStores.h`
> - `include/drivers/esp32/Esp32NvsStore.h`
> - `include/drivers/native/HostFileStore.h`

## Overview

The persistence module keeps small game data between runs: settings, scores,
unlocks and small save records. It is compiled only when
`PIXELROOT32_ENABLE_PERSISTENCE=1` (default `0`) and lives in
`pixelroot32::persistence`, adapters included.

The public surface has four parts:

- Two storage models: `IKeyValueStore` (named integers and blobs) and
  `ISlotStore` (numbered records).
- Adapters, one per medium: NVS on the ESP32, one file per record on native
  builds, and in-RAM tables for tests.
- `RecordCodec`, which wraps a payload in a 16-byte self-checking envelope.
- `PersistenceService`, a role registry with envelope helpers.

`Engine` does not own any of it: the game constructs the stores and the
service. Writes are explicit - nothing is written in the background - and on
the ESP32 every flash write stalls flash-resident code on both cores, so save
at a safe point such as a menu. See the
[Persistence subsystem](../architecture/persistence-subsystem.md) for the
envelope layout, atomicity per adapter, measured costs and limits.

## Usage

### ESP32: settings in NVS

```cpp
#include "persistence/PersistenceService.h"
#include "drivers/esp32/Esp32NvsStore.h"

using namespace pixelroot32::persistence;

struct Settings {             // the game's own record
    uint8_t volume = 8;
    uint8_t brightness = 12;
};
constexpr uint16_t kSettingsSchema = 1;

Esp32NvsKeyValueStore settingsStore("mygame", 64);          // namespace, max blob
Esp32NvsSlotStore     saveStore("mygame", "save", 3, 256);  // keys save0..save2
PersistenceService    persistence;

void setupPersistence() {
    if (settingsStore.init() == StoreResult::Ok) {
        persistence.bindSettings(&settingsStore);
    }
    if (saveStore.init() == StoreResult::Ok) {
        persistence.bindSaves(&saveStore);
    }
    // An unbound role answers Unavailable: the game runs on defaults.
}

// Call from a menu or pause screen, never mid-gameplay.
StoreResult saveSettings(const Settings& s) {
    const uint8_t payload[2] = {s.volume, s.brightness};   // explicit layout
    uint8_t scratch[RecordCodec::kHeaderSize + sizeof(payload)];
    StoreResult r = persistence.saveSettingRecord("cfg", kSettingsSchema, payload,
                                                  sizeof(payload), scratch, sizeof(scratch));
    if (r != StoreResult::Ok) {
        return r;
    }
    return persistence.settings().commit();   // the durability point
}

Settings loadSettings() {
    Settings s;                                // defaults
    uint8_t scratch[RecordCodec::kHeaderSize + 16];
    const uint8_t* payload = nullptr;
    RecordInfo info{};
    if (persistence.loadSettingRecord("cfg", scratch, sizeof(scratch), payload, info) ==
            StoreResult::Ok &&
        info.schemaVersion == kSettingsSchema && info.payloadSize == 2) {
        s.volume = payload[0];
        s.brightness = payload[1];
    }
    // NotFound (first run), Corrupt, an older schema: keep or migrate defaults.
    return s;
}
```

Save slots work the same way through `saveRecord(slot, ...)` and
`loadRecord(slot, ...)`.

### Native (PC) builds: host files

```cpp
#include "persistence/PersistenceService.h"
#include "drivers/native/HostFileStore.h"

using namespace pixelroot32::persistence;

HostFileKeyValueStore settingsStore("save_data", "mygame", 64);     // root, namespace, max blob
HostFileSlotStore     saveStore("save_data", "save", 3, 256);       // root, prefix, slots, max record
PersistenceService    persistence;

void setupPersistence() {
    if (settingsStore.init() == StoreResult::Ok) {   // creates save_data/mygame/
        persistence.bindSettings(&settingsStore);
    }
    if (saveStore.init() == StoreResult::Ok) {       // creates save_data/save/
        persistence.bindSaves(&saveStore);
    }
}
```

The game code that saves and loads is the same as on the ESP32; only the
store construction differs. A game that builds for both picks the adapter with
`#ifdef ESP32` / `#ifdef PLATFORM_NATIVE` around the store definitions.

## Results and keys (`StoreTypes.h`)

```cpp
enum class StoreResult : uint8_t {
    Ok, NotFound, TooLarge, NoSpace, Corrupt, IoError, Unavailable, InvalidKey, InvalidArgument
};

inline constexpr size_t kMaxKeyLength = 15;
inline bool isValidKey(const char* key);
```

| `StoreResult` | Meaning |
|---------------|---------|
| `Ok` | The operation completed |
| `NotFound` | The key or slot holds no record; outputs are untouched |
| `TooLarge` | The data does not fit the caller's buffer or the medium's limit |
| `NoSpace` | The medium is full |
| `Corrupt` | Stored bytes failed validation (envelope, length or CRC) |
| `IoError` | The medium reported a failure the module cannot classify further |
| `Unavailable` | No store is bound, or the medium is not initialised |
| `InvalidKey` | The key or namespace breaks the rules of `isValidKey()` |
| `InvalidArgument` | A null pointer, an out-of-range slot or another bad parameter |

`isValidKey()` accepts 1..`kMaxKeyLength` bytes, each in `[A-Za-z0-9_-]`, and
rejects `nullptr`. Namespaces and slot prefixes follow the same rule. Every
store checks keys before touching the medium.

## `IKeyValueStore`

```cpp
class IKeyValueStore {
public:
    virtual ~IKeyValueStore() = default;
    virtual StoreResult getI32(const char* key, int32_t& out) = 0;
    virtual StoreResult setI32(const char* key, int32_t value) = 0;
    virtual StoreResult getU32(const char* key, uint32_t& out) = 0;
    virtual StoreResult setU32(const char* key, uint32_t value) = 0;
    virtual StoreResult getBlob(const char* key, uint8_t* out, size_t capacity,
                                size_t& stored) = 0;
    virtual StoreResult setBlob(const char* key, const uint8_t* data, size_t size) = 0;
    virtual StoreResult erase(const char* key) = 0;
    virtual StoreResult commit() = 0;
};
```

| Rule | Detail |
|------|--------|
| One value per key | A set of any type replaces whatever the key held; a typed get on a key holding another type is `NotFound` |
| Missing key | `NotFound`, every output untouched |
| `getBlob()` capacity | Never writes past `capacity`. A larger blob is `TooLarge`, nothing written, `stored` set to its size |
| Size query | `capacity == 0` (with `out == nullptr` allowed only then): `TooLarge` + `stored` for a non-empty blob, `Ok` + `0` for an empty one |
| `setBlob()` limits | `TooLarge` above the store's per-value maximum, `NoSpace` when full; a null `data` is allowed only with `size == 0` |
| Failed set | The previous value is kept |
| `erase()` | `NotFound` for a missing key |
| `commit()` | Makes every earlier set and erase durable; the game decides when |

## `ISlotStore`

```cpp
class ISlotStore {
public:
    virtual ~ISlotStore() = default;
    virtual uint16_t slotCount() const = 0;
    virtual size_t maxRecordSize() const = 0;
    virtual StoreResult query(uint16_t slot, size_t& stored) = 0;
    virtual StoreResult read(uint16_t slot, uint8_t* out, size_t capacity,
                             size_t& stored) = 0;
    virtual StoreResult write(uint16_t slot, const uint8_t* data, size_t size) = 0;
    virtual StoreResult erase(uint16_t slot) = 0;
};
```

| Rule | Detail |
|------|--------|
| Slot range | `0..slotCount()-1`; anything else is `InvalidArgument` before the medium is touched |
| Empty slot | `query()`/`read()` answer `NotFound` with `stored = 0` |
| `read()` capacity | Same rules as `getBlob()`, including the `capacity == 0` size query |
| `write()` | Replace-or-keep: after a failed or interrupted write, `read()` returns the old record or the new one, never a mix. `TooLarge` above `maxRecordSize()`. A zero-size record occupies the slot |
| `erase()` | `NotFound` when the slot is already empty |

## `RecordCodec`

```cpp
uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc = 0);

struct RecordInfo {
    uint16_t schemaVersion;   // game-owned, reported as stored
    size_t payloadSize;       // bytes after the header
};

class RecordCodec {
public:
    static constexpr size_t kHeaderSize = 16;
    static constexpr uint8_t kEnvelopeVersion = 1;
    static StoreResult encode(uint16_t schema, const uint8_t* payload, size_t size,
                              uint8_t* out, size_t capacity, size_t& written);
    static StoreResult decode(const uint8_t* in, size_t size, const uint8_t*& payload,
                              RecordInfo& info);
};
```

- `encode()` writes `kHeaderSize + size` bytes into `out`. It answers
  `TooLarge` when `capacity` is smaller, `InvalidArgument` for a null `out`, a
  null `payload` with a non-zero size, or a size beyond 32 bits. `written` is
  `0` on failure and nothing is written.
- `decode()` validates a whole record of exactly `size` bytes. It answers
  `Corrupt` for a truncated record, wrong magic, unknown envelope version,
  non-zero reserved byte, a length mismatch or a CRC mismatch, and
  `InvalidArgument` for a null `in`. On `Ok`, `payload` points at
  `in + kHeaderSize`; on failure `payload` and `info` are untouched. The schema
  version is reported, never judged.
- `crc32()` is CRC-32/IEEE (zlib/PNG). Passing a previous result as `crc`
  continues the stream.

## `PersistenceService`

```cpp
class PersistenceService {
public:
    void bindSettings(IKeyValueStore* store);   // nullptr unbinds; not owned
    void bindSaves(ISlotStore* store);          // nullptr unbinds; not owned
    IKeyValueStore& settings();                 // never null
    ISlotStore& saves();                        // never null
    bool hasSettings() const;
    bool hasSaves() const;

    StoreResult saveRecord(uint16_t slot, uint16_t schema, const uint8_t* payload, size_t size,
                           uint8_t* scratch, size_t scratchCapacity);
    StoreResult loadRecord(uint16_t slot, uint8_t* scratch, size_t scratchCapacity,
                           const uint8_t*& payload, RecordInfo& info);
    StoreResult saveSettingRecord(const char* key, uint16_t schema, const uint8_t* payload,
                                  size_t size, uint8_t* scratch, size_t scratchCapacity);
    StoreResult loadSettingRecord(const char* key, uint8_t* scratch, size_t scratchCapacity,
                                  const uint8_t*& payload, RecordInfo& info);
};
```

- An unbound role returns a null store: `NullKeyValueStore` or `NullSlotStore`,
  both public. Every call on them answers `Unavailable`; `NullSlotStore`
  reports `slotCount() == 0` and `maxRecordSize() == 0`.
- `saveRecord()` / `saveSettingRecord()` check in this order: `Unavailable`
  when the role is unbound, `InvalidArgument` for a null `scratch`, the
  codec's result (`TooLarge` when `scratchCapacity` is below
  `kHeaderSize + size`) with the store untouched, then the store's
  `write()` / `setBlob()` result unchanged.
- `loadRecord()` / `loadSettingRecord()` check `Unavailable`, then
  `InvalidArgument` for a null `scratch`, then pass the store's
  `read()` / `getBlob()` result unchanged (`NotFound`, `TooLarge`, ...), then
  answer `Corrupt` when the envelope fails to decode. On `Ok`, `payload`
  points inside `scratch`.
- The service never calls `commit()`. Call `settings().commit()` yourself at a
  safe point.
- The service holds no buffer and does not validate keys itself: the store
  answers `InvalidKey`.

## Adapters

All adapters are in `pixelroot32::persistence`. Every adapter with an `init()`
answers `Unavailable` to every operation until `init()` returns `Ok`;
`slotCount()` and `maxRecordSize()` report the configuration at any time.

### `Esp32NvsKeyValueStore` and `Esp32NvsSlotStore` (ESP32)

```cpp
inline constexpr const char* kNvsDefaultPartition = "nvs";

class Esp32NvsKeyValueStore final : public IKeyValueStore {
public:
    Esp32NvsKeyValueStore(const char* nameSpace, size_t maxBlobBytes,
                          const char* partition = kNvsDefaultPartition) noexcept;
    StoreResult init() noexcept;
    size_t maxBlobSize() const noexcept;
    // ... IKeyValueStore
};

class Esp32NvsSlotStore final : public ISlotStore {
public:
    Esp32NvsSlotStore(const char* nameSpace, const char* prefix, uint16_t slots,
                      size_t maxRecordBytes,
                      const char* partition = kNvsDefaultPartition) noexcept;
    StoreResult init() noexcept;
    // ... ISlotStore
};
```

- Guarded by `ESP32`. One NVS namespace per store; slot `i` is the blob under
  `<prefix><i>`, and `init()` rejects a prefix that leaves no room for the
  slot digits within 15 characters (`InvalidKey`).
- `init()` initialises the partition if nobody has and opens the namespace. It
  never erases: a partition that needs an erase answers `Corrupt`. Other
  results: `InvalidKey`, `InvalidArgument` (zero maximum, zero slots, null or
  empty partition label), `IoError` (including a missing partition).
- Blobs are byte-compatible with Arduino `Preferences::putBytes()` /
  `getBytes()` in the same namespace.
- Sets reach flash immediately (NVS has no write-back cache); `commit()` calls
  `nvs_commit`. Both stall flash-resident code on both cores.
- Non-copyable; the destructor closes the handle. The namespace and partition
  strings must stay valid until `init()` has run; the slot prefix must outlive
  the slot store. String literals are the intended use.

### `HostFileKeyValueStore` and `HostFileSlotStore` (native)

```cpp
inline constexpr size_t kHostFileMaxPathBytes = 260;
inline constexpr size_t kHostFileNameReserveBytes = 1 + 2 * kMaxKeyLength + 7;

class HostFileKeyValueStore final : public IKeyValueStore {
public:
    HostFileKeyValueStore(const char* rootDir, const char* nameSpace,
                          size_t maxBlobBytes) noexcept;
    StoreResult init() noexcept;
    size_t maxBlobSize() const noexcept;
    void setFailBeforeReplace(bool fail) noexcept;   // test hook
    // ... IKeyValueStore
};

class HostFileSlotStore final : public ISlotStore {
public:
    HostFileSlotStore(const char* rootDir, const char* prefix, uint16_t slots,
                      size_t maxRecordBytes) noexcept;
    StoreResult init() noexcept;
    void setFailBeforeReplace(bool fail) noexcept;   // test hook
    // ... ISlotStore
};
```

- Guarded by `PLATFORM_NATIVE`. Files live in `<rootDir>/<nameSpace>/` or
  `<rootDir>/<prefix>/`; `init()` creates the directory and its parents.
- `init()` answers `InvalidArgument` for a null or empty root, a zero maximum,
  zero slots or a root too long for `kHostFileMaxPathBytes`; `InvalidKey` for
  an invalid namespace or prefix; `IoError` when the directory cannot be
  created.
- Every write goes to a temp file and is moved over the final file in one
  step, so replace-or-keep holds. `commit()` answers `Ok`.
- A malformed file reads as `IoError`. The constructor copies its strings.
- `setFailBeforeReplace(true)` simulates a crash between the temp write and
  the move: the set answers `IoError` and the old value is kept.

### `MemoryKeyValueStore` and `MemorySlotStore` (any target)

```cpp
template <size_t Entries, size_t BlobBytes>
class MemoryKeyValueStore final : public IKeyValueStore {
public:
    // ... IKeyValueStore
    void setFailWrites(bool fail);
    StoreResult corruptByte(const char* key, size_t index);
    uint32_t writes() const;
    uint32_t commits() const;
};

template <uint16_t Slots, size_t RecordBytes>
class MemorySlotStore final : public ISlotStore {
public:
    // ... ISlotStore
    void setFailWrites(bool fail);
    StoreResult corruptByte(uint16_t slot, size_t index);
    uint32_t writes() const;
};
```

- Header-only, no heap, no `init()`. Contents are lost with the object.
- A new key beyond `Entries` is `NoSpace`; a blob above `BlobBytes` or a
  record above `RecordBytes` is `TooLarge`.
- Test hooks: `setFailWrites(true)` makes writes, erases (and, for KV,
  `commit()`) answer `IoError` without changing anything; `corruptByte()`
  flips one stored byte; `writes()` and `commits()` count successful calls.

## Public headers

| Header | Contents |
|--------|----------|
| `include/persistence/StoreTypes.h` | `StoreResult`, `kMaxKeyLength`, `isValidKey()` |
| `include/persistence/KeyValueStore.h` | `IKeyValueStore` |
| `include/persistence/SlotStore.h` | `ISlotStore` |
| `include/persistence/RecordCodec.h` | `RecordCodec`, `RecordInfo`, `crc32()` |
| `include/persistence/PersistenceService.h` | `PersistenceService`, `NullKeyValueStore`, `NullSlotStore` |
| `include/persistence/MemoryStores.h` | `MemoryKeyValueStore`, `MemorySlotStore` |
| `include/drivers/esp32/Esp32NvsStore.h` | `Esp32NvsKeyValueStore`, `Esp32NvsSlotStore`, `kNvsDefaultPartition` (guarded by `ESP32`) |
| `include/drivers/native/HostFileStore.h` | `HostFileKeyValueStore`, `HostFileSlotStore`, path limits (guarded by `PLATFORM_NATIVE`) |

The headers are the source of truth for signatures and exact wording.

## Related documentation

- [Persistence guide](../guide/persistence.md) - setup per platform, settings
  and save slots, schema versions, when to save.
- [Persistence subsystem](../architecture/persistence-subsystem.md) - envelope,
  atomicity, flash stall, memory and limits.
- [Configuration](config.md) - `PIXELROOT32_ENABLE_PERSISTENCE`.
- [API index](index.md) - all modules.
