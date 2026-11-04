# IKeyValueStore

<Badge type="info" text="Class" />

**Source:** `KeyValueStore.h`

## Description

Storage model for small named values: typed integers and blobs.

Rules shared by every adapter:
- Keys follow isValidKey(); any other key returns InvalidKey before the
  medium is touched, and no output is written.
- A key holds one value. A set of any type replaces whatever the key held
  (last write wins), so a typed get on a key that now holds another type
  returns NotFound.
- A missing key returns NotFound and leaves every output untouched.
- Blob reads never write past capacity (see getBlob()).
- Writes are explicit: nothing is written in the background. A set or
  erase may stay volatile until commit(); see commit().
- Calls come from one task (the game loop); adapters are not thread-safe.

## Methods

### `virtual StoreResult getI32(const char* key, int32_t& out)`

**Description:**

Reads a signed 32-bit value.

**Parameters:**

- `key`: Key to read.
- `out`: Receives the value on Ok; untouched otherwise.

**Returns:** Ok; NotFound when the key is missing or holds another type;
        InvalidKey; IoError or Unavailable from the medium.

### `virtual StoreResult setI32(const char* key, int32_t value)`

**Description:**

Stores a signed 32-bit value, replacing any value of any type.

**Returns:** Ok; InvalidKey; NoSpace when the key is new and the store is
        full; IoError or Unavailable from the medium. On failure the
        previous value is kept.

### `virtual StoreResult getU32(const char* key, uint32_t& out)`

**Description:**

Reads an unsigned 32-bit value.

**Parameters:**

- `key`: Key to read.
- `out`: Receives the value on Ok; untouched otherwise.

**Returns:** Ok; NotFound when the key is missing or holds another type;
        InvalidKey; IoError or Unavailable from the medium.

### `virtual StoreResult setU32(const char* key, uint32_t value)`

**Description:**

Stores an unsigned 32-bit value, replacing any value of any type.

**Returns:** As setI32().

### `virtual StoreResult getBlob(const char* key, uint8_t* out, size_t capacity, size_t& stored)`

**Description:**

Reads a blob into a caller-owned buffer.

**Parameters:**

- `key`: Key to read.
- `out`: Destination; nullptr only when
- `capacity`: Size of
- `stored`: Set to the stored blob size on Ok and TooLarge; untouched
       on every other result.

**Returns:** Ok; TooLarge when the blob exceeds @p capacity (nothing is
        written to @p out); NotFound when the key is missing or holds a
        typed integer; InvalidKey; InvalidArgument for a null @p out with
        a non-zero @p capacity; IoError or Unavailable from the medium.

### `virtual StoreResult setBlob(const char* key, const uint8_t* data, size_t size)`

**Description:**

Stores a blob, replacing any value of any type.

**Parameters:**

- `key`: Key to write.
- `data`: Bytes to store; nullptr only when
- `size`: Blob size; 0 stores an empty blob.

**Returns:** Ok; InvalidKey; InvalidArgument for a null @p data with a
        non-zero @p size; TooLarge when @p size exceeds the store's
        per-value maximum; NoSpace when the store (or medium) is full;
        IoError or Unavailable from the medium. On failure the previous
        value is kept.

### `virtual StoreResult erase(const char* key)`

**Description:**

Removes a key and its value, whatever its type.

**Returns:** Ok; NotFound when the key is missing; InvalidKey; IoError or
        Unavailable from the medium.

### `virtual StoreResult commit()`

**Description:**

Makes every earlier set and erase durable.

**Returns:** Ok; IoError or Unavailable from the medium.
