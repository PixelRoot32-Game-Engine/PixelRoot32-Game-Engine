# MemorySlotStore

<Badge type="info" text="Class" />

**Source:** `MemoryStores.h`

**Inherits from:** [ISlotStore](./ISlotStore.md)

## Description

ISlotStore over fixed in-RAM records; for tests and volatile use.

Slots records of up to RecordBytes bytes each. No heap; contents are
lost with the object. A write applies in full or not at all, so
replace-or-keep holds trivially.

Test hooks (not part of ISlotStore): setFailWrites(), corruptByte() and
writes().

Slots Number of slots.
RecordBytes Largest record accepted; a larger one is TooLarge.

## Inheritance

[ISlotStore](./ISlotStore.md) → `MemorySlotStore`

## Methods

### `uint16_t slotCount() const`

### `size_t maxRecordSize() const`

### `StoreResult query(uint16_t slot, size_t& stored)`

### `StoreResult read(uint16_t slot, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult write(uint16_t slot, const uint8_t* data, size_t size)`

### `StoreResult erase(uint16_t slot)`

### `void setFailWrites(bool fail)`

**Description:**

Makes every later write and erase fail with IoError.

### `StoreResult corruptByte(uint16_t slot, size_t index)`

**Description:**

Flips every bit of one stored byte, simulating media damage.

**Parameters:**

- `slot`: Slot whose record is damaged.
- `index`: Byte offset inside the record.

**Returns:** Ok; NotFound for an empty slot; InvalidArgument for an
        out-of-range slot or an @p index past the record size.

### `uint32_t writes() const`

**Description:**

Number of writes and erases that succeeded.
