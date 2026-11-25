# Copyright (c) 2026 PixelRoot32
# Licensed under the MIT License
#
# Resolves SDL2 for PixelRoot32 and exposes it as `PixelRoot32::SDL2`.
#
# Two things make this a module and not a bare `find_package(SDL2)`:
#
# 1. The engine's public headers include `<SDL2/SDL.h>` (the installer layout
#    SDL2's own README documents for `SDL2_DIR`), while SDL2's CMake config
#    exports `<prefix>/include/SDL2` as the include directory. Using that target
#    alone compiles `<SDL.h>` and fails on `<SDL2/SDL.h>`, which is exactly the
#    error `scripts/platformio_native_sdl2.py` exists to avoid on PlatformIO.
#    Whatever layout the found package describes, this module adds the parent
#    directory only when `<that>/SDL2/SDL.h` really exists.
#
# 2. The install tree needs the same answer as the build tree. The generated
#    `PixelRoot32Config.cmake` includes this file, so a consumer of the
#    installed package resolves SDL2 with the same rules and gets the same
#    `PixelRoot32::SDL2` target the exported targets were linked against.
#
# The path is resolved where the package is consumed, never baked into the
# export: an absolute prefix recorded on the build machine is a broken package
# on every other machine.

if(TARGET PixelRoot32::SDL2)
    return()
endif()

# SDL2 ships a CMake config package since 2.0.12, and every platform this engine
# advertises (Homebrew, `libsdl2-dev` on Debian/Ubuntu, MSYS2) ships it. A
# missing config package is reported as missing SDL2 rather than guessed at.
find_package(SDL2 CONFIG QUIET)

if(NOT SDL2_FOUND)
    message(FATAL_ERROR
        "SDL2 was not found with its CMake config package.\n"
        "Install it first: `brew install sdl2` (macOS), "
        "`sudo apt install libsdl2-dev` (Debian/Ubuntu), or "
        "`pacman -S mingw-w64-x86_64-SDL2` (MSYS2).\n"
        "If it is installed somewhere unusual, point CMAKE_PREFIX_PATH (or "
        "SDL2_DIR) at its prefix.")
endif()

if(TARGET SDL2::SDL2)
    set(_pr32_sdl2_link SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(_pr32_sdl2_link SDL2::SDL2-static)
else()
    message(FATAL_ERROR
        "SDL2's config package was found but defines neither SDL2::SDL2 nor "
        "SDL2::SDL2-static.")
endif()

get_target_property(_pr32_sdl2_includes ${_pr32_sdl2_link} INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _pr32_sdl2_includes)
    set(_pr32_sdl2_includes "")
endif()

# The include directory the engine's headers need, which is the one that holds
# an `SDL2/` subdirectory: either the exported `<prefix>/include/SDL2` (add the
# parent) or already `<prefix>/include` (keep it). Nothing is assumed about the
# layout: both candidates are probed.
set(_pr32_sdl2_header_dirs "")
foreach(_dir IN LISTS _pr32_sdl2_includes)
    if(EXISTS "${_dir}/SDL2/SDL.h")
        list(APPEND _pr32_sdl2_header_dirs "${_dir}")
    endif()
    get_filename_component(_parent "${_dir}" DIRECTORY)
    if(EXISTS "${_parent}/SDL2/SDL.h")
        list(APPEND _pr32_sdl2_header_dirs "${_parent}")
    endif()
endforeach()
list(REMOVE_DUPLICATES _pr32_sdl2_header_dirs)

if(NOT _pr32_sdl2_header_dirs)
    message(FATAL_ERROR
        "SDL2 was found, but none of its include directories contains "
        "SDL2/SDL.h (looked at: ${_pr32_sdl2_includes}). The engine's headers "
        "include <SDL2/SDL.h>; install the full SDL2 development package rather "
        "than a stripped runtime one.")
endif()

add_library(PixelRoot32::SDL2 INTERFACE IMPORTED)
set_target_properties(PixelRoot32::SDL2 PROPERTIES
    INTERFACE_LINK_LIBRARIES ${_pr32_sdl2_link}
    INTERFACE_INCLUDE_DIRECTORIES "${_pr32_sdl2_header_dirs}")

unset(_pr32_sdl2_link)
unset(_pr32_sdl2_includes)
unset(_pr32_sdl2_header_dirs)
unset(_dir)
unset(_parent)
