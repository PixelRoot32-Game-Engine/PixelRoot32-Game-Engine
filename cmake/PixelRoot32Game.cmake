# Copyright (c) 2026 PixelRoot32
# Licensed under the MIT License
#
# Packaging helpers, shipped inside the installed package next to
# `PixelRoot32Config.cmake` so a game gets them from `find_package()`.
#
#   pixelroot32_add_game(<target> [VERSION <v>] [ICON <png|icns>] <sources...>)
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
#
# The icon is one square PNG for every platform, the way Godot's
# `application/config/icon` is: Linux stages the PNG itself, macOS gets an
# `.icns` generated from it with `sips`/`iconutil` (both ship with the OS).
# Without `ICON` both helpers fall back to the engine's logo installed with the
# package, so a game is never packaged with the generic OS icon by accident.
# An `.icns` is accepted on macOS as an escape hatch for hand-tuned sizes;
# Windows would need an `.ico` compiled into the executable as a resource, and
# the CMake path has no verified Windows build yet (docs/guide/packaging.md).

# The ten entries an `.iconset` directory holds. iconutil rejects a set with a
# gap, so the list is complete even though the source PNG may be smaller than
# the largest size.
set(_PR32_ICONSET_ENTRIES
    "icon_16x16.png:16"
    "icon_16x16@2x.png:32"
    "icon_32x32.png:32"
    "icon_32x32@2x.png:64"
    "icon_128x128.png:128"
    "icon_128x128@2x.png:256"
    "icon_256x256.png:256"
    "icon_256x256@2x.png:512"
    "icon_512x512.png:512"
    "icon_512x512@2x.png:1024")

# Turns a PNG into `<name>.icns` in the caller's binary directory and returns the
# path in `<out_var>` (empty on failure, with a warning explaining why). The name
# is the game target, so `Contents/Resources` holds `my_game.icns` rather than a
# file named after the source image.
#
# Configure-time rather than a build rule: the bundle's Resources list is
# decided when the target is created, and the file has to exist by then. It runs
# once per icon and is skipped when the `.icns` is already newer.
function(_pixelroot32_icon_to_icns source name out_var)
    set(${out_var} "" PARENT_SCOPE)

    if(NOT APPLE)
        return()
    endif()

    find_program(PIXELROOT32_SIPS NAMES sips)
    find_program(PIXELROOT32_ICONUTIL NAMES iconutil)
    if(NOT PIXELROOT32_SIPS OR NOT PIXELROOT32_ICONUTIL)
        message(WARNING
            "Cannot convert '${source}' to an .icns: sips and iconutil are both "
            "part of macOS and were not found. The bundle will show the generic "
            "application icon. Pass an .icns as ICON, or install the Xcode "
            "command line tools.")
        return()
    endif()

    set(_work "${CMAKE_CURRENT_BINARY_DIR}/pixelroot32-icons")
    set(_iconset "${_work}/${name}.iconset")
    set(_icns "${_work}/${name}.icns")

    if(EXISTS "${_icns}")
        if(NOT "${source}" IS_NEWER_THAN "${_icns}")
            set(${out_var} "${_icns}" PARENT_SCOPE)
            return()
        endif()
    endif()

    file(REMOVE_RECURSE "${_iconset}")
    file(MAKE_DIRECTORY "${_iconset}")

    foreach(_entry IN LISTS _PR32_ICONSET_ENTRIES)
        string(REPLACE ":" ";" _pair "${_entry}")
        list(GET _pair 0 _file)
        list(GET _pair 1 _size)
        # -z height width: a non-square source is stretched here, which is why
        # the docs ask for a square PNG rather than silently cropping it.
        execute_process(
            COMMAND "${PIXELROOT32_SIPS}" -z "${_size}" "${_size}"
                    "${source}" --out "${_iconset}/${_file}"
            RESULT_VARIABLE _result
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT _result EQUAL 0)
            message(WARNING
                "sips could not resize '${source}' to ${_size}x${_size}; the "
                "bundle will show the generic application icon.")
            return()
        endif()
    endforeach()

    execute_process(
        COMMAND "${PIXELROOT32_ICONUTIL}" -c icns "${_iconset}" -o "${_icns}"
        RESULT_VARIABLE _result
        OUTPUT_QUIET ERROR_VARIABLE _error)
    if(NOT _result EQUAL 0 OR NOT EXISTS "${_icns}")
        message(WARNING
            "iconutil could not build an .icns from '${source}': ${_error}"
            "The bundle will show the generic application icon.")
        return()
    endif()

    set(${out_var} "${_icns}" PARENT_SCOPE)
endfunction()

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

    # One icon, no argument required: the engine's logo travels with the
    # package, so the default is "the engine" and not "whatever the OS draws".
    if(_ARG_ICON)
        # A game writes `ICON art/icon.png`, relative to its own source
        # directory. Resolving it here (that is where the directory is known)
        # is what keeps a relative path out of the build-time commands, which
        # run from the binary directory.
        get_filename_component(_ARG_ICON "${_ARG_ICON}" ABSOLUTE)
    else()
        if(PixelRoot32_DEFAULT_ICON AND EXISTS "${PixelRoot32_DEFAULT_ICON}")
            set(_ARG_ICON "${PixelRoot32_DEFAULT_ICON}")
        endif()
    endif()

    if(NOT _ARG_ICON)
        message(WARNING
            "pixelroot32_add_game(${target}): the package has no default "
            "icon (${PixelRoot32_DEFAULT_ICON}), so '${target}' is packaged "
            "with the generic OS icon. Pass ICON <square.png>.")
    elseif(NOT EXISTS "${_ARG_ICON}")
        message(FATAL_ERROR
            "pixelroot32_add_game(${target}): ICON '${_ARG_ICON}' does not exist.")
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
            # A bundle icon has to be an .icns (Finder reads CFBundleIconFile and
            # renders nothing for a PNG sitting in Resources), so a PNG -- the
            # format every platform shares -- is converted here instead of asking
            # every game to keep two files in sync.
            get_filename_component(_pr32_icon_suffix "${_ARG_ICON}" EXT)
            string(TOLOWER "${_pr32_icon_suffix}" _pr32_icon_suffix)
            if(_pr32_icon_suffix STREQUAL ".icns")
                set(_pr32_bundle_icon "${_ARG_ICON}")
            else()
                _pixelroot32_icon_to_icns("${_ARG_ICON}" "${target}" _pr32_bundle_icon)
            endif()

            if(_pr32_bundle_icon)
                get_filename_component(_pr32_icon_name "${_pr32_bundle_icon}" NAME)
                set_target_properties(${target} PROPERTIES
                    MACOSX_BUNDLE_ICON_FILE "${_pr32_icon_name}")
                set_source_files_properties("${_pr32_bundle_icon}" PROPERTIES
                    MACOSX_PACKAGE_LOCATION "Resources")
                target_sources(${target} PRIVATE "${_pr32_bundle_icon}")
            endif()
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

    # Same default as the bundle, and for the same reason: appimagetool reports
    # a missing .DirIcon and carries on, so "no icon" is a green build with a
    # generic artifact.
    if(_ARG_ICON)
        # Same resolution as the bundle: relative to the game's source directory
        # at configure time, never to the directory the target runs in.
        get_filename_component(_ARG_ICON "${_ARG_ICON}" ABSOLUTE)
    elseif(PixelRoot32_DEFAULT_ICON AND EXISTS "${PixelRoot32_DEFAULT_ICON}")
        set(_ARG_ICON "${PixelRoot32_DEFAULT_ICON}")
    else()
        message(WARNING
            "pixelroot32_add_appimage(${_ARG_NAME}): the package has no "
            "default icon, so the AppImage is built without one. "
            "Pass ICON <square.png>.")
    endif()

    # The AppDir root takes the PNG under the app's name; an .icns cannot be
    # used as-is there, and converting one would need tools Linux does not
    # reliably have, so the default covers the case instead.
    if(_ARG_ICON)
        if(NOT EXISTS "${_ARG_ICON}")
            message(FATAL_ERROR
                "pixelroot32_add_appimage(${_ARG_NAME}): ICON '${_ARG_ICON}' does not exist.")
        endif()
        get_filename_component(_pr32_appimage_suffix "${_ARG_ICON}" EXT)
        string(TOLOWER "${_pr32_appimage_suffix}" _pr32_appimage_suffix)
        if(NOT _pr32_appimage_suffix STREQUAL ".png")
            message(WARNING
                "pixelroot32_add_appimage(${_ARG_NAME}): ICON '${_ARG_ICON}' is "
                "not a PNG. An AppImage icon is a PNG at the AppDir root; the "
                "engine default is used instead.")
            if(PixelRoot32_DEFAULT_ICON AND EXISTS "${PixelRoot32_DEFAULT_ICON}")
                set(_ARG_ICON "${PixelRoot32_DEFAULT_ICON}")
            else()
                set(_ARG_ICON "")
            endif()
        endif()
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
