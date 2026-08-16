# GridMotion

<Badge type="info" text="Struct" />

**Source:** `GridMotion.h`

## Description

Plain five-`int` aggregate: logical cell, target cell, progress.

At rest, `toX == cellX`, `toY == cellY` and `progress == 0`. In flight,
`progress` runs 1..stepsPerCell-1 and the logical cell still names the cell
being LEFT — it flips only on arrival. Gameplay rules that read the logical
cell therefore never see an actor occupying two cells, or none.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `cellX` | `int` | Logical cell X — the one every gameplay rule reads. |
| `cellY` | `int` | Logical cell Y — the one every gameplay rule reads. |
| `toX` | `int` | Target cell X; equals cellX when at rest. |
| `toY` | `int` | Target cell Y; equals cellY when at rest. |
| `progress` | `int` | 0..stepsPerCell-1; 0 means "at rest in (cellX, cellY)". |
