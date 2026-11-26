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

# The same ICON on both calls, which is what a game does: one PNG, and each
# platform turns it into the format it reads (an `.icns` on macOS).
#
# `ICON ""` is not "no icon": CMake drops the empty argument before the helper
# sees it, so the next argument would be read as the icon path. The keyword is
# added only when there is one, which is also how a game should spell it.
set(_pr32_icon_arg "")
if(PR32_PACKAGE_ICON)
    set(_pr32_icon_arg ICON "${PR32_PACKAGE_ICON}")
endif()

pixelroot32_add_game(pr32_sprites_demo
    ${_pr32_icon_arg}
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

# Left empty on purpose: the icon then comes from the installed package, which
# is the path a game that defines no icon takes. Set it to a square PNG to
# check the override (`-DPR32_PACKAGE_ICON=/path/to/icon.png`).
set(PR32_PACKAGE_ICON "" CACHE FILEPATH
    "Icon for the packaged game (square PNG; .icns also accepted on macOS). Empty uses the engine's.")

pixelroot32_add_appimage(pr32_sprites_demo
    ${_pr32_icon_arg}
    DISPLAY_NAME "PixelRoot32 Sprites")
