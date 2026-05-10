# Migration Guide: v1.4.0 → v1.5.0

## InputConfig API Simplification

Version 1.5.0 simplifies the `InputConfig` constructor by eliminating the redundant `count` parameter. The number of inputs is now automatically deduced from the template arguments, making the API cleaner and less error-prone.

---

## Breaking Change: `count` Parameter Removed

### What Changed

The `InputConfig` constructor no longer requires the explicit `count` argument. The number of inputs is automatically deduced from the template variadic arguments.

### Before

```cpp
pr32::input::InputConfig inputConfig(
    6,              // count - now auto-deduced
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B
);
```

### After

```cpp
pr32::input::InputConfig inputConfig(
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B
);
```

---

## Memory Optimization: `std::vector` → `std::array`

### Why This Change

Version 1.5.0 replaces `std::vector` with `std::array` in `InputConfig`:

| Aspect | Before (`std::vector`) | After (`std::array`) |
|--------|------------------------|----------------------|
| Heap allocation | Yes (dynamic) | No (stack/global) |
| Fragmentation | Possible | None |
| Determinism | Non-deterministic | O(1) access |
| Max inputs | Dynamic (runtime) | Fixed (16) |
| Zero-init | Required manually | Automatic via `{}` |

### Memory Impact

| Metric | Before | After |
|--------|--------|-------|
| `InputConfig` size | ~24 bytes (heap) | 72 bytes (stack, fixed) |
| ESP32 heap usage | Variable | 0 bytes |
| Allocation at runtime | Yes | None |

---

## API Changes

### Constructor Signature

**Before:**

```cpp
InputConfig(int count, ...);  // variadic with explicit count
```

**After:**

```cpp
template<typename... Args>
InputConfig(Args... args);  // count auto-deduced
```

### Empty Configuration

**Before:**

```cpp
InputConfig config(0);  // empty with count
```

**After:**

```cpp
InputConfig config{};  // default constructor or empty init list
InputConfig config();  // also valid
```

---

## Migration Patterns

### Pattern 1: Remove the Count Parameter

```cpp
// Before
pr32::input::InputConfig config(4, PIN_1, PIN_2, PIN_3, PIN_4);

// After
pr32::input::InputConfig config(PIN_1, PIN_2, PIN_3, PIN_4);
```

### Pattern 2: Empty Configuration

```cpp
// Before
pr32::input::InputConfig config(0);

// After
pr32::input::InputConfig config{};
// or
pr32::input::InputConfig config;
```

### Pattern 3: Single Input

```cpp
// Before
pr32::input::InputConfig config(1, BTN_UP);

// After
pr32::input::InputConfig config(BTN_UP);
```

### Pattern 4: Native Platform (SDL2)

```cpp
// Before
pr32::input::InputConfig config(2, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);

// After
pr32::input::InputConfig config(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
```

---

## Compile-Time Validation

The new implementation includes compile-time validation:

### Overflow Detection

```cpp
// If more than 16 arguments are passed, compilation fails:
InputConfig config(1, 2, 3, ..., 17);  // static_assert error at compile time
```

### Error Message

```
error: static_assert failed due to requirement 'sizeof...(Args) <= MAX_INPUT_COUNT'
"Too many arguments for InputConfig"
```

---

## Runtime Validation

Invalid configurations are handled at runtime:

| Scenario | Behavior |
|----------|----------|
| Empty config | `count = 0`, arrays zero-initialized |
| 1-16 inputs | `count = N`, valid |
| >16 inputs | `count = 16` (clamped) |

---

## Migration Checklist

- [ ] **Search for `InputConfig(` calls**: Find all usages with explicit count
- [ ] **Remove the first integer argument**: The count is now automatic
- [ ] **Verify count deduction**: Ensure the new count matches expected
- [ ] **Check empty configurations**: Replace `InputConfig(0)` with `InputConfig{}`
- [ ] **Build and test**: Ensure all scenes compile and run correctly

### Search Patterns

```bash
# Find all InputConfig usages
grep -rn "InputConfig(" --include="*.cpp" --include="*.h"

# Specific pattern for explicit count (regex)
InputConfig\([0-9]+,
```

---

## Backward Compatibility

**This is a breaking change.** The old constructor signature `InputConfig(int count, ...)` no longer works.

If you have code like this:

```cpp
InputConfig config(3, A, B, C);  // OLD API - WILL NOT COMPILE
```

You must update to:

```cpp
InputConfig config(A, B, C);  // NEW API
```

---

## Verification

After migration, verify:

- [ ] All `InputConfig` calls compile without errors
- [ ] Input count matches the number of arguments provided
- [ ] ESP32 builds succeed with no heap allocation warnings
- [ ] Native (SDL2) builds work correctly
- [ ] All unit tests pass (`pio test -e native_test`)

---

## File Changes

This change affects the following files:

| File | Change |
|------|--------|
| `include/input/InputConfig.h` | `std::vector` → `std::array`, template variadic constructor |
| `src/core/Engine.cpp` | Updated `InputConfig(0)` → `InputConfig{}` |
| All game scenes | Remove `count` parameter from `InputConfig` calls |

---

## References

- [Input System Guide](../guide/input.md)
- [Memory Optimization](../guide/memory.md)
- [ESP32 Performance](../guide/performance/esp32-performance.md)
- [API Reference: Input](../api/input.md)