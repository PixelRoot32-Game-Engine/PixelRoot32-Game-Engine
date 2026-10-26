# NetEvent

<Badge type="info" text="Struct" />

**Source:** `NetTypes.h`

## Description

One fixed-capacity event queued for the application.

The payload buffer is inline at the configured ceiling
(`platforms::config::NetMaxPayload`), so delivering an event never
allocates. Field order is declaration order for alignment and is pinned by
the session's memory budget; do not reorder casually.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `type` | `NetEventType` | Event kind. |
| `peerId` | `uint8_t` | Sender / affected peer slot. |
| `nodeId` | `uint8_t` | Application label of that peer. |
| `sequence` | `uint32_t` | Wire sequence that produced the event. |
| `length` | `uint16_t` | Valid bytes in @ref payload. |
| `payload` | `uint8_t` | Inline application bytes. |
