# HostFileSlotStore

<Badge type="info" text="Class" />

**Source:** `HostFileStore.h`

**Inherits from:** [ISlotStore](../persistence/ISlotStore.md)

## Description

ISlotStore over one file per slot, for native (PC) builds.

Files live in `<rootDir>/<prefix>/` and are named after the slot index in
decimal, then `.slot` (`2.slot`); a file holds the record bytes as they
are, and a missing file is an empty slot. Writes use the same temp file and
atomic move as HostFileKeyValueStore, so replace-or-keep (S7) holds the
same way, and the same error mapping applies: a file larger than
maxRecordSize() is IoError.

init() must succeed before use; until then query(), read(), write() and
erase() return Unavailable, even for an out-of-range slot. slotCount() and
maxRecordSize() report the configuration at any time.

Test hook (not part of ISlotStore): setFailBeforeReplace().

## Inheritance

[ISlotStore](../persistence/ISlotStore.md) → `HostFileSlotStore`

## Methods

### `uint16_t slotCount() const`

### `size_t maxRecordSize() const`

### `StoreResult query(uint16_t slot, size_t& stored)`

### `StoreResult read(uint16_t slot, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult write(uint16_t slot, const uint8_t* data, size_t size)`

### `StoreResult erase(uint16_t slot)`
