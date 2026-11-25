#!/usr/bin/env python3
"""Compiles one example for the browser with em++.

The compiler input is PlatformIO's compile database, so a web build uses the
exact defines and include paths of the `native` one -- including each example's
`PIXELROOT32_ENABLE_*` set -- instead of a second, hand-written flag list that
drifts. Generate the database first, then build:

    pio run -e native -d examples/sprites -t compiledb
    python3 tools/web/build_web.py --example sprites

The result lands in `build/web_engine/<example>.html` (+ `.js` and `.wasm`),
which is where the recipe in docs/_sdl2_multiplatform.md serves it from. Nothing
under `build/` is versioned: this script is, the artifacts are not.

Every em++ flag the build passes lives in the block below and nowhere else --
`BASE_COMPILE_FLAGS`, `BASE_LINK_FLAGS`, `HOST_PATH_PREFIXES` and the per-example
`EXAMPLES` table. Two things are deliberately not flags here: the defines, which
come from the database, and the extra per-example libraries such as `-lidbfs.js`,
which are part of that example's row.
"""

import argparse
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# =============================================================================
# THE FLAGS
# =============================================================================
COMPILER = os.environ.get("PR32_EMXX", "em++")

# Appended to every translation unit, after the database's own flags.
BASE_COMPILE_FLAGS: tuple[str, ...] = ()

# The link line: the objects above plus these. `-sUSE_SDL=2` is the SDL2 port
# that replaces the host SDK; `-sALLOW_MEMORY_GROWTH` keeps a 240x240 game with
# a few decoded assets from failing on a fixed heap.
BASE_LINK_FLAGS = ("-O2", "-sUSE_SDL=2", "-sALLOW_MEMORY_GROWTH")

# PlatformIO records the host's SDL2 for `native`. The browser gets SDL2 from the
# port above, so a host include or library path here would either compile
# against the wrong headers or try to link a native library into a wasm binary.
HOST_PATH_PREFIXES = (
    "-I/opt/homebrew",
    "-L/opt/homebrew",
    "-I/usr/local/include/SDL2",
    "-L/usr/local/lib",
    "-I/usr/include/SDL2",
)

# One row per example that needs more than the base link. An example with no row
# builds with the base flags alone, so adding one to the repository does not
# require editing this table.
EXAMPLES = {
    # sprites: the lightest full path (framebuffer, font, window, keyboard).
    "sprites": {},
    "persistence": {
        # Saves live in the IDBFS mount. Without the library the mount fails and
        # the example still runs -- with no persistence -- which is exactly the
        # silent regression a build check should catch, so the marker below
        # proves the library reached the link.
        "link_flags": ("-lidbfs.js",),
        "markers": ("IDBFS",),
    },
}


def compile_command(entry, index, obj_dir, extra_flags, extra_define_names):
    """Rewrites one database entry into an em++ command that writes an object."""
    args = shlex.split(entry["command"])
    args[0] = COMPILER

    cleaned = []
    skip_next = False
    for arg in args:
        if skip_next:
            skip_next = False
            continue
        if arg == "-o":
            skip_next = True
            continue
        if arg == "-c":
            continue
        if arg.startswith(HOST_PATH_PREFIXES):
            continue
        # An extra -D replaces the database's value for the same macro instead
        # of being appended next to it, where the last one would win by luck.
        if arg.startswith("-D") and arg.split("=")[0] in extra_define_names:
            continue
        cleaned.append(arg)

    cleaned += [*BASE_COMPILE_FLAGS, *extra_flags]
    obj = obj_dir / f"{index:03d}_{Path(entry['file']).name}.o"
    return [*cleaned, "-c", "-o", str(obj)], obj


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--example",
        default=os.environ.get("PR32_EXAMPLE", "sprites"),
        help="directory under examples/ to build (default: %(default)s)",
    )
    parser.add_argument(
        "--out",
        default=os.environ.get("PR32_OUT"),
        help="file name inside build/web_engine (default: <example>.html)",
    )
    parser.add_argument(
        "--extra-flags",
        default=os.environ.get("PR32_EXTRA_FLAGS", ""),
        help="extra flags for every em++ call; a -D here replaces the database's",
    )
    args = parser.parse_args()

    extra_flags = shlex.split(args.extra_flags)
    extra_define_names = {flag.split("=")[0] for flag in extra_flags if flag.startswith("-D")}
    config = EXAMPLES.get(args.example, {})

    example_dir = ROOT / "examples" / args.example
    out_dir = ROOT / "build" / "web_engine"
    obj_dir = out_dir / "obj" / args.example
    out_name = args.out or f"{args.example}.html"
    database_path = example_dir / "compile_commands.json"

    if not database_path.is_file():
        print(
            f"no compile database at {database_path.relative_to(ROOT)}\n"
            f"generate it first: pio run -e native -d examples/{args.example} -t compiledb",
            file=sys.stderr,
        )
        return 2

    with database_path.open() as handle:
        database = json.load(handle)

    obj_dir.mkdir(parents=True, exist_ok=True)
    print(f"em++ flags: compile={' '.join(BASE_COMPILE_FLAGS) or '-'} "
          f"link={' '.join(BASE_LINK_FLAGS)} "
          f"extra={' '.join(extra_flags) or '-'} "
          f"example={args.example} per-example-link="
          f"{' '.join(config.get('link_flags', ())) or '-'}")

    objects = []
    failures = []
    for index, entry in enumerate(database):
        command, obj = compile_command(entry, index, obj_dir, extra_flags, extra_define_names)
        result = subprocess.run(command, cwd=entry["directory"], capture_output=True, text=True)
        if result.returncode != 0:
            first = next(
                (line for line in result.stderr.splitlines() if "error" in line),
                result.stderr.splitlines()[0] if result.stderr else "?",
            )
            failures.append((os.path.relpath(entry["file"], example_dir), first.strip()))
        else:
            objects.append(obj)

    print(f"compiled {len(objects)}/{len(database)} TUs ({args.example})")
    if failures:
        print(f"\n{len(failures)} TU(s) failed:")
        for path, first in failures:
            print(f"  {path}\n    {first}")
        return 1

    out_path = out_dir / out_name
    link = [
        COMPILER,
        *BASE_LINK_FLAGS,
        *config.get("link_flags", ()),
        *extra_flags,
        *(str(obj) for obj in objects),
        "-o",
        str(out_path),
    ]
    result = subprocess.run(link, cwd=example_dir, capture_output=True, text=True)
    if result.returncode != 0:
        print("\nlink failed:")
        for line in result.stderr.splitlines():
            if "error" in line or "undefined" in line:
                print(f"  {line}")
        return 1

    # em++ writes the page, the glue and the wasm as one unit, so a zero exit is
    # not the whole answer: the artifacts are, and so is the marker that proves
    # a per-example library really was linked.
    artifacts = {suffix: out_dir / f"{out_path.stem}{suffix}" for suffix in (".html", ".js", ".wasm")}
    missing = [path for path in artifacts.values() if not path.is_file() or path.stat().st_size == 0]
    if missing:
        print("link reported success but these are missing or empty:")
        for path in missing:
            print(f"  {os.path.relpath(path, ROOT)}")
        return 1

    glue = artifacts[".js"].read_text(errors="replace")
    absent = [marker for marker in config.get("markers", ()) if marker not in glue]
    if absent:
        print(f"{os.path.relpath(artifacts['.js'], ROOT)} is missing {', '.join(absent)}: "
              f"the per-example link flags did not reach it")
        return 1

    sizes = " ".join(f"{suffix[1:]}={path.stat().st_size}" for suffix, path in artifacts.items())
    print(f"link OK -> {os.path.relpath(artifacts['.html'], ROOT)} ({sizes})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
