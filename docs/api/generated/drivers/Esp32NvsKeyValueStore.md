# Esp32NvsKeyValueStore

<Badge type="info" text="Class" />

**Source:** `Esp32NvsStore.h`

**Inherits from:** [IKeyValueStore](../persistence/IKeyValueStore.md)

## Description

IKeyValueStore over one ESP-IDF NVS namespace (`nvs.h`).

Integers are stored as NVS `I32` / `U32` items and blobs as NVS blobs, so a
blob written by Arduino `Preferences::putBytes(key, ...)` in the same
namespace reads back byte-identically through getBlob() (S13), and the
other way round.

One value per key: NVS itself can hold one item per type under the same
key (measured on IDF 4.4.7: after `nvs_set_i32` then `nvs_set_blob` on one
key both succeed, `nvs_get_i32` still returns the old integer and the blob
length query answers NOT_FOUND), so before a set the adapter probes the key
with cheap reads and, when an item of another type is there, erases it
(`nvs_erase_key`, repeated until the key is gone) before writing the new
one. A same-type set is a single NVS
write, which is replace-or-keep: NVS writes the new entry before it marks
the old one erased. A type change is not: power lost between the erase and
the write loses the key. erase() removes every item under the key.

Errors (S12): `ESP_ERR_NVS_NOT_FOUND` and `ESP_ERR_NVS_TYPE_MISMATCH` are
NotFound; `ESP_ERR_NVS_NOT_ENOUGH_SPACE` is NoSpace; `ESP_ERR_NVS_KEY_TOO_LONG`
and `ESP_ERR_NVS_INVALID_NAME` are InvalidKey; `ESP_ERR_NVS_INVALID_LENGTH`
and `ESP_ERR_NVS_VALUE_TOO_LONG` are TooLarge; `ESP_ERR_NVS_NOT_INITIALIZED`
and `ESP_ERR_NVS_INVALID_HANDLE` are Unavailable; anything else is IoError.
Keys are checked with isValidKey() before NVS sees them, so the key errors
are only a safety net.

init() never erases. It initialises the partition when nobody has yet
(`nvs_flash_init_partition`; a partition the Arduino core or the game
already initialised is fine) and opens the namespace read-write. A
partition NVS cannot use as it is - `ESP_ERR_NVS_NO_FREE_PAGES` or
`ESP_ERR_NVS_NEW_VERSION_FOUND` - is reported as Corrupt and left alone:
erasing it would wipe every namespace on it, which is the game's decision
(`nvs_flash_erase_partition`), never the engine's.

Until init() returns Ok every operation, commit() included, returns
Unavailable without touching flash. The destructor closes the handle.

Writes (N4): sets and erases reach flash when they are made (NVS has no
write-back cache), and commit() calls `nvs_commit`; the game calls both at a
safe point. Any NVS write or page erase disables the flash cache on both
cores, stalling every task that runs from flash - render and audio included
- until it ends.

Memory: the object holds a handle, two string pointers and the limits; no
buffers, no heap. The namespace and partition strings are used by init()
only, but must stay valid until it has run (string literals are the
intended use). Calls come from one task.

## Inheritance

[IKeyValueStore](../persistence/IKeyValueStore.md) → `Esp32NvsKeyValueStore`

## Methods

### `StoreResult getI32(const char* key, int32_t& out)`

### `StoreResult setI32(const char* key, int32_t value)`

### `StoreResult getU32(const char* key, uint32_t& out)`

### `StoreResult setU32(const char* key, uint32_t value)`

### `StoreResult getBlob(const char* key, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult setBlob(const char* key, const uint8_t* data, size_t size)`

### `StoreResult erase(const char* key)`

### `StoreResult commit()`
