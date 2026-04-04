# Migration Guide: v1.1.0 → v1.2.0

## Physics Actor Flags Packing

Version 1.2.0 introduces memory optimization for `PhysicsActor` by packing multiple boolean flags into a single byte, reducing memory footprint and improving cache efficiency.

---

## Flag Packing Overview

The `PhysicsActor` class now packs 3 boolean flags (`sensor`, `oneWay`, `bounce`) into a single `uint8_t` field called `physicsFlags`. This saves approximately 4 bytes per actor due to padding elimination.

### Bit Layout

| Bit | Flag | Default |
|-----|------|---------|
| 0 | `sensor` | `false` |
| 1 | `oneWay` | `false` |
| 2 | `bounce` | `true` |

---

## API Changes

### 1. Bounce Property Migration

The `bounce` property has been converted from a public member to a property-like accessor pair. This maintains backward compatibility while using the packed flags internally.

**Before:**

```cpp
PhysicsActor* actor = createActor(x, y, w, h);
actor->bounce = false;  // Direct member access
if (actor->bounce) { ... }
```

**After:**

```cpp
// Both of these now work identically:
actor->bounce = false;  // Uses bounce(bool) setter
actor->setBounce(false);  // Explicit setter

if (actor->bounce) { ... }  // Uses bounce() getter
if (actor->isBounce()) { ... }  // Explicit getter
```

### 2. Sensor and One-Way Flags

These flags were previously stored as separate `bool` members and are now packed. The API remains unchanged.

**Usage (unchanged):**

```cpp
actor->setSensor(true);    // Still works
actor->isSensor();         // Still works

actor->setOneWay(true);    // Still works
actor->isOneWay();        // Still works
```

---

## UI Button Callback Migration

Version 1.2.0 refactors UI button callbacks to use function pointers instead of `std::function` for memory efficiency on ESP32.

### Rationale

- **Memory savings**: `std::function` can use heap allocation for lambdas with captures (~16 bytes). Function pointers use just 4 bytes.
- **Zero allocation**: Function pointers never allocate memory at runtime.
- **Smaller binary**: No `std::function` template overhead in the final binary.

### API Changes

**Before:**

```cpp
#include <functional>

class MyScene : public Scene {
    void onButtonPressed() { /* ... */ }
    
    void init() override {
        button = new UIButton("Click", 0, pos, size, [this]() {
            onButtonPressed();
        });
    }
};
```

**After:**

```cpp
class MyScene : public Scene {
public:
    static MyScene* sCallbackTarget;
    
    static void onButtonPressedStatic() {
        if (sCallbackTarget) {
            sCallbackTarget->onButtonPressed();
        }
    }
    
    void init() override {
        sCallbackTarget = this;
        button = new UIButton("Click", 0, pos, size, onButtonPressedStatic);
    }
};
```

### Class Changes

| Class | Property | Before | After |
|-------|----------|--------|-------|
| `UIButton` | `onClick` | `std::function<void()>` | `UIElementVoidCallback` (function pointer) |
| `UICheckBox` | `onCheckChanged` | `std::function<void(bool)>` | `UIElementBoolCallback` (function pointer) |
| `UITouchCheckbox` | `onChangedCallback` | `CheckboxCallback` (local type) | `UIElementBoolCallback` (unified) |
| `UITouchButton` | `onClickCallback` | `UIElementVoidCallback` | Already using function pointer (no change) |

### UICheckBox Example

```cpp
class MyScene : public Scene {
public:
    static MyScene* sCallbackTarget;
    
    static void onCheckboxChangedStatic(bool checked) {
        if (sCallbackTarget) {
            sCallbackTarget->onCheckboxChanged(checked);
        }
    }
    
    void onCheckboxChanged(bool checked) { /* ... */ }
    
    void init() override {
        sCallbackTarget = this;
        checkbox = new UICheckBox("Enable Sound", 0, pos, size, false, onCheckboxChangedStatic);
    }
};
```

### UITouchCheckbox Constructor Migration

Version 1.2.0 normalizes the `UITouchCheckbox` constructor to match the API of `UICheckBox` and `UITouchButton`.

**Before:**

```cpp
// Legacy constructor with separate x, y, w, h parameters
UITouchCheckbox* checkbox = new UITouchCheckbox("Label", x, y, w, h, false);
// Callback had to be set separately
checkbox->setOnChanged(myCallback);
```

**After:**

```cpp
// Normalized constructor: label, position, size, checked, callback, fontSize
UITouchCheckbox* checkbox = new UITouchCheckbox(
    "Label",
    pixelroot32::math::Vector2(x, y),
    pixelroot32::math::Vector2(w, h),
    false,                      // initialChecked
    myCallback,                 // UIElementBoolCallback
    2                           // fontSize
);
```

### UITouchCheckbox Example

```cpp
class MyScene : public Scene {
public:
    static MyScene* sCallbackTarget;
    
    static void onTouchCheckboxChangedStatic(bool checked) {
        if (sCallbackTarget) {
            sCallbackTarget->onTouchCheckboxChanged(checked);
        }
    }
    
    void onTouchCheckboxChanged(bool checked) { /* ... */ }
    
    void init() override {
        sCallbackTarget = this;
        checkbox = new UITouchCheckbox(
            "Enable Sound",
            pixelroot32::math::Vector2(10, 10),
            pixelroot32::math::Vector2(100, 20),
            false,
            onTouchCheckboxChangedStatic,
            2
        );
    }
};
```

### Memory Impact

| Metric | Before | After | Savings |
|--------|--------|-------|---------|
| `UIButton` callback storage | ~16 bytes | 4 bytes | 12 bytes |
| `UICheckBox` callback storage | ~16 bytes | 4 bytes | 12 bytes |
| `UITouchCheckbox` callback storage | ~4 bytes | 4 bytes | 0 bytes (unified type) |
| `UITouchCheckbox` constructor | Legacy (separated params) | Normalized (Vector2) | Consistent API |
| Heap allocation on construction | Possible | None | Variable |

---

## Migration Checklist

- [ ] **Search for direct `bounce` assignments**: Replace `actor->bounce = value;` with `actor->setBounce(value);` or keep the shorthand `actor->bounce = value;` (still works)
- [ ] **Search for `bounce` conditionals**: Replace `if (actor->bounce)` with `if (actor->isBounce())` or keep shorthand (still works)
- [ ] **Verify physics behavior**: Test that bounce physics still work as expected (restitution should be handled by `setRestitution()`)
- [ ] **Update UIButton callbacks**: If using lambdas with `std::function`, convert to static function + target pattern
- [ ] **Update UICheckBox callbacks**: Same as UIButton - convert lambdas to static function + target pattern
- [ ] **Update UITouchCheckbox**: Convert legacy constructor to normalized Vector2 version
- [ ] **Remove unnecessary `<functional>` includes**: Check UICheckbox.h and remove if present
- [ ] **Build and test**: Ensure all scenes compile and run correctly

---

## Code Examples

### Setting Bounce

```cpp
// Option 1: Using property syntax (recommended for backward compatibility)
actor->bounce = false;

// Option 2: Using explicit setter
actor->setBounce(false);

// Option 3: Using physics property (preferred for physics control)
actor->setRestitution(toScalar(0.0f));  // Disable bounce via restitution
```

### Checking Bounce

```cpp
// Option 1: Using property syntax
if (actor->bounce) { ... }

// Option 2: Using explicit getter
if (actor->isBounce()) { ... }
```

---

## Memory Impact

| Metric | Before | After | Savings |
|--------|--------|-------|---------|
| `PhysicsActor` flags | 3 bytes + padding | 1 byte | ~2-4 bytes |
| Per 64 actors | ~256 bytes | ~192 bytes | ~64 bytes |

---

## Backward Compatibility

The migration maintains full backward compatibility:

- `actor->bounce = value` still works (calls setter)
- `if (actor->bounce)` still works (calls getter)
- `setBounce()` and `isBounce()` are available for explicit usage

---

## Verification

After migration, verify:

- [ ] All scenes compile without errors
- [ ] Physics collisions behave the same as before
- [ ] Bounce property works in both syntax forms
- [ ] No regression in FPS (should be slightly improved)

---

## References

- [Physics System Reference](architecture/ARCH_PHYSICS_SUBSYSTEM.md)
- [API Reference](API_REFERENCE.md)
