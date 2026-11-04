# NullSlotStore

<Badge type="info" text="Class" />

**Source:** `PersistenceService.h`

**Inherits from:** [ISlotStore](./ISlotStore.md)

## Description

ISlotStore that is never available.

Stands in for an unbound saves role: it has no slots (slotCount() and
maxRecordSize() are 0) and every operation returns Unavailable - including
for a slot index that would be out of range - without touching any output.

## Inheritance

[ISlotStore](./ISlotStore.md) → `NullSlotStore`

## Methods

### `uint16_t slotCount() const`

### `size_t maxRecordSize() const`

### `StoreResult query(uint16_t, size_t&)`

### `StoreResult read(uint16_t, uint8_t*, size_t, size_t&)`

### `StoreResult write(uint16_t, const uint8_t*, size_t)`

### `StoreResult erase(uint16_t)`
