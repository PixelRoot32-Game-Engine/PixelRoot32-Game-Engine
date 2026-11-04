# PersistenceService

<Badge type="info" text="Class" />

**Source:** `PersistenceService.h`

## Description

Thin role registry: a settings key-value store and a saves slot store.

The game constructs its stores and this service, binds each store to a
role at setup, and may still call a store directly. The service adds two
things on top:
- settings() and saves() are never null: an unbound role answers through a
  built-in null store whose every call is Unavailable.
- saveRecord()/loadRecord() (save slots) and saveSettingRecord()/
  loadSettingRecord() (settings blob keys) wrap a payload in the
  RecordCodec envelope on the way to the store, and validate it on the way
  back.

Writes are the game's decision (N4). The service never calls commit() and
never writes in the background: on the ESP32 a flash write or erase stalls
flash-resident code on both cores, so the game makes its saves - and calls
`settings().commit()` - at a safe point such as a menu.

The record helpers use only caller-owned scratch buffers; the service holds
no buffer and allocates nothing. Its size is two pointers plus two null
stores (one vtable pointer each): four pointers in total, 16 bytes on the
ESP32. Like the stores, it is used from one task (the game loop).

## Methods

### `void bindSettings(IKeyValueStore* store)`

**Description:**

Binds the settings role; nullptr unbinds it. The store is not owned.

### `void bindSaves(ISlotStore* store)`

**Description:**

Binds the saves role; nullptr unbinds it. The store is not owned.

### `IKeyValueStore& settings()`

**Description:**

The bound settings store, or a null store answering Unavailable.

### `ISlotStore& saves()`

**Description:**

The bound saves store, or a null store with no slots.

### `bool hasSettings() const`

**Description:**

Whether a settings store is bound.

### `bool hasSaves() const`

**Description:**

Whether a saves store is bound.

### `StoreResult saveRecord(uint16_t slot, uint16_t schema, const uint8_t* payload, size_t size, uint8_t* scratch, size_t scratchCapacity)`

**Description:**

Encodes a payload into @p scratch and writes it to a save slot.

**Parameters:**

- `slot`: Slot index.
- `schema`: Game-owned schema version, stored as is.
- `payload`: Payload bytes; nullptr only when
- `size`: Payload length; 0 is a valid record.
- `scratch`: Caller-owned buffer that receives the encoded record.
- `scratchCapacity`: Size of

**Returns:** Ok; Unavailable; InvalidArgument; TooLarge; or any error of
        ISlotStore::write() (InvalidArgument for an out-of-range slot,
        TooLarge, NoSpace, IoError). On failure the slot keeps its
        previous record.

### `StoreResult loadRecord(uint16_t slot, uint8_t* scratch, size_t scratchCapacity, const uint8_t*& payload, RecordInfo& info)`

**Description:**

Reads a save slot into @p scratch and validates its envelope.

**Parameters:**

- `slot`: Slot index.
- `scratch`: Caller-owned buffer that receives the whole record.
- `scratchCapacity`: Size of
- `payload`: On Ok, points at the payload inside
- `info`: On Ok, the stored schema version and payload size. Untouched
       on failure.

**Returns:** Ok or the first failure above.

### `StoreResult saveSettingRecord(const char* key, uint16_t schema, const uint8_t* payload, size_t size, uint8_t* scratch, size_t scratchCapacity)`

**Description:**

Encodes a payload into @p scratch and stores it as a settings blob.

**Parameters:**

- `key`: Settings key the record is stored under.
- `schema`: Game-owned schema version, stored as is.
- `payload`: Payload bytes; nullptr only when
- `size`: Payload length; 0 is a valid record.
- `scratch`: Caller-owned buffer that receives the encoded record.
- `scratchCapacity`: Size of

**Returns:** Ok or the first failure; on failure the key keeps its previous value.

### `StoreResult loadSettingRecord(const char* key, uint8_t* scratch, size_t scratchCapacity, const uint8_t*& payload, RecordInfo& info)`

**Description:**

Reads a settings blob into @p scratch and validates its envelope.

**Parameters:**

- `key`: Settings key to read.
- `scratch`: Caller-owned buffer that receives the whole record.
- `scratchCapacity`: Size of
- `payload`: On Ok, points at the payload inside
- `info`: On Ok, the stored schema version and payload size. Untouched
       on failure.

**Returns:** Ok or the first failure.
