# AGENTS.md - PixelRoot32 Game Engine

## Project Context

PixelRoot32 is a lightweight, modular 2D game engine written in C++17 for ESP32 microcontrollers with PC simulation via SDL2. The engine follows a scene-based architecture inspired by Godot Engine.

## Development Environment Tips

- **PlatformIO** is the primary build system for ESP32 development
- **Native builds** use CMake/SDL2 for PC simulation
- Use **conditional compilation flags** (`PIXELROOT32_ENABLE_*`) to exclude unused subsystems
- All source code is in `src/`, headers in `include/`
- Documentation is in `docs/`

## Code Style

- Follow the C++17 standard
- Use existing conventions in `docs/STYLE_GUIDE.md`
- Prefer `std::string_view` over `const std::string&` for string parameters
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) for memory management
- Follow the modular architecture: each subsystem can work independently
- Add `IRAM_ATTR` to performance-critical functions for ESP32

## Testing Instructions

- Run tests with PlatformIO: `pio test`
- For native builds: compile and run test executables
- Check `docs/TESTING_GUIDE.md` for detailed testing guidelines
- Always run lint/typecheck before committing

## Build Commands

| Command | Purpose |
|---------|---------|
| `pio run -e esp32dev` | Build for ESP32 |
| `pio run -e native` | Build for PC (native) |
| `pio test` | Run tests |
| `pio device monitor` | Serial monitor for ESP32 |

## Key Architectural Patterns

- **Bridge Pattern**: `DrawSurface` for rendering abstraction
- **Strategy Pattern**: `AudioScheduler` for audio backends
- **Scene-Entity System**: Godot-inspired hierarchy (Engine → SceneManager → Scene → Entity → Actor)
- **Actor Types**: StaticActor, KinematicActor, RigidActor, SensorActor

## Important Flags

- `PLATFORM_ESP32` / `PLATFORM_NATIVE`: Target platform
- `PIXELROOT32_DEBUG_MODE`: Enable debug logging
- `PIXELROOT32_ENABLE_AUDIO`: Audio subsystem
- `PIXELROOT32_ENABLE_PHYSICS`: Physics system
- `PIXELROOT32_ENABLE_2BPP_SPRITES` / `PIXELROOT32_ENABLE_4BPP_SPRITES`: Sprite formats

## PR Instructions

- Title format: `[<type>] <description>` (e.g., `[feature] Add new audio channel`)
- Run build and tests before committing
- Update documentation if needed
- Follow the migration guides (`docs/MIGRATION_*.md`) for breaking changes
