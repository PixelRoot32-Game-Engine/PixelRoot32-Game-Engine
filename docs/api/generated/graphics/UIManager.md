# UIManager

<Badge type="info" text="Class" />

**Source:** `UIManager.h`

## Description

Registry of touch UI elements for event routing (non-owning pointers).

The scene (or another owner) constructs widgets and registers them with addElement.
clear/removeElement only unregister pointers — they never destroy objects.

Lifetime Contract

UIManager holds NON-OWNING pointers to UITouchElement instances. The widget
lifetime is managed exclusively by the caller (Scene, game code, arena allocator).

Ownership Rules
- UIManager does NOT delete widgets
- Widgets must call removeElement() BEFORE being destroyed
- Failure to unregister results in dangling pointers and potential crashes

Safe Destruction Sequence
// CORRECTO: Desregistrar antes de destruir
uiManager.removeElement(myButton.get());
myButton.reset();  // or delete myButton;

// INCORRECTO: Destruir sin desregistrar
myButton.reset();  // Widget destruido
// UIManager::capturedWidget o hoverWidget ahora son dangling!
Captured Widget Safety
UIManager automatically clears capturedWidget when removeElement() is called.
However, if a widget is deleted directly without removeElement(), the caller
MUST call uiManager.releaseCapture() to avoid use-after-free.

## Methods

### `bool addElement(UITouchElement* element)`

**Description:**

Register an element for touch hit-testing and processEvents.

**Parameters:**

- `element`: Non-null; must outlive registration (or until remove/clear).

**Returns:** false if full, duplicate pointer, or element is null

### `bool removeElement(uint8_t id)`

### `bool removeElement(UITouchWidget* widget)`

### `UITouchElement* getElement(uint8_t id) const`

### `UITouchElement* getElementAt(uint8_t index) const`

### `uint8_t getElementCount() const`

### `uint8_t getMaxElements() const`

### `bool isFull() const`

### `void clear()`

### `UITouchWidget* getActiveWidget() const`

### `UITouchWidget* getHoverWidget() const`

### `UITouchElement** getElements()`

### `UITouchElement* const* getElements() const`

### `void updateHover(int16_t x, int16_t y)`

### `void clearConsumeFlags()`

### `UITouchWidget* getCapturedWidget() const`

### `void releaseCapture()`

### `void update(unsigned long deltaTime)`

### `int8_t findFreeSlot() const`
