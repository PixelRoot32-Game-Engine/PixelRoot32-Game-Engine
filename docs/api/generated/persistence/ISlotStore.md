# ISlotStore

<Badge type="info" text="Class" />

**Source:** `SlotStore.h`

## Description

Storage model for a fixed number of numbered records (save slots).

Rules shared by every adapter:
- Slots are numbered 0..slotCount()-1; any other slot returns
  InvalidArgument before the medium is touched, and no output is written.
- An empty slot reads as NotFound with `stored = 0`.
- Reads never write past capacity (see read()).
- write() is replace-or-keep: after a failed or interrupted write, read()
  returns either the previous record or the new one, never a mix. Each
  adapter documents how it guarantees this.
- Records are opaque bytes; RecordCodec validates them above this layer.
- Calls come from one task (the game loop); adapters are not thread-safe.

## Methods

### `virtual uint16_t slotCount() const`

**Description:**

Number of slots; valid slot indices are 0..slotCount()-1.

### `virtual size_t maxRecordSize() const`

**Description:**

Largest record write() accepts, in bytes.

### `virtual StoreResult query(uint16_t slot, size_t& stored)`

**Description:**

Reports the size of the record in a slot.

**Parameters:**

- `slot`: Slot index.
- `stored`: Set to the record size on Ok, to 0 on NotFound; untouched
       on every other result.

**Returns:** Ok; NotFound for an empty slot; InvalidArgument for an
        out-of-range slot; IoError or Unavailable from the medium.

### `virtual StoreResult read(uint16_t slot, uint8_t* out, size_t capacity, size_t& stored)`

**Description:**

Reads a record into a caller-owned buffer.

**Parameters:**

- `slot`: Slot index.
- `out`: Destination; nullptr only when
- `capacity`: Size of
- `stored`: Set to the record size on Ok and TooLarge, to 0 on
       NotFound; untouched on every other result.

**Returns:** Ok; NotFound for an empty slot; TooLarge when the record exceeds

### `virtual StoreResult write(uint16_t slot, const uint8_t* data, size_t size)`

**Description:**

Replaces the record in a slot (replace-or-keep).

**Parameters:**

- `slot`: Slot index.
- `data`: Record bytes; nullptr only when
- `size`: Record size; 0 stores an empty record (the slot is then
       occupied, not empty).

**Returns:** Ok; InvalidArgument for an out-of-range slot or a null @p data
        with a non-zero @p size; TooLarge when @p size exceeds
        maxRecordSize(); NoSpace when the medium is full; IoError or
        Unavailable from the medium. On failure the previous record is
        kept.

### `virtual StoreResult erase(uint16_t slot)`

**Description:**

Empties a slot.

**Returns:** Ok; NotFound when the slot is already empty; InvalidArgument for
        an out-of-range slot; IoError or Unavailable from the medium.
