# ImmediateSfxDelayScheduler

<Badge type="info" text="Class" />

**Source:** `SfxBankPlayback.h`

**Inherits from:** [SfxDelayScheduler](./SfxDelayScheduler.md)

## Description

Plays every scheduled event immediately (ignores delay). Test / stub only.

## Inheritance

[SfxDelayScheduler](./SfxDelayScheduler.md) → `ImmediateSfxDelayScheduler`

## Methods

### `explicit ImmediateSfxDelayScheduler(AudioEngine& engine) : engine_(&engine)`

### `void schedule(float /*delaySec*/, const AudioEvent& event)`
