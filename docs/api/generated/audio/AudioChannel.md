# AudioChannel

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Represents the internal state of a single audio channel.

Designed to be static and memory-efficient.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `sweepSamplesTotal` | `uint32_t` | Total samples for the sweep. |
| `sweepSamplesRemaining` | `uint32_t` | Samples remaining in the sweep. |
| `sweepStartHz` | `float` | Starting frequency in Hz. |
| `sweepEndHz` | `float` | Ending frequency in Hz. |
| `sweepStartIncQ32` | `uint32_t` | Q32 phase increment at sweep start. |
| `sweepEndIncQ32` | `uint32_t` | Q32 phase increment at sweep end. |

## Methods

### `void reset()`

**Description:**

Resets the channel to a clean disabled state.
