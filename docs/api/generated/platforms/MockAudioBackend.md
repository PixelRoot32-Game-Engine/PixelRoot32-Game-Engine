# MockAudioBackend

<Badge type="info" text="Class" />

**Source:** `MockAudioBackend.h`

**Inherits from:** [AudioBackend](../audio/AudioBackend.md)

## Description

Mock implementation of AudioBackend for unit testing.

This mock captures initialization state and provides test hooks
for verifying AudioScheduler and AudioEngine interactions.

## Inheritance

[AudioBackend](../audio/AudioBackend.md) → `MockAudioBackend`

## Methods

### `bool wasInitCalled() const`

### `AudioEngine* getEngine() const`

### `void reset()`
