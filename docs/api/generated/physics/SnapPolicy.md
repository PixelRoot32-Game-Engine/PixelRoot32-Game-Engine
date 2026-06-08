# SnapPolicy

<Badge type="info" text="Enum" />

**Source:** `KinematicActor.h`

## Description

Controls floor snap behavior after slide resolution.

- None: slide only (default).
- Step: guarded snap post-step when wasSnapFloor was true at entry.
- Continuous: reserved for future continuous floor adhesion; currently no-op.
