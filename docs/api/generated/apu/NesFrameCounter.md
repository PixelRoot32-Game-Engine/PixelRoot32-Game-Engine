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
