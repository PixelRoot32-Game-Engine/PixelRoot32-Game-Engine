# AudioChannel

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Represents the internal state of a single audio channel.

Designed to be static and memory-efficient.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `dutySteps` | `const SfxBreakpoint*` | Duty stepped table (PULSE); nullptr/0 = use duty + dutySweep. |
| `dutyStepIndex` | `uint8_t` | Next duty step index to apply; == count when finished. |
| `dutyNextBoundarySamples` | `uint32_t` | Sample age at which the next duty step applies (UINT32_MAX = none). |
| `automationAgeSamples` | `uint32_t` | Samples since voice start (duty/pitch automation clock). |
| `pitchEnvelope` | `const SfxBreakpoint*` | Pitch envelope table; runtime multi-segment when count >= 2. |
| `pitchSegIndex` | `uint8_t` | Start-point index of the active pitch segment (0 .. count-2). |
| `pitchSegStartAge` | `uint32_t` | Absolute sample age at the start of the active pitch segment. |
| `pitchSegLenSamples` | `uint32_t` | Length of the active pitch segment in samples (0 = hold final value). |
| `noiseShortMode` | `bool` | @deprecated Use noiseLfsrShort. Will be removed in a future major version. |
| `triangleQuantize4Bit` | `bool` | When type == TRIANGLE: quantize output to 4-bit NES levels (32 steps). |
| `triangleOctaveUp` | `bool` | When type == TRIANGLE: double the effective frequency at initVoiceFromEvent time, matching the NES convention where TRIANGLE is exactly one octave below PULSE for the same period value. |
| `pulseDutyIndex` | `uint8_t` | When type == PULSE: 0..3 selects the NES discrete duty pattern (12.5% / 25% / 50% / 75%); 255 = use continuous `dutyCycle` (default). |
| `nesNoiseLutIndex` | `uint8_t` | NOISE LUT index: 0..15 select a NES noise period from `nes_apu::kNesNoisePeriodLutNtsc`; 255 = derive the period from `frequency` (default, legacy behaviour). |
| `noisePeriodSamples` | `uint32_t` | Samples until next LFSR step on NOISE; `frequency` sets noise clock rate (not pitch). |
| `noiseCountdown` | `uint32_t` | Counts down each output sample; at 0 the LFSR advances and reloads to noisePeriodSamples. |
| `loop` | `bool` | Continuous voice: no auto-disable; cleared only by STOP_CHANNEL / steal. |
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
| `nesLengthCounter` | `NesLengthCounter` | NES APU length counter state for this voice. |
| `nesLinearCounter` | `NesLinearCounter` | NES APU linear counter state for this voice (TRIANGLE). |
| `nesSweepUnit` | `NesSweepUnit` | NES APU sweep unit state for this voice (PULSE). |
| `nesEnvelope` | `NesEnvelope` | NES APU envelope unit state for this voice (PULSE / NOISE). |

## Methods

### `void reset()`

**Description:**

Resets the channel to a clean disabled state.
