# Testing Guide - PixelRoot32 Game Engine

**Document Version:** 1.0  
**Last Updated:** February 2026  
**Engine Version:** v0.9.0-dev  

## Overview

This comprehensive guide covers testing practices for the PixelRoot32 Game Engine. It includes unit testing, integration testing, platform-specific testing, and continuous integration setup.

## Quick Start

### Running Tests

```bash
# Run all tests on native platform
pio test -e native_test

# Run tests with verbose output
pio test -e native_test --verbose

# Run specific test file
pio test -e native_test -f test_mathutil

# Run tests with coverage report
python scripts/coverage_check.py --report
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

```
test/
├── test_config.h                 # Shared test utilities
├── unit/                        # Unit tests by module
│   ├── test_math/              # Math utilities
│   ├── test_physics/           # Physics system
│   ├── test_core/             # Core engine
│   ├── test_graphics/         # Graphics & rendering
│   ├── test_ui/               # User interface
│   ├── test_audio/            # Audio system
│   └── test_input/            # Input handling
├── test_engine_integration/    # Integration tests
├── test_game_loop/            # End-to-end tests
└── mocks/                     # Mock implementations
```

### Test File Naming

```cpp
// Format: test_<module>_<function>_<scenario>.cpp
test_mathutil_basic.cpp
test_physics_collision_circle.cpp
test_ui_button_click.cpp
test_audio_engine_init.cpp
```

---

## Writing Unit Tests

### Basic Test Structure

```cpp
#include <unity.h>
#include "module/Header.h"
#include "../test_config.h"

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
}

// Main function - test runner
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_mathutil_lerp_basic);
    return UNITY_END();
}
```

### Testing with Mocks

```cpp
#include <unity.h>
#include "audio/AudioEngine.h"
#include "../mocks/MockAudioBackend.h"
#include "../test_config.h"

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

```bash
# Generate coverage report
python scripts/coverage_check.py --report

# View HTML report
open coverage_report/index.html

# Check specific file coverage
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
      - run: python scripts/coverage_check.py

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
# Run all tests like CI would
./scripts/run_tests.sh --coverage --strict

# Check for test failures
grep -r "FAIL" test_results/

# Generate coverage badge
python scripts/generate_coverage_badge.py
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
- See `test/unit/` for implementation examples
- Check `examples/` for tested game code
- Review `scripts/coverage_check.py` for automation

---

**Remember**: Good tests catch bugs early, document expected behavior, and give confidence to refactor. Write tests that you'd want to read when debugging at 2 AM!