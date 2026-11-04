# HostFileKeyValueStore

<Badge type="info" text="Class" />

**Source:** `HostFileStore.h`

**Inherits from:** [IKeyValueStore](../persistence/IKeyValueStore.md)

## Description

IKeyValueStore over one file per key, for native (PC) builds.

Files live in `<rootDir>/<nameSpace>/`. A key's file name is the key with
each uppercase letter written as `^` plus its lowercase letter, then `.kv`
(`Ab_C` -> `^ab_^c.kv`): host file systems may ignore case, `^` is not a
key character, so distinct keys always get distinct files. A file holds a
one-byte type tag (1 = i32, 2 = u32, 3 = blob) followed by the value;
integers are 4 little-endian bytes.

Replace-or-keep (S7): every set writes `<file>.tmp`, flushes and closes it,
then moves it over the final file in one step - `MoveFileExA` with
`MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH` on Windows, `rename()`
on POSIX. A crash leaves either the old file or the new one, plus at worst
a stale `.tmp` that reads never open and the next write overwrites.

Errors: a missing file is NotFound; a full disk is NoSpace; any other I/O
failure is IoError. A file whose tag or size is malformed (empty,
truncated, unknown tag, blob above the store's maximum) is IoError on every
get - never a silent NotFound - so damage is visible; a set replaces such a
file and erase removes it. There is no envelope here: RecordCodec validates
records above this layer.

init() must succeed before use; until then every operation, commit()
included, returns Unavailable without touching the disk. commit() returns
Ok because every set is already flushed and in place. The constructor
copies its strings, so they need not outlive it. No exceptions; the object
holds fixed-size buffers only (no heap of its own; the C library may
allocate inside its file calls on the host).

Test hook (not part of IKeyValueStore): setFailBeforeReplace().

## Inheritance

[IKeyValueStore](../persistence/IKeyValueStore.md) → `HostFileKeyValueStore`

## Methods

### `StoreResult getI32(const char* key, int32_t& out)`

### `StoreResult setI32(const char* key, int32_t value)`

### `StoreResult getU32(const char* key, uint32_t& out)`

### `StoreResult setU32(const char* key, uint32_t value)`

### `StoreResult getBlob(const char* key, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult setBlob(const char* key, const uint8_t* data, size_t size)`

### `StoreResult erase(const char* key)`

### `StoreResult commit()`
