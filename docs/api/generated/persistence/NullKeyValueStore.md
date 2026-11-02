# NullKeyValueStore

<Badge type="info" text="Class" />

**Source:** `PersistenceService.h`

**Inherits from:** [IKeyValueStore](./IKeyValueStore.md)

## Description

IKeyValueStore that is never available.

Stands in for an unbound settings role. Every call returns Unavailable
without validating its arguments and without touching any output.

## Inheritance

[IKeyValueStore](./IKeyValueStore.md) → `NullKeyValueStore`

## Methods

### `StoreResult getI32(const char*, int32_t&)`

### `StoreResult setI32(const char*, int32_t)`

### `StoreResult getU32(const char*, uint32_t&)`

### `StoreResult setU32(const char*, uint32_t)`

### `StoreResult getBlob(const char*, uint8_t*, size_t, size_t&)`

### `StoreResult setBlob(const char*, const uint8_t*, size_t)`

### `StoreResult erase(const char*)`

### `StoreResult commit()`
