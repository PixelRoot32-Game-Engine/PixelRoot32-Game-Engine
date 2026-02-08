# Unit Testing - PixelRoot32 Game Engine

This directory contains the unit testing suite for the PixelRoot32 game engine.

## Directory Structure

```text
test/
├── test_config.h                 # Shared configuration and utilities
├── unit/                         # Unit tests organized by module
│   ├── test_math/               # Math tests
│   ├── test_physics/            # Physics tests (Collision, Actor, etc.)
│   ├── test_core/               # Core tests (Entity, Scene, etc.)
│   ├── test_graphics/           # Graphics tests (Color, Camera, Font)
│   ├── test_ui/                 # UI tests (Elements, Layouts)
│   ├── test_input/              # Input tests
│   └── test_audio/              # Audio tests
├── test_engine_integration/      # Engine integration tests
└── test_game_loop/               # Full flow tests (End-to-End)
```

## Running Tests

### Requirements

- [PlatformIO](https://platformio.org/) installed
- (Optional) lcov for coverage report generation

### Commands

```bash
# Run all tests
pio test -e native_test

# Run with verbose output
pio test -e native_test --verbose

# Run with code coverage
python scripts/coverage_check.py

# Generate HTML coverage report
python scripts/coverage_check.py --report
```

### Helper Scripts

#### Linux/Mac

```bash
# Run tests
./scripts/run_tests.sh

# Run with coverage
./scripts/run_tests.sh --coverage
```

#### Windows

```batch
REM Run tests
scripts\run_tests.bat

REM Run with coverage
scripts\run_tests.bat --coverage
```

### Test Types

1. **Unit Tests (`unit/`)**: Validate individual components in isolation.
    - `test_ui`: Validates interface elements (Label, Button, Checkbox) and layout systems (Vertical, Horizontal, Grid, Anchor).
2. **Integration Tests (`test_engine_integration/`)**: Verify that systems (Renderer, SceneManager, Input) work together correctly.
3. **End-to-End Tests (`test_game_loop/`)**: Simulate a full game lifecycle, including state updates, user input, and rendering.

## Mocks and Simulation

To test components that depend on hardware or complex systems, mocks located in `test/mocks/` are used:

- **MockDrawSurface**: Captures drawing calls (lines, rectangles, text) to validate visual rendering without needing a real screen.
- **MockAudioBackend**: Simulates the audio backend to validate sample generation and channel states.
- **MockAudioScheduler**: Allows testing the audio command queue in isolation.
- **MockEngineInstance**: Provides a global engine instance (`engine`) necessary for components using the singleton pattern or global access (like the particle system).

## Testing Conventions

### Test Naming

Tests follow the convention:

```cpp
void test_<module>_<function>_<scenario>()
```

Examples:

- `test_mathutil_lerp_basic()`
- `test_rect_intersects_overlapping()`
- `test_color_black()`

### Test File Structure

```cpp
#include <unity.h>
#include "module/Header.h"
#include "../test_config.h"

using namespace pixelroot32::module;

void setUp(void) {
    // Initialization before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_module_function_scenario(void) {
    // Arrange
    // Act
    // Assert
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_module_function_scenario);
    return UNITY_END();
}
```

## Code Coverage

### Objectives

- **Lines**: Minimum 80%
- **Functions**: Minimum 90%

### Covered Modules (Phase 1)

| Module | Files | Tests | Coverage |
|--------|-------|-------|----------|
| math/MathUtil | MathUtil.h | test_mathutil.cpp | 100% |
| core/Rect | Entity.h | test_rect.cpp | 100% |
| physics/CollisionTypes | CollisionTypes.h | test_collision_types.cpp | 100% |
| graphics/Color | Color.h | test_color.cpp | 100% |

## Testing Framework

We use [Unity](https://github.com/ThrowTheSwitch/Unity) integrated with PlatformIO.

### Available Asserts

- `TEST_ASSERT_EQUAL(expected, actual)`
- `TEST_ASSERT_EQUAL_FLOAT(expected, actual)`
- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_FALSE(condition)`
- `TEST_ASSERT_NULL(pointer)`
- `TEST_ASSERT_NOT_NULL(pointer)`

And many more. See [Unity documentation](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md).

## Contributing

When adding new tests:

1. Create a file in the corresponding directory under `test/unit/`
2. Follow the naming convention
3. Include all edge cases
4. Document with Doxygen comments if necessary
5. Verify tests pass: `pio test -e native_test`
6. Verify coverage: `python scripts/coverage_check.py`



## Resources

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [Contribution Guide](../CONTRIBUTING.md)
