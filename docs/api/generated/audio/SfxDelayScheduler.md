# SfxDelayScheduler

<Badge type="info" text="Class" />

**Source:** `SfxBankPlayback.h`

## Description

Schedules delayed SFX events for `playSfxBank` sequence steps.

Games implement this with a scene timer / command queue. The helper does not
own timing and does not allocate.

Looping events (`AudioEvent::loop == true`) stay active until the game sends
`AudioCommandType::STOP_CHANNEL` (or the voice is stolen). This helper does
not auto-stop loops and does not manage cooldowns or global SFX volume.

## Methods

### `virtual void schedule(float delaySec, const AudioEvent& event)`

**Description:**

Schedule `playEvent(event)` after `delaySec` seconds from effect start.

**Parameters:**

- `delaySec`: Delay relative to the `playSfxBank` call (>= 0).
- `event`: Event to play when the delay elapses.
