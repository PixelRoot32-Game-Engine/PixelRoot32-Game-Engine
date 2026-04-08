# UI Guidelines - PixelRoot32

Patterns and best practices for building user interfaces with the UI system.

---

## 🎨 Layout System

### Choosing the Right Layout

| Layout | Use Case | Navigation |
|--------|----------|------------|
| `UIVerticalLayout` | Lists, menus | UP/DOWN |
| `UIHorizontalLayout` | Toolbars, option rows | LEFT/RIGHT |
| `UIGridLayout` | Inventories, galleries | 4-direction |
| `UIAnchorLayout` | HUD elements, fixed positions | None |

### Vertical Layout (Lists)

```cpp
// ✅ Simple list with automatic navigation
UIVerticalLayout menu(10, 20);  // x=10, y=20
menu.setSpacing(4);

menu.addChild(std::make_unique<UIButton>("Start"));
menu.addChild(std::make_unique<UIButton>("Options"));
menu.addChild(std::make_unique<UIButton>("Quit"));

// UP/DOWN automatically handled
// Selection wraps at edges
```

### Horizontal Layout (Bars)

```cpp
// ✅ Toolbar or button row
UIHorizontalLayout toolbar(10, 200);
toolbar.setSpacing(8);

toolbar.addChild(std::make_unique<UIButton>("A"));
toolbar.addChild(std::make_unique<UIButton>("B"));
toolbar.addChild(std::make_unique<UIButton>("C"));

// LEFT/RIGHT automatically handled
```

### Grid Layout (Inventories)

```cpp
// ✅ Matrix layout with 4-direction navigation
UIGridLayout inventory(10, 50, 4, 3);  // x, y, cols, rows
inventory.setCellSize(32, 32);
inventory.setSpacing(4);

for (int i = 0; i < 12; ++i) {
    inventory.addChild(std::make_unique<UIItemSlot>(i));
}

// UP/DOWN/LEFT/RIGHT with wrapping
```

### Anchor Layout (HUDs)

```cpp
// ✅ Fixed positions without manual calculation
UIAnchorLayout hud;

// Score at top-left
auto score = std::make_unique<UILabel>("Score: 0");
hud.addChild(std::move(score), UIAnchor::TopLeft, 8, 8);

// Lives at top-right
auto lives = std::make_unique<UILabel>("Lives: 3");
hud.addChild(std::move(lives), UIAnchor::TopRight, -8, 8);

// No reflow - positions calculated once
```

---

## 🧱 Container Patterns

### Padding Container

Add spacing around elements or nest layouts:

```cpp
// ✅ Padding around a button
auto paddedButton = std::make_unique<UIPaddingContainer>(8);  // 8px padding
paddedButton->addChild(std::make_unique<UIButton>("Click Me"));

// ✅ Nesting layouts with spacing
auto outer = std::make_unique<UIVerticalLayout>(0, 0);
auto inner = std::make_unique<UIPaddingContainer>(16);
inner->addChild(std::make_unique<UIButton>("Nested"));
outer->addChild(std::move(inner));
```

### Panel (Visual Containers)

Create retro-style windows and dialogs:

```cpp
// ✅ Game & Watch style dialog
auto dialog = std::make_unique<UIPanel>(50, 50, 140, 100);
dialog->setTitle("Pause");

auto layout = std::make_unique<UIVerticalLayout>(0, 0);
layout->addChild(std::make_unique<UIButton>("Resume"));
layout->addChild(std::make_unique<UIButton>("Quit"));

dialog->addChild(std::move(layout));
```

---

## 🎮 Navigation

### Automatic Navigation

Layouts handle D-pad input automatically:

```cpp
// VerticalLayout: UP/DOWN
UIVerticalLayout menu;
menu.addChild(button1);  // First selected
menu.addChild(button2);  // DOWN goes here
menu.addChild(button3);  // DOWN goes here

// UP at top wraps to bottom (configurable)
// DOWN at bottom wraps to top
```

### Manual Selection

```cpp
// Programmatically select
layout.setSelectedIndex(2);  // Select third item

// Get current
int current = layout.getSelectedIndex();
UIElement* selected = layout.getSelectedChild();
```

---

## ⚡ ESP32 Performance

### Viewport Culling

Layouts automatically cull off-screen elements:

```cpp
// ✅ Only visible items rendered
UIVerticalLayout longList(0, 0);
for (int i = 0; i < 100; ++i) {
    longList.addChild(std::make_unique<UIButton>("Item " + i));
}
// Items outside viewport not drawn
```

### Optimized Clearing

Layouts clear only when scroll changes:

```cpp
// ✅ Efficient: No redraw if position unchanged
void update(unsigned long dt) {
    if (scrollChanged) {
        layout.setScrollOffset(newOffset);
        // Redraw happens here
    }
    // No redraw if offset same
}
```

### Instant Scroll (NES-style)

Default behavior for responsive navigation:

```cpp
// ✅ Instant jump on selection change
layout.setScrollMode(UIScrollMode::Instant);

// Optional: Smooth for manual scroll
layout.setScrollMode(UIScrollMode::Smooth);
```

---

## 🎨 Sprite & Graphics

### Sprite Descriptors

Wrap bitmaps in descriptors before rendering:

```cpp
// ✅ Use Sprite descriptor
pixelroot32::graphics::Sprite playerSprite = {
    .width = 16,
    .height = 16,
    .data = playerBitmap
};

renderer.drawSprite(playerSprite, x, y, paletteSlot, flipX);
```

### Multi-Sprite (Layered)

Compose multi-color sprites from 1bpp layers:

```cpp
// ✅ Layered sprite for color
pixelroot32::graphics::MultiSprite ship = {
    .layers = {
        {shipOutline, PaletteType::PR32},    // Outline
        {shipCockpit, PaletteType::NES},     // Cockpit detail
        {shipEngine, PaletteType::GB}          // Engine glow
    },
    .layerCount = 3
};

renderer.drawMultiSprite(ship, x, y);
```

**Keep layer data `static const`** for flash storage.

### Bit Depth Guidelines

| Format | Use For | Memory Cost |
|--------|---------|-------------|
| 1bpp | Game sprites, tiles | 1x |
| 2bpp | Logos, detailed UI | 2x |
| 4bpp | Photos, rich graphics | 4x |

**Default to 1bpp** for gameplay-critical sprites.

---

## 🚫 What Actors Should NOT Do

```cpp
// ❌ WRONG: Actors iterating pixels
void MyActor::draw(Renderer& r) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (bitmap[y] & (1 << x)) {
                r.drawPixel(x, y, color);  // NEVER!
            }
        }
    }
}

// ✅ CORRECT: Use renderer
void MyActor::draw(Renderer& r) {
    r.drawSprite(mySprite, position.x, position.y);
}
```

---

## 📚 Related Documentation

| Document | Topic |
|----------|-------|
| [GRAPHICS_GUIDELINES.md](GRAPHICS_GUIDELINES.md) | Sprites, tilemaps, palettes |
| [CODING_STYLE.md](CODING_STYLE.md) | C++ conventions |
| [performance/ESP32_PERFORMANCE.md](performance/ESP32_PERFORMANCE.md) | Optimization |
| [API_UI.md](api/API_UI.md) | Complete UI API reference |

---

*UIs should be responsive, efficient, and retro-authentic.*
