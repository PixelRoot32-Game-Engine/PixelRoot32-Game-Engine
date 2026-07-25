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
| `sweepStartHz` | `float` | Starting frequency in Hz (NOISE: LFSR clock). |
| `sweepEndHz` | `float` | Ending frequency in Hz (NOISE: LFSR clock). |
| `sweepStartIncQ32` | `uint32_t` | Melodic: Q32 phase inc start; NOISE: start period. |
| `sweepEndIncQ32` | `uint32_t` | Melodic: Q32 phase inc end; NOISE: end period. |

## Methods

### `void reset()`

**Description:**

Resets the channel to a clean disabled state.
