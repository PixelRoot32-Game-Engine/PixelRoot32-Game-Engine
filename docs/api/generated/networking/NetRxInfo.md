# NetRxInfo

<Badge type="info" text="Struct" />

**Source:** `NetTransport.h`

## Description

One received frame together with its source address.

`data` holds a complete frame (header plus payload), not an application
payload, and is sized to the maximum frame so receiving never allocates.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `from` | `NetAddress` | Sender's link-layer address. |
| `length` | `uint16_t` | Valid bytes in @ref data. |
| `data` | `uint8_t` | Complete frame bytes. |
