# PeerSlot

<Badge type="info" text="Struct" />

**Source:** `NetSession.h`

## Description

One fixed peer table row.

A slot is never freed: a timeout only clears `online`, so a returning
peer keeps its index and its accumulated state.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `address` | `NetAddress` | Identity: the peer's source MAC. |
| `lastSeenMs` | `uint32_t` | Session time of the last accepted frame. |
| `uptimeMs` | `uint32_t` | Uptime last advertised by HELLO. |
| `nextTxSequence` | `uint32_t` | Next reliable sequence to assign. |
| `rxHighestSeq` | `uint32_t` | Newest reliable sequence accepted. |
| `rxWindow` | `uint64_t` | Accepted older sequences, bit 0 = highest. |
| `bootId` | `uint16_t` | Peer's boot generation from HELLO. |
| `nodeId` | `uint8_t` | Application label from HELLO. |
| `used` | `bool` | false when the slot was never allocated. |
| `online` | `bool` | false after BYE or the peer timeout. |
