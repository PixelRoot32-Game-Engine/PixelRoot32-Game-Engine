# LoopbackFaults

<Badge type="info" text="Struct" />

**Source:** `LoopbackTransport.h`

## Description

Injectable link faults for deterministic native testing.

Percentages are applied per destination in a fixed order: loss, then
duplication, then reordering. All time comes from the owning bus, never from
the system clock, so a test script is reproducible on any machine.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `lossPercent` | `uint8_t` | 0..100 chance a frame is dropped. |
| `duplicatePercent` | `uint8_t` | 0..100 chance a frame is enqueued twice. |
| `reorderPercent` | `uint8_t` | 0..100 chance a frame jumps the previous one. |
| `delayMs` | `uint16_t` | Delivery delay added to the bus clock. |
| `sendBusy` | `bool` | Makes every send return NetResult::Busy. |
| `seed` | `uint32_t` | Seed of the xorshift32 decision generator. |
