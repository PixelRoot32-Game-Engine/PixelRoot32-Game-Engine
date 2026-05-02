# BaseDrawSurface

<Badge type="info" text="Class" />

**Source:** `BaseDrawSurface.h`

**Inherits from:** [DrawSurface](./DrawSurface.md)

## Description

Optional base class for DrawSurface implementations that provides default primitive rendering.

Users can inherit from this class to avoid implementing every single primitive.
At minimum, a subclass should implement:
- init()
- drawPixel()
- sendBuffer()
- clearBuffer()

## Inheritance

[DrawSurface](./DrawSurface.md) → `BaseDrawSurface`
