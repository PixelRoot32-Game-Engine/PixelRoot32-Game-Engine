# Tile Animation System — Architecture Diagram

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                       PIXELROOT32 ENGINE                    │
│                                                             │
│  ┌────────────┐      ┌──────────────┐      ┌────────────┐   │
│  │   Scene    │────▶│   Renderer   │─────▶│  Display   │   │
│  └────────────┘      └──────────────┘      └────────────┘   │
│        │                     │                              │
│        │                     │                              │
│        ▼                     ▼                              │
│  ┌────────────┐      ┌──────────────┐                       │
│  │ Animation  │      │   TileMap    │                       │
│  │  Manager   │◀────│   Generic    │                       │
│  └────────────┘      └──────────────┘                       │
│        │                     │                              │
│        ▼                     ▼                              │
│  ┌────────────┐      ┌──────────────┐                       │
│  │  Lookup    │      │   Tileset    │                       │
│  │   Table    │      │  (PROGMEM)   │                       │
│  └────────────┘      └──────────────┘                       │
└─────────────────────────────────────────────────────────────┘
```

### Component Roles

| Component        | Responsibility                                      |
| ---------------- | --------------------------------------------------- |
| Scene            | Owns the animation manager and controls update flow |
| Renderer         | Draws tilemaps and resolves animation frames        |
| TileMapGeneric   | Stores tile indices and tileset references          |
| AnimationManager | Updates animation state and manages lookup table    |
| LookupTable      | Fast remapping from tile index → animated frame     |
| Tileset          | Static tile graphics stored in PROGMEM              |

---

# Data Flow

## Initialization (Scene::init)

```
1. Scene creates TileAnimationManager
   ├─ Reads TileAnimation[] from PROGMEM
   ├─ Initializes lookupTable[MAX_TILESET_SIZE]
   └─ Sets identity mapping (i → i), then rebuilds animated ranges for frame 0

2. Scene links manager to tilemap
   └─ backgroundLayer.animManager = &animManager
```

After initialization:

```
Tilemap indices → AnimationManager → Tileset
```

---

# Update Loop (Scene::update)

```
┌─────────────────────────────────────────────────┐
│ Scene::update(deltaTime)                        │
│                                                 │
│   └─▶ animManager.step(deltaTime)               |
│        │                                        │
│        ├─ Wall pacing (~60 Hz max):             │
│        │   accumulate micros between calls      │
│        │   (fallback: deltaTime when 0)         │
│        │   → emit 0..N logical ticks            │
│        │                                        │
│        ├─ For each tick: globalFrameCounter++   │
│        │                                        │
│        └─ For each animation:                   │
│             │                                   │
│             ├─ currentFrame =                   │
│             │   (counter / duration) % frames   │
│             │                                   │
│             ├─ currentTile =                    │
│             │   baseTile + currentFrame         │
│             │                                   │
│             └─ Update lookup table:             │
│                 for f in 0..frameCount-1        │
│                     lookupTable[base+f] =       │
│                       currentTile               │
│                                                 │
│   Return O(1) when no tick; else O(A×F)         │
└─────────────────────────────────────────────────┘
```

Key properties:

• deterministic update cost when the lookup rebuilds (same as before)
• no dynamic allocations
• small constant runtime; pacing decoupled from main-loop iteration rate

---

# Render Loop (Scene::draw)

```
┌─────────────────────────────────────────────┐
│ Renderer::drawTileMap(map, x, y)            │
│                                             │
│   ├─ Viewport culling                       │
│   │   (compute visible tiles)               │
│   │                                         │
│   └─ For each visible tile:                 │
│        │                                    │
│        ├─ index = map.indices[i]            │
│        │                                    │
│        ├─ if (map.animManager)              │
│        │     index =                        │
│        │       map.animManager              │
│        │         ->resolveFrame(index)      │
│        │                                    │
│        ├─ Resolve palette                   │
│        │   (for 2bpp / 4bpp modes)          │
│        │                                    │
│        └─ drawSprite(                       │
│             map.tiles[index], x, y)         │
│                                             │
└─────────────────────────────────────────────┘
```

Animation integration cost:

```
1 array lookup per tile
```

---

# Memory Layout

## PROGMEM (Flash)

Animation definitions are stored in flash.

```
TileAnimation animations[] =
{
  { baseTile: 42, frameCount: 4, frameDuration: 8, reserved: 0 },
  { baseTile: 46, frameCount: 2, frameDuration: 6, reserved: 0 }
};
```

Memory cost:

```
4 bytes × animationCount
```

Example:

```
8 animations = 32 bytes
```

---

## RAM Allocation

```
TileAnimationManager
```

```
animations pointer        4 bytes
animCount                 1 byte
tileCount                 2 bytes (uint16_t - soporta 512+ tiles)
globalFrameCounter        2 bytes
lookupTable[N]            N bytes
```

Total RAM usage:

```
N + 9 bytes
```

**Validación:**
- Compile-time: `static_assert(MAX_TILESET_SIZE >= 64)`
- Runtime (debug): Verifica `tileCount <= MAX_TILESET_SIZE`

Example:

```
N = 256 tiles
Total = 265 bytes RAM
```

---

# Lookup Table Mechanics

### Animation Model

PixelRoot32 uses a **single-frame remap model**:

All tiles in an animation range always point to the **same current frame**.

This matches the behaviour used in many retro engines.

---

## Initial State (Identity Mapping)

```
lookupTable[0]  = 0
lookupTable[1]  = 1
...
lookupTable[42] = 42
lookupTable[43] = 43
lookupTable[44] = 44
lookupTable[45] = 45
```

---

## Animation Example

Water animation (`frameDuration` = **8 logical ticks at ~60 Hz** ≈ **133 ms** per animation cell):

```
baseTile = 42
frameCount = 4
frameDuration = 8
```

Frames stored in tiles:

```
42 43 44 45
```

---

### Logical ticks 0–7 (counter 0–7)

```
currentFrame = (counter / 8) % 4 = 0
currentTile  = 42
```

```
lookupTable[42] = 42
lookupTable[43] = 42
lookupTable[44] = 42
lookupTable[45] = 42
```

---

### Logical ticks 8–15 (counter 8–15)

```
currentFrame = 1
currentTile  = 43
```

```
lookupTable[42] = 43
lookupTable[43] = 43
lookupTable[44] = 43
lookupTable[45] = 43
```

---

### Logical ticks 16–23 (counter 16–23)

```
currentFrame = 2
currentTile  = 44
```

```
lookupTable[42] = 44
lookupTable[43] = 44
lookupTable[44] = 44
lookupTable[45] = 44
```

---

### Logical ticks 24–31 (counter 24–31)

```
currentFrame = 3
currentTile  = 45
```

```
lookupTable[42] = 45
lookupTable[43] = 45
lookupTable[44] = 45
lookupTable[45] = 45
```

---

### Loop

After frame 31:

```
currentFrame = 0
```

The animation repeats.

---

# Rendering Pipeline

## Without Animation

```
Tilemap index
      │
      ▼
Tileset lookup
      │
      ▼
Renderer
      │
      ▼
Display
```

---

## With Animation

```
Tilemap index
      │
      ▼
AnimationManager.resolveFrame()
      │
      ▼
Tileset lookup
      │
      ▼
Renderer
      │
      ▼
Display
```

---

# Performance Characteristics

## Time Complexity

```
Operation        Complexity    Typical Time
--------------------------------------------
Constructor      O(N)          ~50 µs
step()           O(A × F)      ~3–7 µs
resolveFrame()   O(1)          ~0.02 µs
reset()          O(N)          ~50 µs
```

Where:

```
N = tileCount
A = animation count
F = frames per animation
```

---

## Space Complexity

```
Component              Size        Location
-------------------------------------------
TileAnimation[]        4 × A       PROGMEM
lookupTable            N bytes     RAM
animations pointer     4 bytes     RAM
animCount              1 byte      RAM
tileCount              2 bytes     RAM
globalFrameCounter     2 bytes     RAM
```

Total RAM:

```
N + 9 bytes
```

Example:

```
256 tiles → 265 bytes RAM
```

---

# Configuration Matrix

```
MAX_TILESET_SIZE | RAM Usage
-----------------|-----------
64               | 73 bytes
128              | 137 bytes
256              | 265 bytes
512              | 521 bytes
```

Note:

```
tileCount is uint16_t
```

This allows expansion beyond 256 tiles in future versions.

---

# Retro Console Comparison

### NES

```
Animation method:
CHR bank switching
```

PixelRoot32 equivalent:

```
lookup table remapping
```

---

### SNES

```
Animation method:
VRAM updates during VBlank
```

PixelRoot32 equivalent:

```
AnimationManager::step() during update phase
```

---

### Game Boy Advance

```
Animation method:
VRAM tile updates
```

PixelRoot32 advantage:

```
No VRAM writes required
Only an array lookup
```

---

# Summary

Architecture characteristics:

```
Memory:        ~265 bytes RAM
CPU overhead:  <0.2%
Allocations:   zero
Lookup cost:   O(1)
Integration:   minimal changes
```

Result:

```
Production-ready tile animation system
optimized for ESP32-class hardware
```

---

## Related Documentation

### Framebuffer Optimization (v1.2.2+)

For additional rendering optimizations, see [ARCHITECTURE.md](../ARCHITECTURE.md):

- **`shouldRedrawFramebuffer()`**: Scene method that conditionally skips `draw()` and `present()` calls when visual state hasn't changed, reducing unnecessary rendering.
- **`getVisualSignature()`**: Visual signature computation to efficiently detect framebuffer changes and avoid redundant redraws.

See [ARCHITECTURE.md - Rendering Pipeline](../ARCHITECTURE.md#rendering-pipeline) for details.
