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
| `sweepCurve` | `SweepCurve` | Active sweep curve (may fallback to Linear). |
| `sweepLogRatio` | `float` | FPU Exponential: logf(endHz/startHz). |
| `sweepLogStartQ16` | `int32_t` | Q15 path Exponential: log2(start) in Q16. |
| `sweepLogDeltaQ16` | `int32_t` | Q15 path Exponential: log2(end/start) in Q16. |

## Methods

### `void reset()`

**Description:**

Resets the channel to a clean disabled state.
