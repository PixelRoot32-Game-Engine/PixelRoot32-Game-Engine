cmake_minimum_required(VERSION 3.20)

# Packaging acceptance test: takes the *installed* package, builds a real game
# against it and produces the desktop artifact of this platform (a `.app` on
# macOS, an AppImage on Linux). The engine is found through
# `CMAKE_PREFIX_PATH` exactly as in tools/cmake_consumer — nothing here
# includes a header or a source from the engine's tree.
#
#   cmake -S tools/desktop_package -B build/package \
#         -DCMAKE_PREFIX_PATH=<install prefix>
#   cmake --build build/package                    # the game (and the .app)
#   cmake --build build/package --target pr32_sprites_demo_appimage   # Linux
#
# The game is `examples/sprites`, the example the PlatformIO matrix and the
# CMake workflow already build, so the packaged artifact is a real game rather
# than a packaging stub.

project(pr32_desktop_package LANGUAGES CXX)

find_package(PixelRoot32 1.16 CONFIG REQUIRED)

get_filename_component(_pr32_default_sprites
    "${CMAKE_CURRENT_LIST_DIR}/../../examples/sprites" ABSOLUTE)
set(PR32_SPRITES_DIR "${_pr32_default_sprites}" CACHE PATH
    "Directory of the sprites example whose sources are packaged")

if(NOT EXISTS "${PR32_SPRITES_DIR}/src/main.cpp")
    message(FATAL_ERROR
        "PR32_SPRITES_DIR='${PR32_SPRITES_DIR}' does not contain src/main.cpp")
endif()

# `SpritesDemoScene.h` exists only when one of the sprite bit-depth features is
# on, and those are build-time features of the engine, not switches a game can
# flip: the library is compiled with them and the exported interface hands the
# same macros to every translation unit of the game. Building against an engine
# configured without them fails deep inside the example (`spritesdemo` is not a
# namespace then), so it is reported here with the configure line instead.
get_target_property(_pr32_defs PixelRoot32::Engine INTERFACE_COMPILE_DEFINITIONS)
if(NOT _pr32_defs)
    set(_pr32_defs "")
endif()
list(FIND _pr32_defs "PIXELROOT32_ENABLE_2BPP_SPRITES=1" _pr32_2bpp)
list(FIND _pr32_defs "PIXELROOT32_ENABLE_4BPP_SPRITES=1" _pr32_4bpp)
if(_pr32_2bpp EQUAL -1 AND _pr32_4bpp EQUAL -1)
    message(FATAL_ERROR
        "The installed engine was not built with the sprite bit-depth features "
        "examples/sprites needs.\n"
        "Configure the engine with the example's own profile first:\n"
        "  -DPIXELROOT32_ENABLE_2BPP_SPRITES=ON -DPIXELROOT32_ENABLE_4BPP_SPRITES=ON\n"
        "  -DPIXELROOT32_ENABLE_AUDIO=OFF -DPIXELROOT32_ENABLE_PHYSICS=OFF\n"
        "  -DPIXELROOT32_ENABLE_UI_SYSTEM=OFF -DPIXELROOT32_ENABLE_PARTICLES=OFF")
endif()

pixelroot32_add_game(pr32_sprites_demo
    "${PR32_SPRITES_DIR}/src/main.cpp"
    "${PR32_SPRITES_DIR}/src/SpritesDemoScene.cpp")

# The example's own headers (`platforms/native.h`, `assets/Sprites.h`).
target_include_directories(pr32_sprites_demo PRIVATE "${PR32_SPRITES_DIR}/src")

# The two defines `examples/sprites/platformio.ini` passes for `env:native`. The
# rest of its flags are the engine's: PLATFORM_NATIVE and SDL_MAIN_HANDLED come
# from PixelRoot32::Engine's exported interface.
target_compile_definitions(pr32_sprites_demo PRIVATE
    PHYSICAL_DISPLAY_WIDTH=240
    PHYSICAL_DISPLAY_HEIGHT=240)

# The engine's own logo, so the artifact is identifiable in a launcher. Without
# an icon appimagetool reports a missing .DirIcon (and still finishes), which
# reads like a failure in a green build for no good reason.
get_filename_component(_pr32_default_icon
    "${CMAKE_CURRENT_LIST_DIR}/../../assets/pr32_logo.png" ABSOLUTE)
set(PR32_PACKAGE_ICON "${_pr32_default_icon}" CACHE FILEPATH
    "Icon for the packaged game, PNG on Linux and .icns on macOS")

pixelroot32_add_appimage(pr32_sprites_demo
    DISPLAY_NAME "PixelRoot32 Sprites"
    ICON "${PR32_PACKAGE_ICON}")
