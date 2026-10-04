# AudioEvent

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

A fire-and-forget sound event triggered by the game.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `preset` | `const struct InstrumentPreset*` | Optional preset driving ADSR and LFO parameters. MUST be a pointer to a static, constexpr, or global instance. If nullptr, falls back to legacy default behavior. |
| `sweepEndHz` | `float` | Optional frequency/period sweep (PULSE / TRIANGLE / SINE / SAW / NOISE). Active iff sweepDurationSec > 0 and sweepEndHz > 0. Melodic: starts at `frequency`, ends at `sweepEndHz`. NOISE: interpolates LFSR clock Hz (and thus noisePeriodSamples). Curve: Linear (default) or Exponential (geometric in Hz); falls back to Linear if start/end Hz are not both > 0. Duration clamped to note length for one-shots; full sweepDurationSec when loop. API decision: ADR-A1 in docs/architecture/AUDIO_ROADMAP_SHORT_MEDIUM.md §A.7. |
| `loop` | `bool` | Continuous playback: voice stays enabled until STOP_CHANNEL (or steal). When false, duration <= 0 MUST NOT leave a hanging voice (disabled immediately). |
| `sweepCurve` | `SweepCurve` | Sweep interpolation; additive at end of struct for brace-init safety (0 = Linear). |
| `dutySteps` | `const SfxBreakpoint*` | Optional duty stepped table (PULSE). Active when dutySteps != nullptr and dutyStepCount > 0 (clamped to kMaxSfxDutySteps). Hold between points; ignores InstrumentPreset::dutySweep while active. |
| `pitchEnvelope` | `const SfxBreakpoint*` | Optional multi-breakpoint pitch envelope. Active when count >= 2 (clamped to kMaxSfxPitchPoints); then replaces single-segment sweep. |
