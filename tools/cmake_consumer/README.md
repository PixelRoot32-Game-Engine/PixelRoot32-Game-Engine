# CMake consumer

A project that uses PixelRoot32 the way a game would after the engine has been
installed somewhere: `find_package(PixelRoot32)` and a link line. It shares no
path with the engine's source tree — that is the point.

```bash
# 1. Build and install the engine into a prefix of your choice
cmake -S . -B build/cmake -DCMAKE_INSTALL_PREFIX="$PWD/build/prefix"
cmake --build build/cmake
cmake --install build/cmake

# 2. Build and run this project against that prefix
cmake -S tools/cmake_consumer -B build/consumer \
      -DCMAKE_PREFIX_PATH="$PWD/build/prefix"
cmake --build build/consumer
./build/consumer/pr32_cmake_consumer
```

It prints the package version, the display geometry, the feature flags the
library was built with and the SDL2 it linked. It never calls `Engine::init()`,
so it needs no window and runs headless — which is why CI can run it.

## What it checks

- The headers resolve from the install prefix, not from this repository.
- SDL2 arrives through `PixelRoot32::Engine`; this project never calls
  `find_package(SDL2)`.
- The exported compile definitions arrive too: `src/main.cpp` fails to compile
  if `PLATFORM_NATIVE` is missing, and the printed flags are the library's.
- The version file matches: `find_package(PixelRoot32 1.16 …)` succeeds and the
  printed version comes from `${PixelRoot32_VERSION}`.

`.github/workflows/cmake.yml` runs all of the above on Linux and macOS. See
[Installation via CMake](../../README.md#-installation-via-cmake) for the flags
this build exposes.
