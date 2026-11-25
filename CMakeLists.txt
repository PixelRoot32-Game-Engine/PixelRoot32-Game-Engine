# Copyright (c) 2026 PixelRoot32
# Licensed under the MIT License
#
# PixelRoot32 Game Engine — desktop build, install and package config.
#
# This is the packaging/dependency path, not a second build system: PlatformIO
# remains the way the firmware and the examples are built, and ESP32 stays on
# `platformio.ini` (the board needs the Arduino core and the TFT_eSPI/U8G2
# drivers, none of which CMake can supply from a desktop). What CMake adds is
# what PlatformIO does not have: an installable package (`install()` +
# `find_package(PixelRoot32)`), which is what a game distributed as a plain
# CMake project, a `.app` or an AppImage needs.
#
# The engine produced here is the same `native` target PlatformIO builds: every
# source under `src/` except the firmware drivers, SDL2 as the display and
# input backend, and the same `PIXELROOT32_ENABLE_*` flags with the same
# defaults as `include/platforms/PlatformDefaults.h` and `EngineConfig.h`.
#
#   cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/cmake
#   cmake --install build/cmake --prefix <prefix>
#
# Consumers then do `find_package(PixelRoot32 REQUIRED)` and link
# `PixelRoot32::Engine`. `tools/cmake_consumer` is the acceptance test for that
# path: an out-of-tree project that includes nothing from this source tree.

cmake_minimum_required(VERSION 3.20)

# The version is read from library.json rather than written here, because that
# file is what the PlatformIO Registry publishes from. Two version strings is
# two places to forget.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/library.json" PR32_LIBRARY_JSON)
string(JSON PR32_VERSION GET "${PR32_LIBRARY_JSON}" version)
unset(PR32_LIBRARY_JSON)

project(PixelRoot32
    VERSION ${PR32_VERSION}
    DESCRIPTION "Lightweight modular 2D game engine for ESP32, desktop and the web"
    HOMEPAGE_URL "https://pixelroot32.org"
    LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# =============================================================================
# Feature flags
# =============================================================================
# These `option()`s are a transcription, not a catalogue: each one is a
# `PIXELROOT32_ENABLE_*` macro that already exists in the headers, with the
# default the header already declares. Nothing is invented here and no new
# switch is introduced.
#
# Two kinds of flag live in those headers and they are NOT interchangeable:
#
#   numeric (`#if PIXELROOT32_ENABLE_X`)  -> defined as 0 or 1
#   bare    (`#ifdef PIXELROOT32_ENABLE_X`) -> defined, or not defined at all
#
# Writing `-DPIXELROOT32_ENABLE_2BPP_SPRITES=0` for a bare flag would define it
# and switch it ON, which is why the second table below only ever adds the
# definition when the option is ON.
#
# `PIXELROOT32_ENABLE_SD_STORAGE` is deliberately absent: the header rejects it
# anywhere but ESP32, so exposing it here would only offer a configure step that
# cannot build. Firmware flags belong to platformio.ini.

set(PR32_NUMERIC_FLAGS
    # flag                              header default
    PIXELROOT32_ENABLE_AUDIO            1
    PIXELROOT32_ENABLE_PHYSICS          1
    PIXELROOT32_ENABLE_UI_SYSTEM        1
    PIXELROOT32_ENABLE_PARTICLES        1
    PIXELROOT32_ENABLE_CAMERA_EFFECTS   1
    PIXELROOT32_ENABLE_SCENE_TRANSITIONS 1
    PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE 1
    PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT 0
    PIXELROOT32_ENABLE_DIRTY_REGIONS    0
    PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING 0
    PIXELROOT32_ENABLE_TOUCH            0
    PIXELROOT32_ENABLE_NETWORKING       0
    PIXELROOT32_ENABLE_PERSISTENCE      0
    PIXELROOT32_ENABLE_GAMEPLAY_EVENTS  0
    PIXELROOT32_ENABLE_INTERACTION_TRIGGERS 0
    PIXELROOT32_ENABLE_SPATIAL_QUERY    0
    PIXELROOT32_ENABLE_DEPTH_SORT       0
    PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE 0
    PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL 0
    PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE 0
    PIXELROOT32_ENABLE_GAMEPLAY_ROOM    0
    PIXELROOT32_ENABLE_CAMERA_TWEEN     0
    PIXELROOT32_ENABLE_PROJECTION       0
    PIXELROOT32_ENABLE_TILEMAP_PROJECTION 0
    PIXELROOT32_ENABLE_DIALOG           0
    PIXELROOT32_ENABLE_FONT_LATIN1      0)

set(PR32_BARE_FLAGS
    PIXELROOT32_ENABLE_2BPP_SPRITES
    PIXELROOT32_ENABLE_4BPP_SPRITES
    PIXELROOT32_ENABLE_TILE_ANIMATIONS
    PIXELROOT32_ENABLE_SCENE_ARENA
    PIXELROOT32_ENABLE_PROFILING
    PIXELROOT32_ENABLE_DEBUG_OVERLAY
    PIXELROOT32_DEBUG_MODE)

# Only the flags whose value differs from the header default are emitted, so a
# default configure produces the same preprocessor state as a default
# PlatformIO build: no -D at all. That keeps the header the single place that
# decides what "default" means.
set(PR32_FEATURE_DEFINITIONS "")
set(_pr32_pending ${PR32_NUMERIC_FLAGS})
while(_pr32_pending)
    list(POP_FRONT _pr32_pending _flag _header_default)
    if(_header_default)
        set(_pr32_option_default ON)
    else()
        set(_pr32_option_default OFF)
    endif()
    option(${_flag} "Mirrors the ${_flag} macro (header default: ${_header_default})" ${_pr32_option_default})
    if(${_flag})
        set(_pr32_value 1)
    else()
        set(_pr32_value 0)
    endif()
    if(NOT _pr32_value STREQUAL _header_default)
        list(APPEND PR32_FEATURE_DEFINITIONS "${_flag}=${_pr32_value}")
    endif()
endwhile()

foreach(_flag IN LISTS PR32_BARE_FLAGS)
    option(${_flag} "Mirrors the ${_flag} macro (header default: not defined)" OFF)
    if(${_flag})
        list(APPEND PR32_FEATURE_DEFINITIONS "${_flag}=1")
    endif()
endforeach()

# =============================================================================
# Dependencies
# =============================================================================
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PixelRoot32SDL2.cmake")

# The synthesis core is a separate library with its own release cycle
# (PixelRoot32-APU, the same artifact PlatformIO resolves from
# `library.json.dependencies`). PlatformIO's lib_deps has no CMake counterpart,
# and an installed package cannot reference a target it does not ship, so the
# source is fetched here and both targets are exported together by this
# project. `library.json` stays the range; this tag is the pin CMake builds.
include(FetchContent)
set(PIXELROOT32_APU_TAG "v2.5.0" CACHE STRING
    "PixelRoot32-APU tag to build against")
FetchContent_Declare(pixelroot32_apu
    GIT_REPOSITORY https://github.com/PixelRoot32-Game-Engine/PixelRoot32-APU.git
    GIT_TAG ${PIXELROOT32_APU_TAG}
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(pixelroot32_apu)

# =============================================================================
# The engine
# =============================================================================
# Every source under src/, minus the firmware drivers. The PlatformIO build
# discovers its sources the same way, so a hand-written file list here would be
# a second inventory to keep in sync — which is how a new .cpp ends up building
# on one path and not the other. `src/platforms/mock` stays in: it is not
# test-only, it is where the desktop SPI/Arduino shims live.
file(GLOB_RECURSE PR32_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER PR32_SOURCES EXCLUDE REGEX "/src/drivers/esp32/")

add_library(pixelroot32 STATIC ${PR32_SOURCES})
add_library(PixelRoot32::Engine ALIAS pixelroot32)
set_target_properties(pixelroot32 PROPERTIES
    EXPORT_NAME Engine
    OUTPUT_NAME PixelRoot32
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(pixelroot32 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# `PLATFORM_NATIVE` and `SDL_MAIN_HANDLED` are the two flags
# `platformio.ini [base_native]` passes, and both are visible in public headers
# and in a game's own `main.cpp` (the examples select their platform header with
# `#ifdef PLATFORM_NATIVE`), so they belong on the interface rather than on the
# library alone. `PLATFORM_NATIVE` is what `PlatformSelect.h` reads to report
# the desktop identity.
target_compile_definitions(pixelroot32 PUBLIC
    PLATFORM_NATIVE
    SDL_MAIN_HANDLED
    ${PR32_FEATURE_DEFINITIONS})

# Public: the APU headers sit in the engine's public headers (`audio/AudioTypes.h`
# forwards to <pixelroot32/apu/AudioTypes.h>), so a consumer needs both the
# include path and, at link time, the synthesis core itself.
target_link_libraries(pixelroot32 PUBLIC pixelroot32-apu PixelRoot32::SDL2)
if(UNIX AND NOT APPLE)
    # libm is part of glibc since 2.34, but not of every distribution this
    # advertises.
    target_link_libraries(pixelroot32 PUBLIC m)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Matches the firmware and the PlatformIO native builds: the engine is
    # written without try/catch, and `-fno-exceptions` is what keeps it that
    # way. Private, so a game that uses exceptions is unaffected.
    target_compile_options(pixelroot32 PRIVATE -fno-exceptions)
endif()

# =============================================================================
# Install
# =============================================================================
install(TARGETS pixelroot32 pixelroot32-apu
    EXPORT PixelRoot32Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
    # include/test is the unit-test harness (Unity suites), not API.
    PATTERN "test" EXCLUDE)

# The APU headers are part of the engine's public surface: `audio/AudioMixerLUT.h`
# and the `audio/ApuCore.h` shim both forward to <pixelroot32/apu/...>.
install(DIRECTORY "${pixelroot32_apu_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

set(PR32_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/PixelRoot32")

install(EXPORT PixelRoot32Targets
    NAMESPACE PixelRoot32::
    DESTINATION ${PR32_CMAKE_INSTALL_DIR})

# The SDL2 resolver travels with the package: the exported targets link
# PixelRoot32::SDL2, and the consumer's config file has to be able to define it.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PixelRoot32SDL2.cmake"
    DESTINATION ${PR32_CMAKE_INSTALL_DIR})

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PixelRoot32Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PixelRoot32Config.cmake"
    INSTALL_DESTINATION ${PR32_CMAKE_INSTALL_DIR})

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PixelRoot32ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PixelRoot32Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PixelRoot32ConfigVersion.cmake"
    DESTINATION ${PR32_CMAKE_INSTALL_DIR})
