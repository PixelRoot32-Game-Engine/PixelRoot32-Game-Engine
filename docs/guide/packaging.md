# Packaging a desktop game

PixelRoot32 builds the same game on macOS, Linux and the web; what changes is
the container each platform expects. The installed package ships two helpers so
a game does not have to re-invent them:

| call | what it produces |
|---|---|
| `pixelroot32_add_game(<target> <sources...>)` | the executable linked against `PixelRoot32::Engine`; a `.app` bundle on macOS |
| `pixelroot32_add_appimage(<target>)` | a `<name>_appimage` target that stages an AppDir and squashes it into an AppImage (Linux) |

Both come from `cmake/PixelRoot32Game.cmake` inside the installed package, so
`find_package(PixelRoot32 REQUIRED)` is all a game project needs:

```cmake
find_package(PixelRoot32 REQUIRED)

pixelroot32_add_game(my_game
    src/main.cpp
    src/MyScene.cpp)
target_include_directories(my_game PRIVATE src)

pixelroot32_add_appimage(my_game DISPLAY_NAME "My Game")
```

The build itself is the same one PlatformIO's `native` environment produces, so
the two paths stay one game, not two branches. [`tools/desktop_package`](../../tools/desktop_package/)
does exactly this with `examples/sprites`, which is why the packaged artifact in
CI is a real game.

## macOS: the `.app`

`pixelroot32_add_game()` passes `MACOSX_BUNDLE` to `add_executable()`, so the
target builds as `my_game.app` with `Contents/MacOS/my_game` and a generated
`Info.plist`. The template is `cmake/PixelRoot32Info.plist.in`, shipped with the
package, because CMake's default one omits two keys a Finder-launched game
wants:

- `NSHighResolutionCapable` — draw at native resolution on a Retina display
  instead of being scaled up. CMake has no `MACOSX_BUNDLE_HIGH_RESOLUTION_CAPABLE`
  property; setting one is silently ignored, which is why the bundle carries its
  own template.
- `NSPrincipalClass` = `NSApplication` — behave as a GUI app (activation, focus,
  menu bar owner) rather than a scored CLI binary.

What the bundle reports:

```bash
$ plutil -p my_game.app/Contents/Info.plist
  "CFBundleExecutable" => "my_game"
  "CFBundleIdentifier" => "org.pixelroot32.my_game"
  "CFBundleShortVersionString" => "1.16.0"
  "NSHighResolutionCapable" => true
  "NSPrincipalClass" => "NSApplication"
```

`VERSION` overrides the engine's version. `ICON` takes an **`.icns`** and puts it
in `Contents/Resources` with `CFBundleIconFile`; a PNG is accepted with a
warning, because Finder renders nothing for one and a blank icon reads like a
packaging bug.

Two consequences of how a bundle is launched:

- **The working directory is `/`.** A game that reads files relative to the
  executable's location will not find them from Finder. PixelRoot32's examples
  compile their sprites into C++ headers, which is why they need nothing on disk;
  a game with external data has to resolve it from `CFBundle` paths itself.
- **Nothing is signed.** Locally built bundles run fine; a distributed one needs
  a Developer ID certificate and notarisation, which is a release decision, not
  something a build helper should do behind your back.

Universal binaries are a build-time choice and need no helper:
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

## Linux: the AppImage

`pixelroot32_add_appimage()` adds a target that stages this AppDir and hands it
to `appimagetool`:

```
AppDir/AppRun                                   entry point, sets LD_LIBRARY_PATH
AppDir/my_game.desktop                          what appimagetool reads
AppDir/my_game.png, AppDir/.DirIcon             the icon, when ICON is given
AppDir/usr/bin/my_game                          the game
AppDir/usr/lib/libSDL2-2.0.so.0                 the SDL2 runtime, when shared
AppDir/usr/share/applications/my_game.desktop   what a desktop menu installs
```

```bash
cmake --build build --target my_game_appimage
# -> build/my_game.AppImage
```

Notes that come from doing it:

- Without an icon, `appimagetool` reports a missing `.DirIcon` and carries on,
  which reads like a failure in a green build; pass `ICON` as a PNG and the
  icon sits at the AppDir root (`my_game.png` and `.DirIcon`), where the AppImage
  desktop integration reads it. It is stored there rather than under
  `usr/share/icons/hicolor/<size>/` because the root copy makes no claim about
  the image's size.
- `appimagetool` is not vendored and no distribution packages it. Put it in
  `PATH` or pass `-DPIXELROOT32_APPIMAGETOOL=/path/to/appimagetool`. It is
  itself an AppImage and most runners have no FUSE 2, so the script invokes it
  with `--appimage-extract-and-run`.
- The desktop file has to exist **at the AppDir root**: with a copy only under
  `usr/share/applications`, `appimagetool` aborts with `Desktop file not found`.
  The script writes both.
- SDL2 is bundled when the package resolved the shared build, stored under its
  **SONAME** rather than the name the file has on the build machine — the loader
  asks for `libSDL2-2.0.so.0`. With `SDL2::SDL2-static` there is nothing to ship
  and nothing to look for.
- `ARCH` is taken from `uname -m` unless already set; `appimagetool` needs it on
  non-x86 hosts.

## The engine has to be configured with the game's flags

Feature flags are compiled into the library, and the exported interface hands the
same macros to the game, so the two halves have to agree. A game that uses the
2BPP/4BPP sprite paths needs an engine built with them:

```bash
cmake -S PixelRoot32-Game-Engine -B build/cmake \
      -DPIXELROOT32_ENABLE_2BPP_SPRITES=ON \
      -DPIXELROOT32_ENABLE_4BPP_SPRITES=ON
```

That is the CMake spelling of the `-D` flags in `examples/sprites/lib/platformio.ini`,
and it is also why a package is per-profile: one install with audio and one
without are two different libraries. `tools/desktop_package` checks the exported
macros and stops with this configure line instead of failing inside the example.

## Windows: there is no installer yet

Decided, not forgotten. The desktop CMake path has no verified Windows build:
`native-multiplatform.yml` builds the examples there with PlatformIO, while
`cmake.yml` deliberately covers Linux and macOS only, because the CMake path
would need the SDL2 MinGW tarball (already handled for PlatformIO in its own
workflow) plus a generator choice — MinGW Makefiles, Ninja or Visual Studio.

Shipping an installer for a build nobody has run is a worse artifact than a
documented gap: an `.msi`/`.exe` that fails on a user's machine is harder to
diagnose than "Windows packaging is not ready". What it would take, in order:

1. Add `windows-latest` to the `cmake` matrix with the SDL2 MinGW tarball and
   the generator pinned; get `install_and_consume` green there.
2. Add the `package_*` job for Windows and assert the executable runs.
3. Only then add an installer (`CPack` with NSIS, or a portable `.zip`), and
   decide what it installs — a game's installer and an engine's are different
   products.

Until then, a Windows player builds the game with PlatformIO (`native`) or a
MinGW toolchain and ships the directory; nothing in this repository depends on
an installer existing.

## What CI checks

`.github/workflows/cmake.yml` has one job per artifact: `package_macos` asserts
the bundle layout, the `Info.plist` keys and that the bundle links SDL2, then
runs it for three seconds with `SDL_VIDEODRIVER=dummy`; `package_appimage` does
the same for the AppImage — layout, ELF payload, SDL2 resolved from inside the
AppImage, three seconds of runtime. The liveness check is what separates "the
file exists" from "the game came up": assets are compiled in, so a dummy video
driver is the only thing a headless runner needs.
