# API Reference: UI Module

> **Source of truth:**
> - `include/graphics/ui/UIElement.h`, `UIButton.h`, `UILabel.h`, `UIPanel.h`, `UICheckbox.h`
> - `include/graphics/ui/UILayout.h`, `UIGridLayout.h`, `UIVerticalLayout.h`, `UIHorizontalLayout.h`, `UIAnchorLayout.h`, `UIPaddingContainer.h`
> - `include/graphics/ui/UITouchWidget.h`, `UITouchElement.h`, `UITouchButton.h`, `UITouchSlider.h`, `UITouchCheckbox.h`
> - `include/graphics/ui/UIManager.h`, `UIHitTest.h`

## Overview

*(Requires `PIXELROOT32_ENABLE_UI_SYSTEM=1`)*

The UI module provides a hierarchical, object-oriented user interface system. It is designed to be lightweight, resolution-independent, and fully integrated with the engine's rendering and input systems.

## Core Elements

### UIElement

The abstract base class for all UI nodes. It provides the core hierarchical structure (parent/children relationships), bounds management (`x`, `y`, `width`, `height`), rendering phases (`draw`, `drawChildren`), and visibility toggles.

### Standard Widgets

These are primarily non-interactive or controller-driven visual elements.

- **`UIButton`**: A simple button that renders differently based on its state (Normal, Hover, Pressed, Disabled) and displays text.
- **`UILabel`**: Displays a single line or multiline string of text using a specified `Font`.
- **`UIPanel`**: A generic container with a background color and an optional border. Great for grouping other elements.
- **`UICheckbox`**: A simple binary state widget (checked/unchecked).

## Layouts

Layouts are specialized `UIElement`s that automatically position and size their children according to specific rules. 

### UILayout (Base) & ScrollBehavior

`UILayout` extends `UIElement` to add children management, spacing, and scrolling logic.

**Scroll Behaviors:**
- `NO_SCROLL`: Content is clipped.
- `AUTO_SCROLL`: Shows scrollbars only when content exceeds bounds.
- `ALWAYS_SCROLL`: Always displays scrollbars.

### Available Layouts

- **`UIVerticalLayout`**: Stacks children vertically.
- **`UIHorizontalLayout`**: Stacks children horizontally.
- **`UIGridLayout`**: Arranges children in a 2D grid with defined cell sizes.
- **`UIPaddingContainer`**: A container that adds fixed padding around a single child element.
- **`UIAnchorLayout`**: Positions children based on anchor points relative to the parent's bounds.
  - **Anchors**: `TOP_LEFT`, `TOP_CENTER`, `TOP_RIGHT`, `CENTER_LEFT`, `CENTER`, `CENTER_RIGHT`, `BOTTOM_LEFT`, `BOTTOM_CENTER`, `BOTTOM_RIGHT`.

## Touch Widgets Architecture

*(Requires `PIXELROOT32_ENABLE_TOUCH=1`)*

The touch UI system builds on top of the standard UI system but introduces complex state machines for handling direct screen interactions (Press, Drag, Release, Click).

- **`UITouchWidget`**: The base interface for any UI component that wants to receive raw touch events.
- **`UITouchElement`**: Extends `UIElement` and implements `UITouchWidget`, bridging the visual hierarchy with the touch event system.
- **`UIHitTest`**: A utility class used by the `UIManager` to perform deep hit-testing across the UI tree, respecting `clipToBounds` flags.

### UITouchButton

A button fully driven by touch events.

**Example:**
```cpp
auto touchBtn = std::make_shared<pixelroot32::graphics::ui::UITouchButton>();
touchBtn->setBounds(10, 10, 100, 30);
touchBtn->setText("Tap Me!");
touchBtn->setOnClick([](UITouchButton* btn) {
    PR32_LOG_INFO("Button Tapped!");
});
```

### UITouchSlider

A horizontal or vertical slider for selecting a numeric value. Handles dragging and bounds-checking.

**Example:**
```cpp
auto slider = std::make_shared<pixelroot32::graphics::ui::UITouchSlider>();
slider->setBounds(10, 50, 150, 20);
slider->setRange(0.0f, 100.0f);
slider->setValue(50.0f);
slider->setOnValueChanged([](UITouchSlider* s, float val) {
    PR32_LOG_INFO("Volume: %f", val);
});
```

### UITouchCheckbox

A touch-driven toggle switch.

**Example:**
```cpp
auto checkbox = std::make_shared<pixelroot32::graphics::ui::UITouchCheckbox>();
checkbox->setBounds(10, 90, 20, 20);
checkbox->setChecked(true);
checkbox->setOnToggled([](UITouchCheckbox* cb, bool state) {
    PR32_LOG_INFO("Music: %s", state ? "ON" : "OFF");
});
```

## UIManager

The `UIManager` is the root of the UI tree for a `Scene`. It manages the top-level element, dispatches touch events via `processTouchEvents()`, and orchestrates rendering.

**Example:**
```cpp
void MyMenuScene::init() {
    auto& ui = getUIManager();
    
    // Create root panel
    auto panel = std::make_shared<pixelroot32::graphics::ui::UIPanel>();
    panel->setBounds(0, 0, 240, 240);
    panel->setBackgroundColor(Color::PR32_BLUE);
    
    // Add child
    auto label = std::make_shared<pixelroot32::graphics::ui::UILabel>();
    label->setBounds(10, 10, 100, 20);
    label->setText("Main Menu");
    panel->addChild(label);
    
    // Set as root
    ui.setRoot(panel);
}
```

## Related Documentation

- [API Reference](index.md) - Main index
- [Input Module](input.md) - Touch event generation
- [Graphics Module](graphics.md) - Renderer and Color