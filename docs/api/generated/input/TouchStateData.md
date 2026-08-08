# TouchStateData

<Badge type="info" text="Struct" />

**Source:** `TouchStateMachine.h`

## Description

Per-touch-ID state tracking

## Properties

| Name | Type | Description |
|------|------|-------------|
| `state` | `TouchState` | Current state |
| `pressTime` | `uint32_t` | When press started (ms) |
| `pressX` | `int16_t` | X position at press |
| `pressY` | `int16_t` | Y position at press |
| `lastX` | `int16_t` | Last known X position |
| `lastY` | `int16_t` | Last known Y position |
| `longPressFired` | `bool` | Long press already fired |
| `dragStarted` | `bool` | Drag already started |

## Methods

### `, pressTime(0)`

### `, pressX(0)`

### `, pressY(0)`

### `, lastX(0)`

### `, lastY(0)`

### `, longPressFired(false)`

### `, dragStarted(false)`
