# Esp32NvsSlotStore

<Badge type="info" text="Class" />

**Source:** `Esp32NvsStore.h`

**Inherits from:** [ISlotStore](../persistence/ISlotStore.md)

## Description

ISlotStore over one ESP-IDF NVS namespace: one blob per slot.

Slot `i` is the blob under key `<prefix><i>` (decimal, `save0`, `save1`,
...). init() checks that the longest key, prefix plus the digits of
`slots - 1`, fits kMaxKeyLength. An empty slot is a missing key.

Replace-or-keep (S7): a write is one `nvs_set_blob`, and NVS writes the new
blob (its data chunks, then its index entry) before it erases the old one,
so after a power loss read() returns the old record or the new one. If the
key holds an item of another type (not written by this store), it is erased
first, as in Esp32NvsKeyValueStore.

Errors, init(), Unavailable-before-init, writes and memory as
Esp32NvsKeyValueStore; the prefix string is used on every call and must
outlive the object. slotCount() and maxRecordSize() report the
configuration at any time; query(), read(), write() and erase() return
Unavailable before init(), even for an out-of-range slot.

## Inheritance

[ISlotStore](../persistence/ISlotStore.md) → `Esp32NvsSlotStore`

## Methods

### `uint16_t slotCount() const`

### `size_t maxRecordSize() const`

### `StoreResult query(uint16_t slot, size_t& stored)`

### `StoreResult read(uint16_t slot, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult write(uint16_t slot, const uint8_t* data, size_t size)`

### `StoreResult erase(uint16_t slot)`

### `StoreResult init() noexcept; uint16_t slotCount() const`

**Description:**

As Esp32NvsKeyValueStore::init(), with InvalidArgument also for
       zero slots and InvalidKey for an invalid prefix or one too long
       for the slot digits.
