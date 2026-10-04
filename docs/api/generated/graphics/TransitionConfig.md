# TransitionConfig

<Badge type="info" text="Struct" />

**Source:** `TransitionEffect.h`

## Description

Full per-call description of a scene transition (issue #240).

Passed by value to Engine::triggerTransition() / SceneManager::transitionToScene().
A value type scales better than one overload per parameter, and passing the
whole description on every call prevents state carry-over between
transitions (a wipe triggered later never inherits the direction,
sub-step or iris centers of an earlier one).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `type` | `TransitionType` | Fade, Iris or DiagonalWipe. |
| `durationMs` | `unsigned long` | Duration of each phase (Out and In). |
| `wipeDirection` | `WipeDirection` | DiagonalWipe corner direction. |
| `irisOutCx` | `int` | Iris center X, Out phase (-1 = buffer center). |
| `irisOutCy` | `int` | Iris center Y, Out phase (-1 = buffer center). |
| `irisInCx` | `int` | Iris center X, In phase (-1 = buffer center). |
| `irisInCy` | `int` | Iris center Y, In phase (-1 = buffer center). |
| `subStepMs` | `uint16_t` | DiagonalWipe sub-step (0 = disabled). |
