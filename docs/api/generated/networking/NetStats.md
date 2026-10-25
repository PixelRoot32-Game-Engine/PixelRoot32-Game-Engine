# NetStats

<Badge type="info" text="Struct" />

**Source:** `NetTypes.h`

## Description

Monotonic traffic counters owned by the session.

Counters only ever increase for the lifetime of the session; consumers
derive rates by differencing two snapshots.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `txMessages` | `uint32_t` | Frames handed to the transport. |
| `rxMessages` | `uint32_t` | Frames accepted from the transport. |
| `txRetries` | `uint32_t` | Reliable retransmissions. |
| `txFailed` | `uint32_t` | Reliable messages that gave up. |
| `rxDropped` | `uint32_t` | Frames dropped before delivery (bad frame, full queue). |
| `rxDuplicates` | `uint32_t` | Frames rejected by the dedupe window. |
| `txBusy` | `uint32_t` | Sends deferred because the transport was busy. |
| `lastRttMs` | `uint16_t` | Most recent round-trip time. |
