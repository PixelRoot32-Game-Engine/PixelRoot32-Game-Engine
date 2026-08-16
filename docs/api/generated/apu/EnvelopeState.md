# EnvelopeState

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Holds ADSR envelope state for a single voice.

Tracks the attack/decay/sustain/release stages and provides both
floating-point and Q15 fixed-point variants to support platforms
without an FPU (e.g., ESP32-C3 RISC-V).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `attackSamples` | `uint32_t` | Samples for attack phase (0 = instantaneous). |
| `decaySamples` | `uint32_t` | Samples for decay phase. |
| `sustainLevel` | `float` | Target volume at sustain phase [0.0 - 1.0]. |
| `releaseSamples` | `uint32_t` | Samples for release phase. |
| `sampleCounter` | `uint32_t` | Counter within current stage. |
| `currentLevel` | `float` | Current envelope amplitude [0.0 - 1.0]. |
| `attackDelta` | `float` | Volume increment per sample during attack: 1.0 / attackSamples. |
| `decayDelta` | `float` | Volume decrement per sample during decay: (1.0 - sustainLevel) / decaySamples. |
| `releaseDelta` | `float` | Volume decrement per sample during release: sustainLevel / releaseSamples. |
| `currentLevelQ15` | `int32_t` | Q15 mirror of currentLevel (-32768 to +32768). |
| `attackDeltaQ15` | `int32_t` | Q15 attack delta: 32768 / attackSamples. |
| `decayDeltaQ15` | `int32_t` | Q15 decay delta: (32768 - sustainLevelQ15) / decaySamples. |
| `sustainLevelQ15` | `int32_t` | Q15 sustain level. |
| `releaseDeltaQ15` | `int32_t` | Q15 release delta: sustainLevelQ15 / releaseSamples. |

## Methods

### `void reset()`

**Description:**

Resets all envelope state to OFF.
