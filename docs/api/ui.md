# API Reference: UI Module

This document covers the user interface system, touch widgets, and layout containers in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

> **Note**: The UI system is only available if `PIXELROOT32_ENABLE_UI_SYSTEM=1`

---

## UIElement

**Include:** `graphics/ui/UIElement.h`

**Inherits:** [Entity](API_CORE.md#entity)

Base class for all UI elements. Provides positioning, sizing, and layout capabilities.

### Properties

- **`UIElementType type`**: The type of UI element (GENERIC, BUTTON, LABEL, CHECKBOX, LAYOUT).
- **`bool fixedPosition`**: If true, the element ignores camera scroll and stays fixed on screen.

### Public Methods

- **`UIElement(Vector2 position, int w, int h, UIElementType t)`**
    Constructs a new UIElement.

- **`UIElement(Scalar x, Scalar y, int w, int h, UIElementType t)`**
    Constructs a new UIElement with scalar coordinates.

- **`UIElementType getType() const`**
    Returns the type of UI element.

- **`virtual bool isFocusable() const`**
    Returns true if the element can receive focus for navigation.

- **`void setFixedPosition(bool fixed)`**
    Sets whether the element ignores camera scroll (true = fixed HUD position).

- **`bool isFixedPosition() const`**
    Returns true if the element is in fixed position mode.

- **`virtual void setPosition(Scalar newX, Scalar newY)`**
    Sets the position of the element.

- **`virtual void getPreferredSize(Scalar& preferredWidth, Scalar& preferredHeight) const`**
    Gets the preferred size of the element for layout calculations.

---

## UIButton

**Include:** `graphics/ui/UIButton.h`

**Inherits:** [UIElement](#uielement)

A clickable button UI element with support for physical buttons and D-pad navigation.

### Public Methods

- **`UIButton(std::string_view t, uint8_t index, Vector2 position, Vector2 size, UIElementVoidCallback callback, TextAlignment textAlign = CENTER, int fontSize = 2)`**
    Constructs a new UIButton. `UIElementVoidCallback` is `void(*)()` (function pointer, not `std::function`).

- **`void setStyle(Color textCol, Color bgCol, bool drawBg)`**
    Configures the button's visual style.

- **`void setSelected(bool selected)`**
    Sets the selection state (for D-pad navigation).

- **`bool getSelected() const`**
    Returns true if the button is selected.

- **`bool isFocusable() const override`**
    Returns true (buttons are always focusable).

- **`void handleInput(const InputManager& input)`**
    Handles input events (touch or button press).

- **`void press()`**
    Manually triggers the button's callback.

---

## UILabel

**Include:** `graphics/ui/UILabel.h`

**Inherits:** [UIElement](#uielement)

A simple text label for displaying information.

### Public Methods

- **`UILabel(std::string_view t, Vector2 position, Color col, uint8_t sz)`**
    Constructs a new UILabel.

- **`void setText(std::string_view t)`**
    Updates the label's text and recalculates dimensions.

- **`void setVisible(bool v)`**
    Sets visibility.

- **`void centerX(int screenWidth)`**
    Centers the label horizontally.

---

## UIPanel

**Include:** `graphics/ui/UIPanel.h`

**Inherits:** [UIElement](#uielement)

A visual container that draws a background and border around child elements. Useful for dialogs and menus.

### Public Methods

- **`UIPanel(Scalar x, Scalar y, int w, int h)`**
    Constructs a new UIPanel.

- **`UIPanel(Vector2 position, int w, int h)`**
    Constructs a new UIPanel with vector position.

- **`void setChild(UIElement* element)`**
    Sets the child element to display inside the panel.

- **`UIElement* getChild() const`**
    Gets the child element.

- **`void setBackgroundColor(Color color)`**
    Sets the background color.

- **`void setBorderColor(Color color)`**
    Sets the border color.

- **`void setBorderWidth(uint8_t width)`**
    Sets the border width in pixels.

- **`uint8_t getBorderWidth() const`**
    Gets the border width.

- **`void setPosition(Scalar newX, Scalar newY) override`**
    Sets position and updates child position.

---

## UILayout

**Include:** `graphics/ui/UILayout.h`

**Inherits:** [UIElement](#uielement)

Base class for UI layout containers. Provides automatic element positioning, spacing, and optional scrolling.

### Types

- **`enum class ScrollBehavior`**
  - `NONE`: No scrolling allowed
  - `SCROLL`: Scroll freely within bounds
  - `CLAMP`: Scroll but clamp to content bounds

### Public Methods

- **`UILayout(Scalar x, Scalar y, int w, int h)`**
    Constructs a new UILayout.

- **`UILayout(Vector2 position, int w, int h)`**
    Constructs a new UILayout with vector position.

- **`virtual void addElement(UIElement* element)`**
    Adds a UI element to the layout.

- **`virtual void removeElement(UIElement* element)`**
    Removes a UI element from the layout.

- **`virtual void updateLayout()`**
    Recalculates positions of all elements.

- **`virtual void handleInput(const InputManager& input)`**
    Handles input for navigation/scrolling.

- **`void setPadding(Scalar p)`**
    Sets the internal padding.

- **`Scalar getPadding() const`**
    Gets the current padding.

- **`void setSpacing(Scalar s)`**
    Sets the spacing between elements.

- **`Scalar getSpacing() const`**
    Gets the current spacing.

- **`size_t getElementCount() const`**
    Gets the number of elements in the layout.

- **`UIElement* getElement(size_t index) const`**
    Gets the element at a specific index.

- **`void clearElements()`**
    Removes all elements from the layout.

- **`void setScrollingEnabled(bool enabled)`**
    Enables or disables scrolling.

- **`bool isScrollingEnabled() const`**
    Returns true if scrolling is enabled.

---

## UIGridLayout

**Include:** `graphics/ui/UIGridLayout.h`

**Inherits:** [UILayout](#uilayout)

Grid layout container that organizes elements in a matrix with 4-direction navigation support.

### Public Methods

- **`UIGridLayout(Scalar x, Scalar y, int w, int h)`**
    Constructs a new UIGridLayout.

- **`UIGridLayout(Vector2 position, int w, int h)`**
    Constructs a new UIGridLayout with vector position.

- **`void addElement(UIElement* element)`**
    Adds a UI element to the grid.

- **`void removeElement(UIElement* element)`**
    Removes a UI element from the grid.

- **`void updateLayout()`**
    Recalculates positions of all elements.

- **`void handleInput(const InputManager& input)`**
    Handles navigation input (UP/DOWN/LEFT/RIGHT).

- **`void setColumns(uint8_t cols)`**
    Sets the number of columns.

- **`uint8_t getColumns() const`**
    Gets the number of columns.

- **`uint8_t getRows() const`**
    Gets the calculated number of rows.

- **`int getSelectedIndex() const`**
    Gets the currently selected element index.

- **`void setSelectedIndex(int index)`**
    Sets the selected element by index.

- **`UIElement* getSelectedElement() const`**
    Gets the currently selected element.

- **`void setNavigationButtons(uint8_t upButton, uint8_t downButton, uint8_t leftButton, uint8_t rightButton)`**
    Configures button indices for navigation.

- **`void setButtonStyle(Color selectedTextCol, Color selectedBgCol, Color unselectedTextCol, Color unselectedBgCol)`**
    Sets style colors for selected/unselected states.

---

## UICheckBox

**Include:** `graphics/ui/UICheckbox.h`

**Inherits:** [UIElement](#uielement)

A checkbox UI element with support for physical buttons and D-pad navigation.

### Public Methods

- **`UICheckBox(std::string_view label, uint8_t index, Vector2 position, Vector2 size, bool checked = false, UIElementBoolCallback callback = nullptr, int fontSize = 2)`**
    Constructs a new UICheckBox. `UIElementBoolCallback` is `void(*)(bool)` (function pointer, not `std::function`).

- **`void setStyle(Color textCol, Color bgCol, bool drawBg = false)`**
    Configures the checkbox's visual style.

- **`void setChecked(bool checked)`**
    Sets the checked state.

- **`bool isChecked() const`**
    Returns true if the checkbox is checked.

- **`void setSelected(bool selected)`**
    Sets the selection state (for D-pad navigation).

- **`bool getSelected() const`**
    Returns true if the checkbox is selected.

- **`bool isFocusable() const override`**
    Returns true (checkboxes are always focusable).

- **`void handleInput(const InputManager& input)`**
    Handles input events.

- **`void toggle()`**
    Toggles the checkbox state.

---

## UIVerticalLayout

**Inherits:** [UILayout](#uilayout)

Vertical layout container with scroll support. Organizes UI elements vertically, one below another.

### Public Methods

- **`UIVerticalLayout(float x, float y, float w, float h)`**
    Constructs a new UIVerticalLayout.

- **`void setScrollEnabled(bool enable)`**
    Enables or disables scrolling.

- **`float getScrollOffset() const`**
    Gets the current scroll offset in pixels.

- **`void setScrollOffset(float offset)`**
    Sets the scroll offset directly.

- **`float getContentHeight() const`**
    Gets the total content height.

- **`int getSelectedIndex() const`**
    Gets the currently selected element index.

- **`void setSelectedIndex(int index)`**
    Sets the selected element index.

- **`void setScrollSpeed(float speed)`**
    Sets the scroll speed for smooth scrolling.

- **`void setNavigationButtons(uint8_t upButton, uint8_t downButton)`**
    Sets the navigation button indices.

- **`void setButtonStyle(...)`**
    Sets style colors for selected/unselected buttons.

---

## UIHorizontalLayout

**Inherits:** [UILayout](#uilayout)

Horizontal layout container with scroll support. Organizes UI elements horizontally, one next to another.

### Public Methods

- **`UIHorizontalLayout(float x, float y, float w, float h)`**
    Constructs a new UIHorizontalLayout.

- **`void setScrollEnabled(bool enable)`**
    Enables or disables scrolling.

- **`float getScrollOffset() const`**
    Gets the current scroll offset.

- **`float getContentWidth() const`**
    Gets the total content width.

- **`int getSelectedIndex() const`**
    Gets the currently selected element index.

- **`void setSelectedIndex(int index)`**
    Sets the selected element index.

- **`void setNavigationButtons(uint8_t leftButton, uint8_t rightButton)`**
    Sets navigation button indices.

- **`void setButtonStyle(...)`**
    Sets style colors.

---

## UIAnchorLayout

**Inherits:** [UILayout](#uilayout)

Layout that positions elements at fixed anchor points on the screen without reflow. Very efficient for HUDs.

### Public Methods

- **`UIAnchorLayout(float x, float y, float w, float h)`**
    Constructs a new UIAnchorLayout.

- **`void addElement(UIElement* element, Anchor anchor)`**
    Adds a UI element with a specific anchor point.

- **`void removeElement(UIElement* element)`**
    Removes a UI element.

- **`void setScreenSize(float screenWidth, float screenHeight)`**
    Sets the screen size for anchor calculations.

### Anchor Values

- `TOP_LEFT`, `TOP_RIGHT`, `BOTTOM_LEFT`, `BOTTOM_RIGHT`
- `CENTER`, `TOP_CENTER`, `BOTTOM_CENTER`, `LEFT_CENTER`, `RIGHT_CENTER`

---

## UIPaddingContainer

**Inherits:** [UIElement](#uielement)

Container that wraps a single UI element and applies padding.

### Public Methods

- **`UIPaddingContainer(float x, float y, float w, float h)`**
    Constructs a new UIPaddingContainer.

- **`void setChild(UIElement* element)`**
    Sets the child element to wrap.

- **`UIElement* getChild() const`**
    Gets the child element.

- **`void setPadding(float p)`**
    Sets uniform padding on all sides.

- **`void setPadding(float left, float right, float top, float bottom)`**
    Sets asymmetric padding.

---

## Touch Widgets

The touch widget system provides optimized UI elements for touchscreen input. It uses a memory-efficient pool pattern where `UITouchWidget` structs are allocated from a fixed pool, and `UITouchElement` classes wrap them to provide Entity interface.

**Architecture:**

```
UITouchWidget (struct) → UITouchElement (class: UIElement) → UITouchButton/UITouchSlider
```

> **Note:** UITouchElement inherits from UIElement (not Entity), enabling integration with UILayout containers.

---

## UITouchWidget

**Include:** `graphics/ui/UITouchWidget.h`

Lightweight struct stored in a fixed-size pool. Contains position, size, state, and flags.

### Properties

- **`UIWidgetType type`**: Type of touch widget (Button, Slider)
- **`int16_t x, y`**: Position
- **`uint16_t width, height`**: Dimensions
- **`UIWidgetState state`**: Current state (Idle, Pressed, Dragging, Hover)
- **`UIWidgetFlags flags`**: Enabled, Visible, Active flags

### Public Methods

- **`UITouchWidget(UIWidgetType t, uint8_t index, int16_t x, int16_t y, uint16_t w, uint16_t h)`**
    Constructs a new touch widget.

- **`void setEnabled(bool enabled)`**
    Enables or disables the widget.

- **`bool isEnabled() const`**
    Returns true if enabled.

- **`void setVisible(bool visible)`**
    Sets visibility.

- **`bool isVisible() const`**
    Returns true if visible.

- **`void setPosition(int16_t x, int16_t y)`**
    Sets position.

- **`void setSize(uint16_t w, uint16_t h)`**
    Sets size.

- **`bool contains(int16_t px, int16_t py) const`**
    Checks if point is within bounds.

---

## UITouchElement

**Include:** `graphics/ui/UITouchElement.h`

**Inherits:** [UIElement](#uielement)

Abstract base class for touch-optimized UI elements. Contains an embedded **`UITouchWidget`**.

### Public Methods

- **`UITouchElement(int16_t x, int16_t y, uint16_t w, uint16_t h, UIWidgetType type)`**
    Constructs a touch element.

- **`virtual bool processEvent(const TouchEvent& event) = 0`**
    Implemented by subclasses. Returns `true` when the event should be consumed.

- **`void update(unsigned long deltaTime) override`**
    Base implementation is empty; subclasses may override.

- **`void draw(Renderer& renderer) override`**
    Base implementation is empty; concrete widgets override.

- **`void setPosition(Scalar newX, Scalar newY) override`**
    Sets position and synchronizes both Entity position and widget data.

- **`uint8_t getWidgetState() const`**
    Returns the current widget state.

- **`bool isPressed() const`**
    Returns `true` if the element is currently pressed.

- **`bool isEnabled() const`**
    Returns `true` if the element is enabled.

- **`bool isVisible() const`**
    Returns `true` if the element is visible.

- **`UITouchWidget& getWidgetData()`**
    Returns reference to the embedded widget data.

---

## UITouchButton

**Include:** `graphics/ui/UITouchButton.h`

**Inherits:** [UITouchElement](#uitouchelement)

Touch-optimized button with press, release, and click callbacks.

### Public Methods

- **`UITouchButton(std::string_view t, int16_t x, int16_t y, uint16_t w, uint16_t h)`**
    Constructs a touch button.

- **`void setLabel(std::string_view t)`**
    Sets the button label.

- **`std::string_view getLabel() const`**
    Gets the button label.

- **`void setColors(Color normal, Color pressed, Color disabled)`**
    Sets the colors for different states.

- **`ButtonCallback`** — type alias for **`void (*)()`**

- **`void setOnDown(ButtonCallback callback)`** / **`setOnUp`** / **`setOnClick`**
    Sets callbacks for touch down, up, and click.

- **`bool processEvent(const TouchEvent& event) override`**
    Handles hit testing and state.

### Example

```cpp
void onPressed() { /* ... */ }

auto btn = std::make_unique<UITouchButton>("Press Me", 50, 100, 120, 40);
getUIManager().addElement(btn.get());
btn->setColors(Color::White, Color::Cyan, Color::Gray);
btn->setOnClick(onPressed);
```

---

## UITouchSlider

**Include:** `graphics/ui/UITouchSlider.h`

**Inherits:** [UITouchElement](#uitouchelement)

Touch-optimized slider with draggable thumb. Supports horizontal orientation with value range 0-100.

### Public Methods

- **`UITouchSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue = 50)`**
    Constructs a touch slider.

- **`uint8_t getValue() const`**
    Gets the current value (0-100).

- **`void setValue(uint8_t value)`**
    Sets the value (clamped to 0-100).

- **`void setColors(Color track, Color thumb)`**
    Sets the track and thumb colors.

- **`SliderCallback`** — type alias **`void (*)(uint8_t)`**

- **`void setOnValueChanged(SliderCallback callback)`**
    Called when the value changes.

- **`void setOnDragStart(SliderCallback callback)`** / **`setOnDragEnd`**
    Optional drag lifecycle hooks.

- **`bool processEvent(const TouchEvent& event) override`**
    Handles drag and hit testing.

### Example

```cpp
void onVol(uint8_t v) { volume = v / 100.0f; }

auto slider = std::make_unique<UITouchSlider>(50, 150, 200, 30, 75);
getUIManager().addElement(slider.get());
slider->setColors(Color::Gray, Color::White);
slider->setOnValueChanged(onVol);
```

---

## UITouchCheckbox

**Include:** `graphics/ui/UITouchCheckbox.h`

**Inherits:** [UITouchElement](#uitouchelement)

Touch-optimized checkbox widget.

### Public Methods

- **`UITouchCheckbox(std::string_view label, int16_t x, int16_t y, uint16_t w, uint16_t h, bool initialChecked = false)`**
    Constructs a touch checkbox.

- **`void setChecked(bool checked)`**
    Sets the checked state.

- **`bool isChecked() const`**
    Gets the current checked state.

- **`void toggle()`**
    Toggles the checked state.

- **`void setLabel(std::string_view label)`**
    Sets the checkbox label.

- **`std::string_view getLabel() const`**
    Gets the current label.

- **`void setColors(Color normal, Color checked, Color disabled)`**
    Sets checkbox colors.

- **`CheckboxCallback`** — type alias **`void (*)(bool)`**

- **`void setOnChanged(CheckboxCallback callback)`**
    Sets callback for state changes.

- **`bool processEvent(const TouchEvent& event) override`**
    Handles hit testing and toggle semantics.

- **`void draw(Renderer& renderer) override`**
    Renders the checkbox.

### Example

```cpp
void onSound(bool enabled) { soundEnabled = enabled; }

auto checkbox = std::make_unique<UITouchCheckbox>("Enable Sound", 50, 200, 150, 30, true);
getUIManager().addElement(checkbox.get());
checkbox->setColors(Color::White, Color::Cyan, Color::Gray);
checkbox->setOnChanged(onSound);
```

---

## UIManager

**Include:** `graphics/ui/UIManager.h`

**Inherits:** None

**Non-owning registry** for touch UI elements. Holds up to **`MAX_ELEMENTS`** (16) pointers for hit testing and event dispatch.

> **⚠️ Lifetime Contract:** Widgets **MUST** call `removeElement()` before being destroyed.

### Constants

- **`MAX_ELEMENTS`**: Maximum number of registered elements (16).

### Public Methods

- **`UIManager()`**
    Constructs an empty manager.

- **`~UIManager()`**
    Calls **`clear()`** (unregisters only).

- **`bool addElement(UITouchElement* element)`**
    Registers element for hit testing and events.

- **`bool removeElement(uint8_t id)`** / **`bool removeElement(UITouchWidget* widget)`**
    Unregisters element. Does not destroy the object.

- **`UITouchElement* getElement(uint8_t id) const`**
    Returns registered element for that widget id.

- **`uint8_t getElementCount() const`**
    Number of registered elements.

- **`void clear()`**
    Clears all registrations.

- **`uint8_t processEvents(TouchEvent* events, uint8_t count)`**
    For each event: hit test, then `hit->processEvent(event)`.

- **`bool processEvent(TouchEvent& event)`**
    Convenience wrapper around `processEvents(&event, 1)`.

- **`void releaseCapture()`**
    Clears captured widget.

- **`void update(unsigned long deltaTime)`** / **`void draw(Renderer& renderer)`**
    **No-op** (deprecated). Touch widgets are updated/drawn through Scene.

### Usage

```cpp
void MyScene::initUI() {
    auto& ui = getUIManager();
    ui.clear();

    okButton = std::make_unique<UITouchButton>("OK", 10, 20, 100, 40);
    ui.addElement(okButton.get());
    okButton->setOnClick(onOkClicked);

    // Optional: add to layout and register as entity
    barLayout = std::make_unique<UIHorizontalLayout>(/* ... */);
    barLayout->addElement(okButton.get());
    addEntity(barLayout.get());
}
```

---

## UIHitTest

**Include:** `graphics/ui/UIHitTest.h`

Helper class for hit testing UITouchElement entities.

### Public Methods

- **`static bool hitTest(const UITouchElement& element, int16_t x, int16_t y)`**
    Returns true if the point is within the element's bounds.

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Entity, Scene
- [API Graphics](API_GRAPHICS.md) - Rendering system
- [API Input](API_INPUT.md) - Touch input system