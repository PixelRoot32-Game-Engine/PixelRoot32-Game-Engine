# UI Guidelines - PixelRoot32

Patterns and best practices for building user interfaces with the UI system.

**Architecture first:** read **[UI system](./ui-system.md)** for `UIManager`, entity vs touch registration, and lifecycle. Authoritative API surface: `include/graphics/ui/` and [UI module](../api/ui.md).

---

## Layout system

### Choosing the right layout

| Layout | Use case | Navigation |
|--------|----------|------------|
| `UIVerticalLayout` | Lists, menus | UP/DOWN (via `handleInput`) |
| `UIHorizontalLayout` | Toolbars, option rows | LEFT/RIGHT |
| `UIGridLayout` | Inventories, galleries | Four-way |
| `UIAnchorLayout` | HUD elements, fixed positions | None at layout level |

Layouts take a **viewport rectangle** `(x, y, width, height)` and use **`addElement` / `removeElement`**, not `addChild`.

### Vertical layout (lists)

```cpp
using namespace pixelroot32;

void onStart() { /* ... */ }

auto* menu = new graphics::ui::UIVerticalLayout(
    math::toScalar(10), math::toScalar(20),
    200, 120);
menu->setSpacing(4);

auto* start = new graphics::ui::UIButton(
    "Start", 0,
    math::Vector2(math::toScalar(0), math::toScalar(0)),
    math::Vector2(math::toScalar(80), math::toScalar(24)),
    onStart);
menu->addElement(start);
// ... more buttons

// From scene update: drive D-pad / focus navigation
menu->handleInput(engine.getInputManager());
```

### Horizontal layout (bars)

```cpp
auto* toolbar = new graphics::ui::UIHorizontalLayout(
    math::toScalar(10), math::toScalar(200),
    240, 32);
toolbar->setSpacing(8);
// toolbar->addElement(... UIButton with full constructor ...)
```

### Grid layout (inventories)

Grid **cell size is computed** from the layout’s width/height, `setColumns`, padding, and `setSpacing`. There is **no** `setCellSize`.

```cpp
auto* inventory = new graphics::ui::UIGridLayout(
    math::toScalar(10), math::toScalar(50),
    200, 160);
inventory->setColumns(4);
inventory->setSpacing(4);

for (int i = 0; i < 12; ++i) {
    // new UIButton(...) with index i, callback, etc.
    // inventory->addElement(btn);
}
```

### Anchor layout (HUDs)

`addElement` takes **`(UIElement*, Anchor)`** only. There are **no** extra `(ox, oy)` parameters; use a full-screen layout and `setScreenSize`, or adjust the layout’s position/size for margins.

```cpp
constexpr int SW = 320;
constexpr int SH = 240;

auto* hud = new graphics::ui::UIAnchorLayout(
    math::toScalar(0), math::toScalar(0), SW, SH);
hud->setFixedPosition(true);
hud->setScreenSize(SW, SH);

auto* score = new graphics::ui::UILabel(
    "Score: 0",
    math::Vector2(math::toScalar(0), math::toScalar(0)),
    graphics::Color::White,
    1);
hud->addElement(score, graphics::ui::Anchor::TOP_LEFT);
```

Enum literals use **`Anchor::TOP_LEFT`**, `TOP_RIGHT`, `BOTTOM_RIGHT`, etc.

---

## Container patterns

### Padding container

`UIPaddingContainer` wraps **one** child (`setChild`) and uses a normal **bounding box** `(x, y, w, h)`.

```cpp
auto* padded = new graphics::ui::UIPaddingContainer(
    math::toScalar(0), math::toScalar(0), 120, 40);
// padded->setPadding(...);
// padded->setChild(button);
```

### Panel (visual containers)

`UIPanel` also wraps **one** child. Nest a `UILayout` for multiple rows.

```cpp
auto* dialog = new graphics::ui::UIPanel(
    math::toScalar(50), math::toScalar(50), 140, 100);
dialog->setBackgroundColor(graphics::Color::Black);
dialog->setBorderColor(graphics::Color::White);

auto* layout = new graphics::ui::UIVerticalLayout(
    math::toScalar(0), math::toScalar(0), 130, 90);
// layout->addElement(...);
dialog->setChild(layout);
```

---

## Navigation

### D-pad and `handleInput`

`Scene::update` does **not** call `UILayout::handleInput` for you. Call it from your scene when you want list/grid/toolbar navigation:

```cpp
void MyScene::update(unsigned long dt) {
    if (mainMenu) {
        mainMenu->handleInput(engine.getInputManager());
    }
    Scene::update(dt);
}
```

### Selection helpers

```cpp
layout->setSelectedIndex(2);
int idx = layout->getSelectedIndex();
graphics::ui::UIElement* sel = layout->getSelectedElement();  // not getSelectedChild
```

---

## ESP32 performance

### Viewport culling

`UIVerticalLayout` and `UIGridLayout` skip drawing children outside the visible viewport when scrolling is relevant. Still prefer **fewer nodes** and **less frequent text changes**.

### Scrolling

Use **`setScrollEnabled`** / **`setScrollingEnabled`** (see `UIVerticalLayout` and `UILayout`) and **`setScrollOffset`** where applicable. There is **no** `UIScrollMode` enum in the current API.

---

## Sprite and graphics

### Sprite descriptors

Wrap bitmaps in descriptors before rendering:

```cpp
pixelroot32::graphics::Sprite playerSprite = {
    .width = 16,
    .height = 16,
    .data = playerBitmap
};

renderer.drawSprite(playerSprite, x, y, paletteSlot, flipX);
```

### Multi-sprite (layered)

Compose multi-color sprites from 1bpp layers:

```cpp
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

### Bit depth guidelines

| Format | Use for | Memory cost |
|--------|---------|-------------|
| 1bpp | Game sprites, tiles | 1x |
| 2bpp | Logos, detailed UI | 2x |
| 4bpp | Photos, rich graphics | 4x |

**Default to 1bpp** for gameplay-critical sprites.

---

## What actors should not do

```cpp
// Wrong: per-pixel loops in actors
void MyActor::draw(Renderer& r) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (bitmap[y] & (1 << x)) {
                r.drawPixel(x, y, color);
            }
        }
    }
}

// Correct: batch drawing via renderer
void MyActor::draw(Renderer& r) {
    r.drawSprite(mySprite, position.x, position.y);
}
```

---

## Touch vs classic widgets

- **`UITouchButton` / `UITouchCheckbox` / `UITouchSlider`**: register with `UIManager::addElement` and implement `Scene::processTouchEvents`. Also `addEntity` so widgets draw.
- **`UIButton` / `UICheckBox`**: use **`handleInput`** and focus/selection; they do not use the touch event dispatcher for hit testing.

See [architecture/touch-input.md](../architecture/touch-input.md).

---

## Related documentation

| Document | Topic |
|----------|-------|
| [graphics-guidelines.md](./graphics-guidelines.md) | Sprites, tilemaps, palettes |
| [coding-style.md](./coding-style.md) | C++ conventions |
| [performance/esp32-performance.md](./performance/esp32-performance.md) | Optimization |
| [api/ui.md](../api/ui.md) | UI API reference |

---

UIs should be responsive, efficient, and consistent with the real `UIElement` / `UITouchElement` split.
