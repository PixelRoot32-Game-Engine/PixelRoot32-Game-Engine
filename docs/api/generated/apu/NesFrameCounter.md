# NesFrameCounter

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

NES APU frame counter (sequencer) state.

Counts APU cycles (CPU/2) and dispatches quarter/half clocks at the
step boundaries defined by nes_apu::kMode0ApuCyclesPerStep (4-step,
mode 0) and nes_apu::kMode1ApuCyclesPerStep (5-step, mode 1).

Default state: enabled=false (counter OFF), mode=0, all clocks
disabled. Consumers opt in with `ApuCore::setNesFrameCounterMode(0 or 1)`.
Hito 2 M3.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `mode` | `int` | 0 = 4-step (mode 0), 1 = 5-step (mode 1). Default is 0. |
| `enabled` | `bool` | When false, the frame counter does not advance and dispatches no clocks. Set to true by `ApuCore::setNesFrameCounterMode` and reset to false by `ApuCore::reset` (or unit-test reset). Default OFF keeps the canonical track PCM byte-for-byte identical. |
| `currentStep` | `int` | Current step within the cycle (0 = wrap point, 1..maxStep = active). |
| `apuCycles` | `double` | Continuous APU-cycle accumulator. Doubled per render sample. |
| `irqFlag` | `bool` | Frame interrupt flag. Set on mode-0 cycle wrap (if not inhibited). |
| `irqInhibit` | `bool` | $4017 bit 6 (inhibit). When true, the IRQ flag is cleared and not set. |
| `resetPending` | `bool` | Reset pending: when true, the next "step entry" zeroes the cycle accumulator and fires quarter+half (mirrors $4017 M=1 side effect). |
