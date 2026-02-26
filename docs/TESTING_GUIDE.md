# Testing Guide - PixelRoot32 Game Engine

**Document Version:** 1.1  
**Last Updated:** February 2026  
**Engine Version:** v0.9.0-dev  

## Overview

This comprehensive guide covers testing practices for the PixelRoot32 Game Engine. It includes unit testing, integration testing, platform-specific testing, and continuous integration setup. The test suite uses the **Unity** framework and runs on the **native** platform by default (`native_test`).

**Recent updates (v1.1):** Document structure aligned with the current test tree: full list of unit test suites under `test/unit/`, correct include paths (`../../test_config.h`, `../../mocks/` from unit tests), coverage scripts split into `coverage_win.py` and `coverage_linux.py` (with `--report` and `--no-tests`), and PlatformIO details (default env, `test_ignore`, coverage build flags).

## Quick Start

### Running Tests

```bash
# Run all tests on native platform (default env)
pio test -e native_test

# Run tests with verbose output
pio test -e native_test --verbose

# Run a specific test suite (e.g. only test_physics_actor)
pio test -e native_test -f test_physics_actor

# Run tests with coverage report (Windows)
python scripts/coverage_win.py --report

# Run tests with coverage report (Linux)
python scripts/coverage_linux.py --report

# Generate coverage without re-running tests
python scripts/coverage_win.py --no-tests --report
python scripts/coverage_linux.py --no-tests --report
```

### Platform-Specific Testing

```bash
# ESP32 tests (requires hardware)
pio test -e esp32dev

# ESP32-S3 tests
pio test -e esp32s3

# Native tests (PC/Mac/Linux)
pio test -e native_test
```

---

## Test Structure

### Directory Organization

Each test suite lives in its own folder under `test/unit/<suite_name>/` with one or more `.cpp` files. Integration and game-loop tests live at the top level of `test/`. PlatformIO compiles each unit test folder separately (unit test sources are excluded from the main build via `build_src_filter`).

```
test/
├── test_config.h                    # Shared test utilities and macros
├── unit/                            # Unit tests (one folder per suite)
│   ├── test_actor/                  # Actor entity
│   ├── test_audio_command_queue/    # Audio command queue
│   ├── test_audio_engine/           # Audio engine
│   ├── test_audio_scheduler/        # Audio scheduler
│   ├── test_camera2d/               # 2D camera
│   ├── test_collision_primitives/   # Collision primitives (AABB, circle, etc.)
│   ├── test_collision_system/       # Collision system
│   ├── test_collision_types/        # Collision types
│   ├── test_color/                  # Color utilities
│   ├── test_entity/                 # Entity base
│   ├── test_font_manager/           # Font manager
│   ├── test_graphics/               # Graphics (e.g. particles)
│   ├── test_graphics_ownership/     # Graphics ownership
│   ├── test_input_config/           # Input configuration
│   ├── test_input_manager/          # Input manager
│   ├── test_kinematic_actor/        # Kinematic actor
│   ├── test_math/                   # Math utilities (MathUtil, Scalar, etc.)
│   ├── test_music_player/           # Music player
│   ├── test_physics_actor/          # PhysicsActor (body type, bounds, bounce)
│   ├── test_physics_expansion/      # Physics expansion
│   ├── test_rect/                   # Rect type
│   ├── test_scene/                  # Scene
│   ├── test_scene_manager/           # Scene manager
│   ├── test_ui/                     # UI elements and layouts
│   └── ...
├── test_engine_integration/         # Engine integration tests
├── test_game_loop/                  # Game loop / end-to-end tests
└── mocks/                           # Mock implementations
    ├── MockAudioBackend.h
    ├── MockAudioScheduler.h
    ├── MockDisplay.h
    ├── MockDrawSurface.h
    └── MockRenderer.h
```

**PlatformIO configuration (relevant):**

- **Default env:** `native_test` (`[platformio] default_envs = native_test`).
- **Test framework:** Unity.
- **Ignored tests:** `test_embedded` is excluded via `test_ignore` (embedded-only tests).
- **Coverage:** Build uses `--coverage` and `-lgcov`; scripts are `scripts/coverage_win.py` (Windows) and `scripts/coverage_linux.py` (Linux). The previous single `coverage_check.py` has been replaced by these two platform-specific scripts.

### Test File Naming

- **Folder:** `test/unit/test_<module>/` (e.g. `test_physics_actor/`, `test_ui/`).
- **Source file(s):** Typically `test_<module>.cpp` or `test_<module>_<topic>.cpp` (e.g. `test_physics_actor.cpp`, `test_ui_elements.cpp`, `test_ui_layouts.cpp`).

```cpp
// Function naming: test_<module>_<function>_<scenario>
test_mathutil_lerp_basic
test_physics_actor_set_velocity_float
test_physics_actor_resolve_left_boundary
test_ui_button_click
test_audio_engine_play_event
```

---

## Writing Unit Tests

### Basic Test Structure

Tests use Unity and the shared `test_config.h`, which provides float comparison helpers (`float_eq`, `TEST_ASSERT_FLOAT_EQUAL`), `test_setup()`/`test_teardown()`, and common data (`test_data::PI`, `test_data::SCREEN_WIDTH`, etc.). From a file under `test/unit/<suite>/`, include the config as `../../test_config.h` (or `../test_config.h` from `test_engine_integration/`).

```cpp
#include <unity.h>
#include "module/Header.h"
#include "../../test_config.h"

using namespace pixelroot32::module;

// Setup function - runs before each test
void setUp(void) {
    test_setup(); // Initialize test environment
}

// Teardown function - runs after each test
void tearDown(void) {
    test_teardown(); // Cleanup test environment
}

// Test function naming: test_<module>_<function>_<scenario>
void test_mathutil_lerp_basic(void) {
    // Arrange
    Scalar a = toScalar(0.0f);
    Scalar b = toScalar(10.0f);
    Scalar t = toScalar(0.5f);
    
    // Act
    Scalar result = lerp(a, b, t);
    
    // Assert
    TEST_ASSERT_EQUAL_FLOAT(5.0f, toFloat(result));
    // Or use test_config.h helper: TEST_ASSERT_FLOAT_EQUAL(5.0f, toFloat(result));
}

// Main function - test runner
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_mathutil_lerp_basic);
    return UNITY_END();
}
```

### Testing with Mocks

```cpp
#include <unity.h>
#include "audio/AudioEngine.h"
#include "../../mocks/MockAudioBackend.h"
#include "../../test_config.h"

using namespace pixelroot32::audio;

void test_audio_engine_play_event(void) {
    // Arrange
    AudioConfig config;
    MockAudioBackend backend;
    AudioEngine engine(config);
    
    AudioEvent event = {
        WaveType::PULSE,
        440.0f,  // A4
        0.5f,    // Volume
        0.1f     // Duration
    };
    
    // Act
    engine.playEvent(event);
    
    // Assert
    TEST_ASSERT_EQUAL(1, backend.getEventCount());
    TEST_ASSERT_EQUAL_FLOAT(440.0f, backend.getLastEvent().frequency);
}
```

---

## Integration Testing

### Engine Integration Test

```cpp
#include <unity.h>
#include "core/Engine.h"
#include "../mocks/MockDrawSurface.h"
#include "../test_config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

void test_engine_scene_lifecycle(void) {
    // Arrange
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Engine engine(config);
    
    auto scene = std::make_unique<MockScene>();
    
    // Act & Assert
    engine.setScene(scene.get());
    TEST_ASSERT_TRUE(engine.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL_PTR(scene.get(), engine.getCurrentScene().value());
    
    // Test scene transition
    auto newScene = std::make_unique<MockScene>();
    engine.setScene(newScene.get());
    TEST_ASSERT_EQUAL_PTR(newScene.get(), engine.getCurrentScene().value());
}
```

### Game Loop Test

```cpp
#include <unity.h>
#include "core/Engine.h"
#include "../test_config.h"

void test_game_loop_timing(void) {
    // Arrange
    Engine engine;
    MockScene scene;
    engine.setScene(&scene);
    
    // Act - simulate 60 frames
    for (int i = 0; i < 60; i++) {
        engine.update();
    }
    
    // Assert
    TEST_ASSERT_EQUAL(60, scene.getUpdateCount());
    TEST_ASSERT_EQUAL(60, scene.getDrawCount());
    
    // Verify timing consistency
    TEST_ASSERT_UINT32_WITHIN(100, 1000, scene.getTotalTime()); // ~1 second ±100ms
}
```

---

## Platform-Specific Testing

### ESP32 Hardware Testing

```cpp
#ifdef ESP32
#include <unity.h>
#include "esp32_specific_test.h"

void test_esp32_audio_dac(void) {
    // Only runs on actual ESP32 hardware
    TEST_ASSERT_TRUE(ESP.getChipModel() == CHIP_ESP32);
    
    AudioConfig config;
    ESP32_DAC_AudioBackend dac(config);
    
    TEST_ASSERT_TRUE(dac.init());
    TEST_ASSERT_EQUAL(44100, dac.getSampleRate());
}
#endif
```

### Cross-Platform Compatibility

```cpp
void test_scalar_math_consistency(void) {
    // Test that Scalar produces consistent results across platforms
    Scalar a = toScalar(3.14159f);
    Scalar b = toScalar(2.71828f);
    
    Scalar result = a * b;
    
    // Allow small floating-point differences
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.53973f, toFloat(result));
}
```

---

## Test Coverage

### Coverage Targets

| Metric | Target | Current Status |
|--------|--------|----------------|
| Line Coverage | ≥80% | Track in CI |
| Function Coverage | ≥90% | Track in CI |
| Branch Coverage | ≥70% | Optional |

### Coverage Analysis

Coverage is handled by two platform-specific scripts (the former single `coverage_check.py` script has been removed):

| Platform | Script | Notes |
|----------|--------|--------|
| Windows  | `scripts/coverage_win.py` | Prefers **gcovr** (e.g. `pip install gcovr`), falls back to **lcov**. Excludes `src/drivers/native/` and `include/drivers/native/` from coverage. |
| Linux    | `scripts/coverage_linux.py` | Uses **lcov** / **genhtml**. Same exclusions for drivers and test code. |

**Options (both scripts):**

- `--report` — Generate HTML report in `coverage_report/`.
- `--no-tests` — Skip running tests; only generate/parse coverage from existing build.

```bash
# Generate coverage report (Windows)
python scripts/coverage_win.py --report

# Generate coverage report (Linux)
python scripts/coverage_linux.py --report

# Coverage without re-running tests (e.g. after pio test)
python scripts/coverage_win.py --no-tests --report
python scripts/coverage_linux.py --no-tests --report

# View HTML report (Windows)
start coverage_report/index.html

# View HTML report (Linux)
xdg-open coverage_report/index.html

# Check specific file coverage (gcov)
gcov -f src/math/MathUtil.cpp
```

### Coverage Example

```cpp
// Test edge cases for full coverage
void test_physics_collision_edge_cases(void) {
    // Test perfect overlap (circle vs circle)
    CollisionCircle c1({0, 0}, 10);
    CollisionCircle c2({0, 0}, 10);
    TEST_ASSERT_TRUE(c1.intersects(c2));
    
    // Test barely touching
    CollisionCircle c3({0, 0}, 10);
    CollisionCircle c4({19.9f, 0}, 10);
    TEST_ASSERT_TRUE(c3.intersects(c4));
    
    // Test barely not touching
    CollisionCircle c5({0, 0}, 10);
    CollisionCircle c6({20.1f, 0}, 10);
    TEST_ASSERT_FALSE(c5.intersects(c6));
}
```

---

## Continuous Integration

### GitHub Actions Workflow

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test-native:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
      - run: pip install platformio
      - run: pio test -e native_test
      - run: python scripts/coverage_linux.py

  test-esp32:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
      - run: pip install platformio
      - run: pio test -e esp32dev
```

### Local CI Simulation

```bash
# Run all native unit and integration tests
pio test -e native_test

# Run with verbose output to see failures clearly
pio test -e native_test --verbose

# Run coverage check and generate HTML report (Windows)
python scripts/coverage_win.py --report

# Run coverage check and generate HTML report (Linux)
python scripts/coverage_linux.py --report
```

---

## Performance Testing

### Memory Usage Testing

```cpp
void test_memory_usage_stability(void) {
    size_t initialHeap = ESP.getFreeHeap();
    
    {
        // Create and destroy many objects
        std::vector<std::unique_ptr<Actor>> actors;
        for (int i = 0; i < 100; i++) {
            actors.push_back(std::make_unique<Actor>(0, 0, 32, 32));
        }
        actors.clear();
    }
    
    size_t finalHeap = ESP.getFreeHeap();
    
    // Should return to approximately initial state
    TEST_ASSERT_UINT32_WITHIN(100, initialHeap, finalHeap);
}
```

### Frame Rate Testing

```cpp
void test_performance_60fps(void) {
    Engine engine;
    PerformanceScene scene;
    engine.setScene(&scene);
    
    auto start = millis();
    
    // Run for 1 second
    while (millis() - start < 1000) {
        engine.update();
        engine.draw();
    }
    
    TEST_ASSERT_GREATER_OR_EQUAL(60, scene.getFrameCount());
}
```

---

## Debugging Failed Tests

### Common Issues

1. **Memory Leaks**
   ```cpp
   void test_no_memory_leak(void) {
       size_t heapBefore = ESP.getFreeHeap();
       
       // Code that might leak
       createAndDestroyObjects();
       
       size_t heapAfter = ESP.getFreeHeap();
       TEST_ASSERT_EQUAL(heapBefore, heapAfter);
   }
   ```

2. **Timing Issues**
   ```cpp
   void test_timing_sensitive(void) {
       // Use consistent timing
       unsigned long fixedDelta = 16; // 60 FPS
       
       actor.update(fixedDelta);
       TEST_ASSERT_EQUAL(expectedPosition, actor.position.x);
   }
   ```

3. **Platform Differences**
   ```cpp
   void test_platform_consistency(void) {
       #ifdef ESP32
       // ESP32-specific test
       TEST_ASSERT_TRUE(ESP.getChipModel() == CHIP_ESP32);
       #else
       // Native platform test
       TEST_ASSERT_TRUE(sizeof(void*) == 8); // 64-bit
       #endif
   }
   ```

### Debug Output

```cpp
void test_with_debug_output(void) {
    Actor actor(0, 0, 32, 32);
    
    Serial.print("Initial position: ");
    Serial.println(actor.position.x);
    
    actor.update(16);
    
    Serial.print("Final position: ");
    Serial.println(actor.position.x);
    
    // This will help identify the issue
    TEST_ASSERT_EQUAL(16, actor.position.x);
}
```

---

## Testing Best Practices

### 1. Test Naming
- Be descriptive: `test_physics_gravity_acceleration()`
- Include edge cases: `test_collision_circle_perfect_overlap()`
- Group related tests in same file

### 2. Test Independence
- Each test should be independent
- Use `setUp()` and `tearDown()` for consistent state
- Avoid relying on test execution order

### 3. Test Coverage
- Test happy path and error cases
- Include boundary conditions
- Test platform-specific behavior when relevant

### 4. Performance
- Keep individual tests fast (<100ms)
- Use mocks for slow external dependencies
- Consider test suite execution time

### 5. Maintainability
- Write clear, readable test code
- Document complex test scenarios
- Update tests when code changes

---

## Resources

### Documentation
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Google Test Primer](https://github.com/google/googletest/blob/main/docs/primer.md) (concepts apply)

### Tools
- **gcov**: Code coverage analysis
- **Valgrind**: Memory debugging (native)
- **ESP32 Exception Decoder**: Crash analysis
- **PlatformIO Test Explorer**: VS Code extension

### Examples
- See `test/unit/` for all unit test suites (e.g. `test_physics_actor/`, `test_ui/`, `test_math/`).
- See `test/test_engine_integration/` and `test/test_game_loop/` for integration and game-loop tests.
- See `test/test_config.h` for shared macros and helpers (`TEST_ASSERT_FLOAT_EQUAL`, `test_data`, `test_setup`/`test_teardown`).
- Review `scripts/coverage_win.py` and `scripts/coverage_linux.py` for coverage automation (no longer a single `coverage_check.py`).

---

**Remember**: Good tests catch bugs early, document expected behavior, and give confidence to refactor. Write tests that you'd want to read when debugging at 2 AM!