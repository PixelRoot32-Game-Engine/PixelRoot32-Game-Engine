# Tilemap Editor - Technical Reference

**Level**: ⭐⭐⭐ Advanced

---

> **Quick Index**
> - [Engine Limits](#engine-limits)
> - [File Formats](#file-formats)
> - [Project Structure](#project-structure)
> - [API Services](#api-services)
> - [C++ Export](#c-export)
> - [Data Formats](#data-formats)
> - [Compatibility](#compatibility)
> - [Glossary](#glossary)

---

## Engine Limits

### ⚠️ Limits Table

| Parameter | Limit | Description |
|-----------|--------|-------------|
| **MAX_TILE_WIDTH** | 32 px | Maximum tile size |
| **MAX_TILE_HEIGHT** | 32 px | Maximum tile size |
| **MAX_MAP_DIM** | 255 tiles | Maximum map dimension |
| **MAX_UNIQUE_TILES** | 256 | Unique tiles per project |
| **MAX_LAYERS** | 8 | Layers per scene |
| **MAX_SCREEN_WIDTH** | 320 px | Screen width |
| **MAX_SCREEN_HEIGHT** | 240 px | Screen height |
| **MAX_ANIMATIONS** | 64 | Animations per scene |
| **MAX_ANIMATION_FRAMES** | 256 | Total frames |
| **MIN_FRAME_COUNT** | 1 | Minimum frames |
| **MAX_FRAME_COUNT** | 255 | Maximum frames |
| **MIN_FRAME_DURATION** | 1 | Minimum duration (ticks) |
| **MAX_FRAME_DURATION** | 255 | Maximum duration (ticks) |

> ⚠️ **Correction**: Previous docs said 4 layers. Actual limit is **8 layers**.

### Screen Resolutions

**Landscape**:
- Maximum: 320×240 px
- Aspect ratio: 4:3

**Portrait**:
- Maximum: 240×320 px
- Aspect ratio: 3:4

---

## File Formats

### Supported Formats

| Format | Extension | Advantages | Disadvantages |
|--------|-----------|------------|-------------|
| **JSON** | `.pr32scene` | Human-readable, git-friendly | Large files |
| **Binary** | `.pr32scene.bin` | Up to 335× smaller, 10× faster | Not readable |

### Binary Format Versions

| Version | Features | Compatibility |
|---------|----------|---------------|
| 1 | Basic | ✅ Compatible |
| 2 | Tile attributes | ✅ Compatible |
| 3 | Palette slots | ✅ Compatible |
| 4 | Multi-palette complete | ✅ Compatible |

> ⚠️ **Correction**: Previous docs said version 3. Actual is **version 4**.

### Compression Benchmarks

| Project | JSON | Binary | Reduction |
|----------|------|--------|----------|
| Small (1 scene) | 2.6 KB | 355 bytes | **86%** |
| Medium (3 scenes) | 227 KB | 752 bytes | **99.7%** |
| Large (10 scenes) | ~1 MB | ~5 KB | **99.5%** |

### Performance

| Operation | JSON | Binary | Improvement |
|-----------|------|--------|------------|
| **Save** | 20ms | 2ms | **10×** |
| **Load** | 60ms | 21ms | **3×** |

---

## Project Structure

### File Structure

```
my_project/
├── my_project.pr32scene      # Main file
├── my_project.pr32scene.bin  # Binary version (optional)
├── tile_flag_rules.json   # Custom rules (optional)
└── assets/
    └── tilesets/
        ├── tileset1.png
        └── tileset2.png
```

### Exported Files

```
output/
├── my_scene.h              # Declarations
├── my_scene.cpp            # Data (palettes, tiles, indices)
├── my_scene_animations.h  # Animation declarations (if any)
├── my_scene_animations.cpp # Animation data (if any)
└── shared_palette.h    # Shared palette (single palette mode)
```

---

## API Services

### Services

#### ProjectService

```python
class ProjectService:
    def create_project(self, name: str, tile_size: int, ...) -> ProjectModel:
        """Create new project"""
    
    def load_project(self, path: str) -> ProjectModel:
        """Load project"""
    
    def save_project(self, project: ProjectModel, binary: bool = False):
        """Save project"""
    
    def export_to_cpp(self, project: ProjectModel, output_dir: str) -> Dict:
        """Export to C++"""
```

#### ExporterService

```python
class ExporterService:
    def can_export(self) -> bool:
        """Check if user can export (requires valid license)"""
    
    def export_project(self, project, output_dir: str) -> Dict:
        """Export project to C++ files"""
        # Auto-detects single/multi-palette
```

#### ValidationService

```python
class ValidationService:
    def validate_project(self, project: ProjectModel) -> ValidationResult:
        """Validate full project"""
    
    def validate_animations(self, animations) -> ValidationResult:
        """Validate against limits"""
```

#### AnimationService

```python
class AnimationService:
    def create_animation(self, name: str) -> TileAnimation:
        """Create animation"""
    
    def link_to_tile(self, animation: TileAnimation, base_tile: int):
        """Link animation to tile"""
    
    def export_animations(self, animations, output_dir: str):
        """Export animations to C++"""
```

---

## C++ Export

### Requirements

⚠️ **Important**: C++ export **requires a valid license**.

- No license: Button shows 🔒
- Other features work without license

### Export Options

| Option | Description | Recommended |
|--------|-------------|-------------|
| **C++ Namespace** | Namespace for code | Project name |
| **Color Depth** | Bit depth (auto-detect) | Auto-detect |
| **Store in Flash** | Save to PROGMEM | ✅ Always |
| **Legacy Format** | Without Flash attributes | Compatibility only |

### Export Mode

| Mode | Trigger | Generated |
|------|---------|-----------|
| **Single Palette** | All layers use P0 | Shared palette |
| **Multi-Palette** | Any layer P1-P7 | Per-slot palettes |

### Generated Files

#### Single Palette

```cpp
// level1.h
extern const uint16_t TILEMAP_PALETTE_DATA[];
extern const pixelroot32::graphics::Sprite4bpp TILESET_SPRITES[];
extern const pixelroot32::graphics::TileMap layer_foreground;

// level1.cpp
static const uint16_t TILEMAP_PALETTE_DATA[] = { /* RGB565 */ };
static const pixelroot32::graphics::Sprite4bpp TILESET_SPRITES[] = { /* tiles */ };
static const uint8_t LAYER_FOREGROUND_INDICES[] = { /* indices */ };

void init() {
    layer_foreground.palette = TILEMAP_PALETTE_DATA;
    layer_foreground.tiles = TILESET_SPRITES;
    layer_foreground.indices = LAYER_FOREGROUND_INDICES;
}
```

#### Multi-Palette

```cpp
// level1.h
// setBackgroundCustomPaletteSlot(1, PLATFORMS_PALETTE);
extern const uint16_t PLATFORMS_PALETTE[];
extern const uint16_t STAIRS_PALETTE[];
extern const pixelroot32::graphics::Sprite4bpp PLATFORMS_TILESET_SPRITES[];

// level1.cpp
void init() {
    setBackgroundCustomPaletteSlot(1, PLATFORMS_PALETTE);
    setBackgroundCustomPaletteSlot(2, STAIRS_PALETTE);
}
```

### Engine Integration

**Single Palette**:
```cpp
#include "level1.h"

level1::init();
renderer.drawTileMap(level1::layer_background, x, y);
```

**Multi-Palette**:
```cpp
#include "level1.h"

level1::init();  // Registers palettes
renderer.drawTileMap(level1::background, 0, 0);
renderer.drawTileMap(level1::platforms,  0, 0);
```

**Attributes/Flags**:
```cpp
// Query attributes
const char* type = level1::get_tile_attribute(0, x, y, "type");

// Query flags
uint8_t flags = level1::behavior_layer_background[y * width + x];
if (flags & TILE_SOLID) { /* collision */ }
```

**Animations**:
```cpp
level1::get_animation_manager().step();
renderer.drawTileMap(level1::layer_background, x, y);
```

---

## Data Formats

### Palette

- **Format**: RGB565
- **Size**: 16 colors max
- **Index 0**: Transparent (multi-bpp)

### Tiles

| BPP | Per Row | Colors |
|-----|--------|--------|
| 1 bpp | 1 byte | 2 |
| 2 bpp | 2 bytes | 4 |
| 4 bpp | 4 bytes | 16 |

### Index Map

- 1 byte per cell (`uint8_t`)
- Value -1 (editor) = Index 0 (export) = Empty

### BPP Auto-Detection

| Colors Used | BPP | Maximum |
|------------|-----|----------|
| 1-2 | 1 bpp | 2 |
| 3-4 | 2 bpp | 4 |
| 5-16 | 4 bpp | 16 |

---

## Compatibility

### Dependencies

| Package | Minimum Version |
|---------|--------------|
| **Python** | 3.8+ |
| **Tkinter** | 8.6+ |
| **ttkbootstrap** | 1.0+ |
| **Pillow** | 9.0+ |

### Target Hardware

- **ESP32** (PixelRoot32 engine)
- Flash: 4MB minimum recommended
- RAM: 520KB minimum

---

## Glossary

| Term | Definition |
|------|------------|
| **ENGINE_LIMITS** | Engine limit constants |
| **ProjectModel** | Project class |
| **SceneModel** | Scene model |
| **LayerModel** | Layer model |
| **TileAnimation** | Tile animation model |
| **RGB565** | Color format (5+6+5 bits) |
| **BPP** | Bits per pixel |
| **PROGMEM** | ESP32 flash storage |
| **Sprite4bpp** | 4bpp sprite |
| **TileMap** | Exported tilemap structure |

---

## Related Guides

- [Quick Start](/tools/tilemap-editor/quick-start) - 5 minute guide
- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - Advanced features