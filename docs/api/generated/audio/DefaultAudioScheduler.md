# DefaultAudioScheduler

<Badge type="info" text="Class" />

**Source:** `DefaultAudioScheduler.h`

**Inherits from:** [AudioScheduler](./AudioScheduler.md)

## Description

Backend-driven scheduler used on platforms without a dedicated
       audio task.

Delegates all synthesis to ApuCore; generateSamples() runs in whichever
context the backend invokes it (tests, simulators without a thread,
etc.).

## Inheritance

[AudioScheduler](./AudioScheduler.md) → `DefaultAudioScheduler`

## Methods

### `const ApuCore& core() const`

### `ApuCore& core()`
