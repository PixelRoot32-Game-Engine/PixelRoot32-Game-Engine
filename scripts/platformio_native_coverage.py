"""Add gcov coverage instrumentation to a native test environment.

`[env:native_test]` cannot express "instrument everywhere except macOS":
PlatformIO has no per-OS build flags, so `--coverage -lgcov` was committed for
every host. Apple's clang has no `libgcov` (`ld: library 'gcov' not found`), and
it does not need it: clang's own `--coverage` links its profiling runtime, which
is why the flag has to reach the linker explicitly here.
"""

import platform

Import("env")  # noqa: F821 - injected by PlatformIO (SCons)

env.AppendUnique(BUILD_FLAGS=["--coverage"])
env.AppendUnique(LINKFLAGS=["--coverage"])

if platform.system() != "Darwin":
    # GCC and MinGW link libgcov explicitly; clang calls it a missing library.
    env.AppendUnique(LINKFLAGS=["-lgcov"])
