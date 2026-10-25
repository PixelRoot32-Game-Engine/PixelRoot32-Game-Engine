# NetPacketHeader

<Badge type="info" text="Struct" />

**Source:** `NetProtocol.h`

## Description

Decoded, host-order view of the 16-byte wire header.

This struct is never copied to or from the wire as a whole: padding and
host endianness are handled by explicit field-by-field encode/decode, so
the on-wire bytes are identical on ESP32 (little-endian RISC-V) and native.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `version` | `uint8_t` | Wire protocol version, must equal the build's. |
| `type` | `NetPacketType` | Packet discriminator. |
| `flags` | `uint8_t` | Bit 0 reliable; every other bit reserved (must be zero). |
| `sourceNodeId` | `uint8_t` | Sender's application label. |
| `sourceBootId` | `uint16_t` | Sender's boot generation. |
| `sequence` | `uint32_t` | Sender's monotonic sequence. |
| `ackSequence` | `uint16_t` | Reserved (0 in v1); ACKs carry the 32-bit sequence in their 4-byte payload. |
| `payloadLength` | `uint16_t` | Declared payload byte count. |
