# LfoState

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Holds LFO (Low-Frequency Oscillator) state for pitch or volume modulation.

Supports both floating-point and Q15 fixed-point variants for no-FPU platforms.
The LFO produces a sine-like waveform that can modulate pitch (vibrato) or
volume (tremolo) of a voice.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `enabled` | `bool` | Whether the LFO is active. |
| `target` | `LfoTarget` | What the LFO modulates: PITCH or VOLUME. |
| `depth` | `float` | Modulation depth (pitch: ratio e.g. 0.05; volume: 0.0-1.0). |
| `periodSamples` | `uint32_t` | LFO period in samples (derived from lfoFrequency). |
| `sampleCounter` | `uint32_t` | Current sample position within the period. |
| `currentValue` | `float` | Current LFO output value [-1.0 to +1.0]. |
| `depthQ15` | `int32_t` | Q15 depth: 0-32768 maps to 0.0-1.0. |
| `currentValueQ15` | `int32_t` | Q15 output: -32768 to +32768 maps to -1.0 to +1.0. |
| `delaySamples` | `uint16_t` | Samples to wait before LFO starts (in seconds * sampleRate). |
| `delayCounter` | `uint16_t` | Countdown for delay phase. |

## Methods

### `void reset()`

**Description:**

Resets all LFO state to disabled.
