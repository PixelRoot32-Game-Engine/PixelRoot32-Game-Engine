# SdSlotStore

<Badge type="info" text="Class" />

**Source:** `SdStore.h`

**Inherits from:** [ISlotStore](../persistence/ISlotStore.md)

## Description

ISlotStore over one file per slot on a FAT microSD card.

Files live in `<rootDir>/<prefix>/` and are named after the slot index in
decimal, then `.slot` (`2.slot`); a missing file is an empty slot. `rootDir`
is an SD path relative to the mountpoint the Arduino `SD` library uses
(`/sd` by default), so root `"/pr32"` maps to `<mountpoint>/pr32` on the
card; `prefix` names a sub-directory of it. The two strings are held by
pointer and must outlive the object, exactly like the NVS adapter's prefix
(string literals are the intended use).

init(config) is non-destructive: it brings up the SPI bus named by
config, mounts the card with `SD.begin(cs, spi, frequencyHz)` and creates
the two directories (`mkdir` on an existing directory is not an error). It
never formats, erases or repairs the card. Until init() returns Ok, every
operation - including out-of-range and null arguments - returns Unavailable,
so a game can boot, fail to mount and keep running. Calling init() again on
a ready store whose directory still answers is Ok; when the card went away,
init() releases the dead volume and mounts again, so reinserting a card
needs no reboot.

Replace-or-keep (S5): every write goes to `<file>.tmp`, which is flushed
(the Arduino FS flush calls `fsync`) and closed, then the old `<file>` is
removed and `SD.rename(tmp, file)` moves the new bytes into place. FAT
cannot rename over an existing destination, so the removal is required, and
that pair is the accepted non-atomic replace on FAT: a power loss before the
removal keeps the old record, a loss during the rename gives the new record
or a torn one that the RecordCodec envelope rejects, and a loss in between
leaves an empty slot - the limit FAT imposes. A failed write removes the
stray `.tmp`; an early failure leaves the previous record untouched.

Errors (S6): a missing file is NotFound; a full card is NoSpace; a file
larger than maxRecordSize(), a malformed file or any other I/O failure is
IoError (never a silent NotFound, so damage is visible); a store that was
never initialised, or whose SD volume was released, is Unavailable; an
out-of-range slot or a null buffer with a non-zero capacity is
InvalidArgument; a record above maxRecordSize() is TooLarge. A card pulled
after init() cannot be detected up front - the Arduino SD library caches
the card type at mount - so the call that touches the card answers IoError
(or NotFound for a missing file); init() mounts it again. There is no
commit(): every write is flushed and in place.

Memory: no heap of its own, fixed buffers on the call stack only; two
pointer members and the limits. Calls come from one task (the game loop);
the adapter is not thread-safe. The FATFS driver the Arduino SD library
brings in costs several KB of RAM.

## Inheritance

[ISlotStore](../persistence/ISlotStore.md) → `SdSlotStore`

## Methods

### `uint16_t slotCount() const`

### `size_t maxRecordSize() const`

### `StoreResult query(uint16_t slot, size_t& stored)`

### `StoreResult read(uint16_t slot, uint8_t* out, size_t capacity, size_t& stored)`

### `StoreResult write(uint16_t slot, const uint8_t* data, size_t size)`

### `StoreResult erase(uint16_t slot)`

### `StoreResult init(const SdConfig& config) noexcept; uint16_t slotCount() const`

**Description:**

Checks the arguments, mounts (or re-mounts) the card and creates
       the directory.

**Parameters:**

- `config`: SPI bus, pins and clock; `spi` must be non-null and `cs`
       a valid pin.

**Returns:** Ok (also when already mounted and its directory still answers);
        InvalidArgument for a null or
        empty root, a root without a leading '/', zero slots, a zero
        maximum, a null bus, a negative CS or any pin outside -1..127,
        or an over-long path; InvalidKey for a prefix that fails
        isValidKey(); Unavailable when the card cannot be mounted.
        Nothing is created or mounted on an argument error.
