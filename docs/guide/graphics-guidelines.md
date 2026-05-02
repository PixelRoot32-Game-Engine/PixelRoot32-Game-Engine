# Graphics Guidelines - PixelRoot32

Patterns and best practices for rendering, sprites, tilemaps, and palette systems.

**Pipeline first:** read **[Rendering](./rendering.md)** for `Renderer`, layers, camera, and draw flow. This page focuses on **asset layout**, bitmap conventions, and tile/palette usage.

---

## 🎨 Sprite Guidelines

### 1bpp Sprite Definition

Define sprites as `static const uint16_t` arrays, one row per element:

```cpp
// ✅ 1bpp sprite: bit 0 = leftmost, bit (width-1) = rightmost
static const uint16_t playerBitmap[] = {
    0b0000001111000000,  // Row 0
    0b0000011111100000,  // Row 1
    0b0000111111110000,  // Row 2
    0b0000111111110000,  // Row 3
    0b0000010101000000,  // Row 4
};

pixelroot32::graphics::Sprite playerSprite = {
    .width = 16,
    .height = 5,
    .data = playerBitmap
};
```

### Sprite Descriptors

Always wrap bitmaps in descriptors:

```cpp
// ✅ Descriptor approach
void MyActor::draw(Renderer& r) {
    r.drawSprite(playerSprite, static_cast<int>(x), static_cast<int>(y));
}

// ❌ Never pass raw bitmaps
void MyActor::draw(Renderer& r) {
    r.drawBitmap(playerBitmap, ...);  // Wrong API
}
```

### Layered Sprites (Multi-Color)

Compose from multiple 1bpp layers:

```cpp
// ✅ Layered multi-color sprite
static const uint16_t shipOutline[] = { ... };
static const uint16_t shipCockpit[] = { ... };
static const uint16_t shipEngine[] = { ... };

pixelroot32::graphics::MultiSprite ship = {
    .layers = {
        {shipOutline, PaletteType::PR32},
        {shipCockpit, PaletteType::NES},
        {shipEngine, PaletteType::GB}
    },
    .layerCount = 3
};

renderer.drawMultiSprite(ship, x, y);
```

**Keep layer data `static const`** for flash storage.

### Higher Bit Depth (Optional)

Enable for specific use cases:

```cpp
// platformio.ini
-DPIXELROOT32_ENABLE_2BPP_SPRITES=1   // 2x memory
-DPIXELROOT32_ENABLE_4BPP_SPRITES=1   // 4x memory
```

| Format | Use For | Cost |
|--------|---------|------|
| 1bpp (default) | Gameplay sprites, tiles | 1x |
| 2bpp | Logos, detailed UI | 2x |
| 4bpp | Photos, title screens | 4x |

**Default to 1bpp** for gameplay-critical assets.

---

## 🗺️ Tilemap System

### Basic Tilemap

```cpp
// Tile indices (compact uint8_t)
static const uint8_t tileIndices[] = {
    0, 0, 0, 0,  // Row 0
    0, 1, 1, 0,  // Row 1 (tile 1 = ground)
    0, 1, 1, 0,  // Row 2
    0, 0, 0, 0,  // Row 3
};

pixelroot32::graphics::TileMap level = {
    .width = 4,
    .height = 4,
    .tileWidth = 16,
    .tileHeight = 16,
    .tiles = tileIndices,
    .tileset = groundTiles,
    .tilesetSize = 2  // 2 unique tiles
};

// Draw
renderer.drawTileMap(level, 0, 0);  // x, y offset
```

### Tile Reuse

Reuse tiles across the map to minimize flash:

```cpp
// ✅ Reuse: indices point to same tile data
// Tile 0 = sky (used 100 times)
// Tile 1 = ground (used 20 times)
// Total unique tiles: 2, not 120
```

### Scrolling with Camera

```cpp
// ✅ Centralized camera logic
class GameScene : public Scene {
    Camera2D camera;
    
public:
    void update(unsigned long dt) {
        camera.setTarget(player->position);
        camera.update(dt);
    }
    
    void draw(Renderer& r) {
        r.setDisplayOffset(-camera.getX(), -camera.getY());
        r.drawTileMap(backgroundLayer, 0, 0);
        Scene::draw(r);  // Actors
    }
};
```

---

## 🎬 Tile Animation

### Memory Budget

| Tileset Size | RAM Usage | % ESP32 DRAM |
|--------------|-----------|--------------|
| 64 tiles | 73 bytes | 0.02% |
| 128 tiles | 137 bytes | 0.04% |
| 256 tiles | 265 bytes | 0.08% |

**Start with 64-128 tiles.**

### Initialization

```cpp
// In PROGMEM
PIXELROOT32_SCENE_FLASH_ATTR const TileAnimation animations[] = {
    { 2, 4, 8, 0 },  // Water: tiles 2-5, 4 frames, 8× (1/60 s) ticks per cell
    { 6, 2, 6, 0 },  // Lava: tiles 6-7, 2 frames, 6× (1/60 s) ticks per cell
};

TileAnimationManager animManager(animations, 2, 64);

// Link to tilemap
TileMap2bpp backgroundLayer = {
    // ... other fields ...
    .animManager = &animManager  // Enables animations
};
```

### Game Loop Integration

```cpp
void MyScene::update(unsigned long dt) {
    animManager.step(dt);  // Wall-time pacing (see TileAnimationManager API)
    Scene::update(dt);
}

void MyScene::draw(Renderer& r) {
    r.drawTileMap(backgroundLayer, 0, 0);
    Scene::draw(r);
}
```

### Speed Control

Animation speed is driven by **`frameDuration`** in **`TileAnimation`** data (larger value → each sprite frame held longer). Do not gate **`step(dt)`** on engine loop count to change speed; that breaks wall-clock pacing when the loop runs faster than the display.

```cpp
// Pause when game paused (do not call step while frozen)
if (!isPaused) {
    animManager.step(dt);
}
```

### Common Pitfalls

1. **Sequential frames only**: Tiles 2,3,4,5 - not 2,5,9,12
2. **Shared state**: All instances of a tile share the same frame
3. **StaticTilemapLayerCache**: If tilemap is in **static** group, advancing tile animation requires `invalidate()` on that cache when applicable

---

## 🎨 Multi-Palette Systems

### Slot-Based Palettes

Separate palettes for sprites and backgrounds:

```cpp
// Initialize during scene init
void MyScene::init() {
    pixelroot32::graphics::Color::enableDualPaletteMode(true);
    
    // Background slots (for tilemaps)
    pixelroot32::graphics::initBackgroundPaletteSlots();
    setBackgroundPaletteSlot(0, PaletteType::PR32);  // Ground
    setBackgroundPaletteSlot(1, PaletteType::NES);   // Water
    setBackgroundPaletteSlot(2, PaletteType::GB);    // Underground
    
    // Sprite slots
    pixelroot32::graphics::initSpritePaletteSlots();
    setSpritePaletteSlot(0, PaletteType::PR32);  // Player
    setSpritePaletteSlot(1, PaletteType::NES);   // Fire enemies
    setSpritePaletteSlot(2, PaletteType::GBC);   // Ice enemies
}
```

### Custom Palettes

```cpp
// In PROGMEM
static const uint16_t CUSTOM_FIRE[] = {
    0x0000, 0xFFFF, 0xF800, 0xFC00,  // Colors 0-3
    0xFA00, 0xF800, 0xF600, 0xF400,  // Colors 4-7
    // ... 16 colors total
};

// Apply to slot
setSpriteCustomPaletteSlot(5, CUSTOM_FIRE);
```

### Batching with Context

```cpp
// ✅ Set context once for many sprites
void BulletManager::drawAll(Renderer& r) {
    r.setSpritePaletteSlotContext(1);  // Fire palette
    
    for (auto& bullet : bullets) {
        r.drawSprite(bulletSprite, bullet.x, bullet.y);
    }
    
    r.setSpritePaletteSlotContext(0xFF);  // Reset
}
```

### Slot Documentation

```cpp
// Background slots:
// 0: Default ground (PR32)
// 1: Water areas (NES - blue)
// 2: Underground (GB - green)
// 3: Lava (custom red)
// 4-7: Reserved

// Sprite slots:
// 0: Player (PR32)
// 1: Fire enemies (NES)
// 2: Ice enemies (GBC)
// 3: Boss (PICO8)
// 4-7: Reserved
```

---

## 🧱 Render Layers

Standard layer assignment:

| Layer | Content |
|-------|---------|
| 0 | Background (tilemaps, fills) |
| 1 | Gameplay (player, enemies, bullets) |
| 2 | UI (HUD, menus, text) |

```cpp
class MyActor : public Actor {
public:
    MyActor() {
        renderLayer = 1;  // Gameplay layer
    }
};

class HUD : public Actor {
public:
    HUD() {
        renderLayer = 2;  // UI layer (top)
    }
};
```

---

## 📚 Related Documentation

| Document | Topic |
|----------|-------|
| [UI Guidelines](ui-guidelines.md) | Layouts, panels, HUDs |
| [Performance](performance/esp32-performance.md) | Hot paths, optimization |
| [Tile animation](../architecture/tile-animation.md) | Animation system deep dive |
| [API Graphics](../api/graphics.md) | Complete graphics API |

---

*Graphics should be efficient, authentic, and layer-friendly.*
