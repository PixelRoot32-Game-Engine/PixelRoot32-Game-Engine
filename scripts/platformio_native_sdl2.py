"""Point a `native` example build at this machine's SDL2 installation.

PlatformIO has no per-OS build flags. Each example's `[env:native]` used to
commit the Windows/MSYS2 include and library paths and leave the macOS Homebrew
ones commented out, which meant the example only built after someone flipped
those comments by hand -- on macOS it failed with `'SDL2/SDL.h' file not found`,
and `-mconsole` is an option clang rejects outright.

This pre-script resolves the SDL2 prefix for the host actually running the
build, so one committed `platformio.ini` works on Windows, macOS and Linux:

    [base_native]
    extra_scripts = pre:../../scripts/platformio_native_sdl2.py

Linux needs nothing: the distribution package (libsdl2-dev) puts the headers and
libraries on the default search paths.
"""

import os
import platform

Import("env")  # noqa: F821 - injected by PlatformIO (SCons)


def _has_sdl2(prefix):
    return bool(prefix) and os.path.isdir(os.path.join(prefix, "include", "SDL2"))


def _darwin_prefix():
    candidates = [os.environ.get("HOMEBREW_PREFIX"), "/opt/homebrew", "/usr/local"]
    return next((prefix for prefix in candidates if _has_sdl2(prefix)), None)


def _windows_prefix():
    candidates = [os.environ.get("MINGW_PREFIX")]
    for base in ("C:/msys64", "C:/msys2"):
        for flavour in ("mingw64", "ucrt64", "clang64"):
            candidates.append(os.path.join(base, flavour))
    return next((prefix for prefix in candidates if _has_sdl2(prefix)), None)


_system = platform.system()

if _system == "Darwin":
    _prefix = _darwin_prefix()
elif _system == "Windows":
    _prefix = _windows_prefix()
else:
    # Linux and the BSDs carry SDL2 on the default search paths.
    _prefix = None

if _prefix:
    env.AppendUnique(CPPPATH=[os.path.join(_prefix, "include")])
    env.AppendUnique(LIBPATH=[os.path.join(_prefix, "lib")])
elif _system in ("Darwin", "Windows"):
    print(
        "platformio_native_sdl2: no SDL2 installation found. Install it "
        "(macOS: brew install sdl2; MSYS2: pacman -S mingw-w64-x86_64-SDL2) "
        "or add its include and library paths to [env:native] build_flags."
    )

if _system == "Windows":
    # MinGW already links a console subsystem binary for a plain `main`, but the
    # examples are terminal programs: state it instead of relying on the default.
    env.AppendUnique(LINKFLAGS=["-mconsole"])
