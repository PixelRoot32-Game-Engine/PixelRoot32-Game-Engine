# NetPeerInfo

<Badge type="info" text="Struct" />

**Source:** `NetTypes.h`

## Description

Snapshot of one peer table slot, copied out on query.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `address` | `NetAddress` | Peer MAC, the identity used by the transport. |
| `nodeId` | `uint8_t` | Application label last advertised by the peer. |
| `bootId` | `uint16_t` | Boot generation, used to reset dedupe state. |
| `rttMs` | `uint16_t` | Last measured round-trip time. |
| `lastSeenMs` | `uint32_t` | Timestamp of the last accepted packet. |
| `uptimeMs` | `uint32_t` | Peer uptime last advertised by HELLO. |
| `online` | `bool` | false once the peer timeout elapsed. |
