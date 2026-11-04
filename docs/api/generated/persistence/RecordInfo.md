# RecordInfo

<Badge type="info" text="Struct" />

**Source:** `RecordCodec.h`

## Description

Header fields reported by RecordCodec::decode().

## Properties

| Name | Type | Description |
|------|------|-------------|
| `schemaVersion` | `uint16_t` | Game-owned schema version, reported as stored. |
| `payloadSize` | `size_t` | Payload length in bytes, excluding the header. |
