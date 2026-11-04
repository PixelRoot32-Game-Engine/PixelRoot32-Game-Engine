# MemoryKeyValueStore

<Badge type="info" text="Class" />

**Source:** `MemoryStores.h`

**Inherits from:** [IKeyValueStore](./IKeyValueStore.md)

## Description

IKeyValueStore over a fixed in-RAM table; for tests and volatile use.

Holds up to Entries keys, each with an integer or a blob of up to
BlobBytes bytes. Integers are stored as 4 little-endian bytes. No heap;
contents are lost with the object. A set or erase applies in full or not at
all, so replace-or-keep holds trivially; commit() only counts.

Test hooks (not part of IKeyValueStore): setFailWrites(), corruptByte(),
writes() and commits().

Entries Maximum number of keys; a new key beyond it is NoSpace.
BlobBytes Largest blob accepted; a larger one is TooLarge.

## Inheritance

[IKeyValueStore](./IKeyValueStore.md) → `MemoryKeyValueStore`

## Methods

### `StoreResult getI32(const char* key, int32_t& out)`

### `StoreResult setI32(const char* key, int32_t value)`

### `StoreResult getU32(const char* key, uint32_t& out)`

### `StoreResult setU32(const char* key, uint32_t value)`

### `StoreResult getBlob(const char* key, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult setBlob(const char* key, const uint8_t* data, size_t size)`

### `StoreResult erase(const char* key)`

### `StoreResult commit()`

### `void setFailWrites(bool fail)`

**Description:**

Makes every later set, erase and commit fail with IoError.

### `StoreResult corruptByte(const char* key, size_t index)`

**Description:**

Flips every bit of one stored byte, simulating media damage.

**Parameters:**

- `key`: Key whose value is damaged; integers count as 4 bytes.
- `index`: Byte offset inside the stored value.

**Returns:** Ok; InvalidKey; NotFound for a missing key; InvalidArgument when

### `uint32_t writes() const`

**Description:**

Number of sets and erases that succeeded.

### `uint32_t commits() const`

**Description:**

Number of commit() calls that succeeded.
