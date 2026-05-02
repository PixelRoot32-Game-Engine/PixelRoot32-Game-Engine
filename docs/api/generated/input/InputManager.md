# InputManager

<Badge type="info" text="Class" />

**Source:** `InputManager.h`

## Description

Handles input from physical buttons, keyboard (on PC), and touch/mouse.

The InputManager polls configured pins, handles debouncing, and tracks button states
(Pressed, Released, Down, Clicked). It also provides touch event processing for
both ESP32 (via TouchManager) and Native (via mouse-to-touch mapping).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `constexpr` | `static` | Maximum number of buttons supported. |

## Methods

### `void init()`

**Description:**

Initializes the input pins.

### `void update(unsigned long dt, const uint8_t* keyboardState)`

**Description:**

Updates input state based on SDL keyboard state.

**Parameters:**

- `dt`: Delta time.
- `keyboardState`: Pointer to the SDL keyboard state array.

### `void update(unsigned long dt)`

**Description:**

Updates input state by polling hardware pins.

**Parameters:**

- `dt`: Delta time.

### `bool isButtonPressed(uint8_t buttonIndex) const`

**Description:**

Checks if a button was just pressed this frame.

**Parameters:**

- `buttonIndex`: Index of the button to check.

**Returns:** true if the button transitioned from UP to DOWN this frame.

### `bool isButtonReleased(uint8_t buttonIndex) const`

**Description:**

Checks if a button was just released this frame.

**Parameters:**

- `buttonIndex`: Index of the button to check.

**Returns:** true if the button transitioned from DOWN to UP this frame.

### `bool isButtonClicked(uint8_t buttonIndex) const`

**Description:**

Checks if a button was clicked (pressed and released).

**Parameters:**

- `buttonIndex`: Index of the button to check.

**Returns:** true if the button was clicked.

### `bool isButtonDown(uint8_t buttonIndex) const`

**Description:**

Checks if a button is currently held down.

**Parameters:**

- `buttonIndex`: Index of the button to check.

**Returns:** true if the button is currently in the DOWN state.

### `uint8_t getTouchEvents(TouchEvent* buffer, uint8_t maxCount)`

**Description:**

Get touch events from the event dispatcher.

**Parameters:**

- `buffer`: Caller-provided buffer for events.
- `maxCount`: Maximum number of events to retrieve.

**Returns:** Number of events retrieved (removed from queue).

### `bool hasTouchEvents() const`

**Description:**

Check if there are pending touch events.

**Returns:** true if there are events in the queue.

### `TouchState getTouchState(uint8_t touchId) const`

**Description:**

Get the current state of a specific touch ID.

**Parameters:**

- `touchId`: Touch identifier (0-4).

**Returns:** Current touch state.

### `void processSDLEvent(const void* sdlEvent)`

**Description:**

Process an SDL event (mouse/keyboard).

**Parameters:**

- `sdlEvent`: The SDL event to process.

This method handles:
- SDL_MOUSEBUTTONDOWN/UP: Maps to touch events
- SDL_MOUSEMOTION: Maps to drag events when button is held
- SDL_KEYDOWN/SDL_KEYUP: Maps to button events (existing)

### `void injectTouchPoint(int16_t x, int16_t y, bool pressed, uint8_t id, uint32_t timestamp)`

**Description:**

Inject a raw touch point from external source (e.g., TouchManager).

**Parameters:**

- `point`: The touch point to inject.
- `timestamp`: Current timestamp in ms.

Used by ESP32 examples to connect TouchManager with InputManager.
