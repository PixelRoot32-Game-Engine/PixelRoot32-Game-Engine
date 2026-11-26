# Desktop packaging

Packages `examples/sprites` — the example the PlatformIO matrix builds — using
the **installed** PixelRoot32 package, and produces the desktop artifact of this
platform:

| host | output | what makes it that |
|---|---|---|
| macOS | `build/package/pr32_sprites_demo.app` | `MACOSX_BUNDLE` + a generated `Info.plist` |
| Linux | `build/package/pr32_sprites_demo.AppImage` | an AppDir with `AppRun` and the SDL2 runtime, squashed by `appimagetool` |
| Windows | not built here | see [packaging docs](../../docs/guide/packaging.md#windows-there-is-no-installer-yet) |

The engine is resolved through `CMAKE_PREFIX_PATH`, exactly like
[`tools/cmake_consumer`](../cmake_consumer/): this project compiles no header
and no source from the engine's tree, only the game's own.

The flags in step 1 are the ones `examples/sprites/lib/platformio.ini` passes,
spelled as CMake options. They are not this project's to choose: the engine's
features are compiled into the library, and the exported interface hands the
same macros to the game, so both halves have to agree. Configuring the engine
without 2BPP/4BPP makes this project stop with that configure line instead of
failing inside the example.

```bash
# 1. Build and install the engine, with the flags the example itself sets
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$PWD/build/prefix" \
      -DPIXELROOT32_ENABLE_2BPP_SPRITES=ON \
      -DPIXELROOT32_ENABLE_4BPP_SPRITES=ON \
      -DPIXELROOT32_ENABLE_AUDIO=OFF \
      -DPIXELROOT32_ENABLE_PHYSICS=OFF \
      -DPIXELROOT32_ENABLE_UI_SYSTEM=OFF \
      -DPIXELROOT32_ENABLE_PARTICLES=OFF
cmake --build build/cmake --parallel
cmake --install build/cmake

# 2. Build the game (and the .app on macOS)
cmake -S tools/desktop_package -B build/package \
      -DCMAKE_PREFIX_PATH="$PWD/build/prefix"
cmake --build build/package --parallel

# 3. On Linux, squash the AppDir into an AppImage
cmake --build build/package --target pr32_sprites_demo_appimage
```

`appimagetool` is not vendored; the target looks for it in `PATH` and accepts
`-DPIXELROOT32_APPIMAGETOOL=/path/to/appimagetool` instead. The AppDir is staged
in `build/package/AppDir/pr32_sprites_demo` either way, which is the useful half
when inspecting the layout.

The icon is the engine's logo (`assets/pr32_logo.png`), overridable with
`-DPR32_PACKAGE_ICON=<path>`. Linux takes the PNG as is; a macOS bundle would
need an `.icns`, which is why this project passes no icon there.

The two calls the engine adds are `pixelroot32_add_game()` and
`pixelroot32_add_appimage()`, both from `cmake/PixelRoot32Game.cmake` inside the
installed package. A game project uses the same two, and nothing else changes:
the bundle keys, the `AppRun`, the `.desktop` file and the SDL2 runtime copy are
the engine's job, not every game's.
