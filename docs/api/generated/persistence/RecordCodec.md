# RecordCodec

<Badge type="info" text="Class" />

**Source:** `RecordCodec.h`

## Description

Wraps a game payload in a self-checking envelope.

Adapters move opaque bytes; this codec is the one place a record is
validated. The 16-byte header is written byte by byte, little-endian:

| Bytes | Field |
|-------|-------|
| 0-3   | Magic, ASCII `P` `R` `S` `T` |
| 4     | Envelope version (kEnvelopeVersion) |
| 5     | Reserved, 0 |
| 6-7   | Schema version (u16, game-owned) |
| 8-11  | Payload length (u32) |
| 12-15 | CRC-32 over bytes 0..11 followed by the payload |

Covering the header in the CRC means a flipped schema version or length is
caught even though neither field is checked against a fixed value.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kHeaderSize` | `static constexpr size_t` | Size in bytes of the envelope header. |
| `kEnvelopeVersion` | `static constexpr uint8_t` | Envelope layout version written and accepted by this codec. |

## Methods

### `static StoreResult encode(uint16_t schema, const uint8_t* payload, size_t size, uint8_t* out, size_t capacity, size_t& written)`

**Description:**

Writes header and payload into a caller-owned buffer.

**Parameters:**

- `schema`: Game-owned schema version, stored as is.
- `payload`: Payload bytes; may be nullptr only when
- `size`: Payload length; 0 is a valid record.
- `out`: Destination buffer.
- `capacity`: Size of
- `written`: Set to kHeaderSize +

**Returns:** Ok; InvalidArgument for a null @p out, a null @p payload with a
        non-zero size, or a size beyond the u32 length field; TooLarge
        when @p capacity is below kHeaderSize + @p size. Nothing is
        written to @p out on failure.

### `static StoreResult decode(const uint8_t* in, size_t size, const uint8_t*& payload, RecordInfo& info)`

**Description:**

Validates a record and locates its payload.

**Parameters:**

- `in`: Record bytes, header first.
- `size`: Exact record size in bytes.
- `payload`: Set to `in + kHeaderSize` on success.
- `info`: Set to the stored schema version and payload size on success.

**Returns:** Ok; InvalidArgument for a null @p in; Corrupt for a truncated
        record, wrong magic, unknown envelope version, non-zero reserved
        byte, a length field that disagrees with @p size, or a CRC
        mismatch. The schema version is reported, never judged.
