# PixelRoot32 Unit Tests

## Overview

PixelRoot32 uses the **Unity Test Framework** for unit testing. Tests are organized by module and run on the native (PC) platform using SDL2 mocks.

## Test Structure

```
test/
├── test_config.h              # Test configuration macros
├── mocks/                     # Mock implementations
│   ├── MockArduino.h
│   ├── MockAudioBackend.h
│   └── MockScene.h
├── test_engine_integration/   # Engine integration tests
└── unit/                      # Unit tests by module
    ├── test_audio/
    ├── test_core/
    ├── test_collision_system/
    ├── test_engine/
    ├── test_entity/
    ├── test_graphics/
    ├── test_input/
    ├── test_kinematic_actor/
    ├── test_math/
    ├── test_physics_actor/
    ├── test_physics_shapes/
    ├── test_scene/
    ├── test_tile_collision_builder/
    └── test_ui/
```

## Running Tests

### Run All Tests
```bash
python run_tests.py
```

### Run Specific Test Suite
```bash
python run_tests.py <Suite-Name>
# e.g., python run_tests.py Physics-System
```

### Run with PlatformIO
```bash
pio test -e native_test
```

## Test Coverage

**Current Coverage Metrics:**

| Metric | Coverage | Target |
|--------|----------|--------|
| Lines | 81.4% | ≥80% |
| Functions | 87.6% | ≥80% |
| Branches | 60.2% | ≥60% |

Generate coverage report:
```bash
python scripts/coverage_win.py --report
```

## Test Categories

### Phase 1: Quick Wins
- **Engine Core** (`test_engine/`) - Constructor, getter, time/scene tests
- **CollisionSystem** (`test_collision_system/`) - 39 tests covering entity management, collisions, edge cases
- **AudioScheduler** (`test_audio_scheduler/`) - Error handling, buffer underrun, edge cases

### Phase 2: Critical Areas
- **TileCollisionBuilder** (`test_tile_collision_builder/`) - Tilemap collision generation
- **Engine Integration** (`test_engine_integration/`) - Mock-based game loop tests
- **UI Layouts** (`test_graphics/test_ui.cpp`) - Grid, anchor, horizontal, vertical layouts

### Phase 3: Consolidation
- **AudioBackend Mocking** (`test_audio_backend/`) - Mock SDL2 audio backend tests
- **Physics Branch Coverage** - STATIC/STATIC, KINEMATIC/KINEMATIC collision filtering

## Adding New Tests

1. Create test file in appropriate `test/unit/test_<module>/` directory
2. Include `test_config.h` and Unity headers
3. Define `setUp()` and `tearDown()` functions
4. Add test functions: `void test_<name>(void)`
5. Register in `main()` with `RUN_TEST(test_<name>)`
6. Add to `run_tests.py` test list

## Test Guidelines

- Use `TEST_ASSERT_*` macros from Unity
- Mock external dependencies (SDL2, hardware)
- Keep tests isolated and deterministic
- Name tests descriptively: `test_<module>_<behavior>_<condition>`
