# Slot

<Badge type="info" text="Struct" />

**Source:** `NetCounter.h`

## Description

One node's accumulated contribution to the shared value.

`cursor` is the serial number of the last operation applied from this
node. It survives normal operation, is exported in a snapshot and reset
when the node advertises a new boot generation, because a reboot starts
a fresh sequence space. The local slot's cursor stays 0: it tracks
inbound operations, and a node never applies its own.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `value` | `int32_t` | Sum of the deltas applied from this node. |
| `cursor` | `uint32_t` | Last applied sequence from this node. |
| `bootId` | `uint16_t` | Node boot generation, learned from peerInfo. |
| `nodeId` | `uint8_t` | Application label that owns this slot. |
| `used` | `bool` | false when the slot was never allocated. |
