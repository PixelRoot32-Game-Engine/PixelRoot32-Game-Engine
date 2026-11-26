# Copyright (c) 2026 PixelRoot32
# Licensed under the MIT License
#
# Packaging helpers, shipped inside the installed package next to
# `PixelRoot32Config.cmake` so a game gets them from `find_package()`.
#
#   pixelroot32_add_game(<target> [VERSION <v>] [ICON <png>] <sources...>)
#
#       The executable linked against `PixelRoot32::Engine`. On macOS it is a
#       `.app` bundle (CMake's `MACOSX_BUNDLE`), which is what makes it
#       double-clickable and what carries the `Info.plist` keys Finder and
#       Gatekeeper read; everywhere else it stays a plain executable.
#
#   pixelroot32_add_appimage(<target> [NAME <n>] [DISPLAY_NAME <d>]
#                            [VERSION <v>] [ICON <png>])
#
#       Adds a `<name>_appimage` target that stages an AppDir from the built
#       executable and turns it into an AppImage with `appimagetool`. Linux
#       only; on other hosts the function explains that and does nothing, so a
#       cross-platform `CMakeLists.txt` does not need its own `if()`.
#
# Both are deliberately thin. The engine's part in shipping a game is producing
# the same binary PlatformIO's `native` env produces and knowing how a desktop
# bundle is laid out on each platform; it is not a distribution channel of its
# own, which is why nothing here downloads, uploads or signs anything.

function(pixelroot32_add_game target)
    cmake_parse_arguments(_ARG "" "VERSION;ICON" "" ${ARGN})

    if(NOT _ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "pixelroot32_add_game(${target}) needs at least one source file.")
    endif()

    # The version a bundle reports is the engine's, unless the game sets its
    # own: a game that ships 2.0 with engine 1.16 is normal.
    if(NOT _ARG_VERSION)
        if(PixelRoot32_VERSION)
            set(_ARG_VERSION "${PixelRoot32_VERSION}")
        else()
            set(_ARG_VERSION "0")
        endif()
    endif()

    # MACOSX_BUNDLE is a keyword of add_executable(), so it has to be decided
    # before the target exists.
    set(_pr32_bundle "")
    if(APPLE)
        set(_pr32_bundle MACOSX_BUNDLE)
    endif()

    add_executable(${target} ${_pr32_bundle} ${_ARG_UNPARSED_ARGUMENTS})
    target_link_libraries(${target} PRIVATE PixelRoot32::Engine)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if(APPLE)
        # A plain executable launches from a terminal; a bundle is what Finder
        # opens, and these are the keys the generated Info.plist needs to be
        # treated as a GUI app rather than a scored one. The working directory
        # of a bundle launched from Finder is `/`, which is why the engine's
        # assets being compiled in (examples ship their sprites as C++ headers)
        # matters here.
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "${target}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.pixelroot32.${target}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${_ARG_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${_ARG_VERSION}"
            # CMake's default template omits NSHighResolutionCapable and
            # NSPrincipalClass, which a Finder-launched game wants; this one is
            # shipped with the package for the same reason the SDL2 module is.
            MACOSX_BUNDLE_INFO_PLIST
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PixelRoot32Info.plist.in")

        if(_ARG_ICON)
            if(NOT EXISTS "${_ARG_ICON}")
                message(FATAL_ERROR
                    "pixelroot32_add_game(${target}): ICON '${_ARG_ICON}' does not exist.")
            endif()
            # A bundle icon has to be an .icns: Finder reads CFBundleIconFile and
            # renders nothing for a PNG sitting in Resources, which looks like a
            # packaging bug rather than the format mismatch it is.
            get_filename_component(_pr32_icon_suffix "${_ARG_ICON}" EXT)
            if(NOT _pr32_icon_suffix STREQUAL ".icns")
                message(WARNING
                    "pixelroot32_add_game(${target}): ICON '${_ARG_ICON}' is not an .icns, "
                    "so the bundle will show the generic application icon. Both artifacts "
                    "can use their own format: a PNG for the AppImage, an .icns here.")
            endif()
            get_filename_component(_pr32_icon_name "${_ARG_ICON}" NAME)
            set_target_properties(${target} PROPERTIES
                MACOSX_BUNDLE_ICON_FILE "${_pr32_icon_name}")
            set_source_files_properties("${_ARG_ICON}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources")
            target_sources(${target} PRIVATE "${_ARG_ICON}")
        endif()
    endif()
endfunction()

function(pixelroot32_add_appimage target)
    cmake_parse_arguments(_ARG "" "NAME;DISPLAY_NAME;VERSION;ICON" "" ${ARGN})

    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "pixelroot32_add_appimage(${target}): '${target}' is not a target. "
            "Call pixelroot32_add_game() first.")
    endif()

    if(NOT _ARG_NAME)
        set(_ARG_NAME "${target}")
    endif()
    if(NOT _ARG_DISPLAY_NAME)
        set(_ARG_DISPLAY_NAME "${_ARG_NAME}")
    endif()
    if(NOT _ARG_VERSION)
        if(PixelRoot32_VERSION)
            set(_ARG_VERSION "${PixelRoot32_VERSION}")
        else()
            set(_ARG_VERSION "0")
        endif()
    endif()

    if(NOT UNIX OR APPLE)
        message(STATUS
            "pixelroot32_add_appimage: AppImage is a Linux format, so '${_ARG_NAME}_appimage' "
            "is not created on this host. macOS ships the .app bundle built by "
            "pixelroot32_add_game() instead.")
        return()
    endif()

    # Which SDL2 the package resolved decides whether there is an SDL2 runtime
    # to ship inside the AppImage: the static build's objects are already in
    # the executable, the shared one has to travel with it.
    get_target_property(_pr32_sdl2_link PixelRoot32::SDL2 INTERFACE_LINK_LIBRARIES)
    set(_pr32_sdl2_runtime "")
    foreach(_lib IN LISTS _pr32_sdl2_link)
        if(_lib STREQUAL "SDL2::SDL2")
            set(_pr32_sdl2_runtime "$<TARGET_FILE:SDL2::SDL2>")
        endif()
    endforeach()

    find_program(PIXELROOT32_APPIMAGETOOL
        NAMES appimagetool
        DOC "appimagetool used by the <name>_appimage targets")

    set(_pr32_appdir "${CMAKE_CURRENT_BINARY_DIR}/AppDir/${_ARG_NAME}")
    set(_pr32_appimage "${CMAKE_CURRENT_BINARY_DIR}/${_ARG_NAME}.AppImage")

    # A property, not a compile definition: the script is text the package
    # installs, and `CMAKE_CURRENT_FUNCTION_LIST_DIR` keeps the reference
    # relative to the package instead of the machine that built it.
    set(_pr32_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PixelRoot32AppImage.sh")

    add_custom_target(${_ARG_NAME}_appimage
        COMMAND ${CMAKE_COMMAND} -E env
            "APPIMAGETOOL=${PIXELROOT32_APPIMAGETOOL}"
            sh "${_pr32_script}"
            "$<TARGET_FILE:${target}>"
            "${_pr32_appdir}"
            "${_ARG_NAME}"
            "${_ARG_DISPLAY_NAME}"
            "${_ARG_VERSION}"
            "${_pr32_sdl2_runtime}"
            "${_ARG_ICON}"
            "${_pr32_appimage}"
        DEPENDS ${target}
        BYPRODUCTS "${_pr32_appimage}"
        COMMENT "Packaging ${_ARG_DISPLAY_NAME} as an AppImage"
        VERBATIM)
endfunction()
