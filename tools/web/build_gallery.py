#!/usr/bin/env python3
"""Assembles the static gallery for the browser demos.

`tools/web/build_web.py` turns one example into `<name>.html` + `.js` + `.wasm`.
This script is the other half: it takes a directory holding those builds, copies
them into a self-contained site next to an `index.html` it writes, and fails
when a demo listed below has no build behind it. A gallery that links to a page
nobody built is worse than no gallery, so a missing artifact is an error and not
a card that 404s.

    # one example, then the site
    pio run -e native -d examples/sprites -t compiledb
    python3 tools/web/build_web.py --example sprites
    python3 tools/web/build_gallery.py --build-dir build/web_engine --out build/web_engine/site
    python3 -m http.server 8000 --directory build/web_engine/site

    # the CI matrix, so the list of demos lives in one place
    python3 tools/web/build_gallery.py --matrix

`DEMOS` is the single source of truth for what the gallery publishes: the
workflow asks for `--matrix` instead of repeating the list in YAML, and the
build page in the workflow keys its assertions off the same list.

The page is deliberately static — no JavaScript, no external font, no analytics,
one copied logo — because it is the artifact that is deployed to Pages, and the
only thing it has to do is link to the pages Emscripten wrote. It must be served
over HTTP: a `.wasm` fetched from `file://` is blocked by the browser, so
`python3 -m http.server` is part of the recipe and not a suggestion.
"""

import argparse
import html
import json
import os
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


@dataclass(frozen=True)
class Demo:
    """One example as the gallery presents it.

    `controls` is copied from the example's own README, not invented here: the
    gallery is a door to the demos, not a second manual that can drift.
    """

    slug: str
    title: str
    tag: str
    summary: str
    controls: str


# The order is the "Suggested learning order" of examples/README.md, with the
# hardware-facing demos last. Every entry must build with tools/web/build_web.py.
# The ESP-NOW examples are deliberately absent: they are a point solution for a
# radio only an ESP32 has, and two simulated boards on one page would teach the
# stub, not the technology.
DEMOS = (
    Demo(
        slug="sprites",
        title="Sprites and palettes",
        tag="Graphics",
        summary=(
            "2bpp and 4bpp sprites animating over a drawn background, with the "
            "16-entry palette table rendered underneath them."
        ),
        controls="A cycles single, dual and dual-inverted palette modes.",
    ),
    Demo(
        slug="camera",
        title="Camera",
        tag="Graphics",
        summary=(
            "Camera2D smoothing and bounds, parallax, tile platforms, and a "
            "scripted tween next to shake, punch and offset effects."
        ),
        controls=(
            "Left / Right move, A jumps, B cycles the camera effect, "
            "Up pans to the third platform, Down cancels every effect."
        ),
    ),
    Demo(
        slug="animated_tilemap",
        title="Animated tilemap",
        tag="Graphics",
        summary=(
            "Tile animation, palette swapping and the static tilemap framebuffer "
            "cache that keeps a still map off the SPI bus."
        ),
        controls="Runs on its own — there is no input.",
    ),
    Demo(
        slug="physics",
        title="Physics",
        tag="Physics",
        summary=(
            "Rigid, kinematic and static actors, a layer-aware radius query "
            "overlay, and particles spawned from the collision callback."
        ),
        controls=(
            "Keyboard moves the player; the pointer goes through the same touch "
            "pipeline the board's panel uses."
        ),
    ),
    Demo(
        slug="platformer",
        title="Platformer",
        tag="Physics",
        summary=(
            "The three actor types in the scene they were designed for: one-way "
            "platforms and collision layers stated on both sides."
        ),
        controls="Left / Right move, A jumps while grounded, B resets the scene.",
    ),
    Demo(
        slug="dialog",
        title="Dialog",
        tag="UI",
        summary=(
            "A headless DialogRunner in five states, driven by semantic actions, "
            "with DialogBox drawing an auto-advance line, a chain and a three-way "
            "choice."
        ),
        controls="Up / Down choose, A advances or confirms, B cancels.",
    ),
    Demo(
        slug="persistence",
        title="Persistence",
        tag="Persistence",
        summary=(
            "A settings record and a numbered save slot through "
            "PersistenceService, with the store result and the record's schema "
            "and CRC reported on load."
        ),
        controls=(
            "A mutates both records, saves them and reads them back; reload the "
            "page to see the save survive in the browser's IndexedDB."
        ),
    ),
    Demo(
        slug="mono_oled",
        title="Monochrome OLED",
        tag="Display",
        summary=(
            "A 1-bit SSD1306 through U8G2_Drawer: a 72x40 logical screen inside a "
            "128x64 framebuffer, and the renderer paths a monochrome panel gives up."
        ),
        controls="Space moves to the next page.",
    ),
    Demo(
        slug="apu_profiles",
        title="APU sound profiles",
        tag="Audio",
        summary=(
            "Legacy, NES and Famicom on the same nine 12-second blocks, so the "
            "profiles are compared on identical notes rather than on a favourite tune."
        ),
        controls="Runs on its own. Browser audio starts after a click on the page.",
    ),
)


PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PixelRoot32 — examples in the browser</title>
<style>
  :root {{
    color-scheme: dark;
    --bg: #0e1014;
    --card: #171a21;
    --line: #272c36;
    --ink: #e8e6e1;
    --muted: #9aa3b2;
    --accent: #f0b429;
  }}
  * {{ box-sizing: border-box; }}
  body {{
    margin: 0;
    padding: 2.5rem 1.25rem 4rem;
    background: var(--bg);
    color: var(--ink);
    font: 16px/1.55 -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  }}
  header, main, footer {{ max-width: 62rem; margin: 0 auto; }}
  header {{ display: flex; gap: 1.25rem; align-items: center; flex-wrap: wrap; }}
  header img {{ width: 64px; height: 64px; image-rendering: pixelated; }}
  h1 {{ font-size: 1.6rem; margin: 0 0 .25rem; letter-spacing: -.01em; }}
  .lede {{ margin: 0; color: var(--muted); max-width: 46rem; }}
  .note {{
    margin: 1.5rem 0 2rem;
    padding: .85rem 1rem;
    border: 1px solid var(--line);
    border-left: 3px solid var(--accent);
    border-radius: 6px;
    background: var(--card);
    color: var(--muted);
    font-size: .92rem;
  }}
  .note strong {{ color: var(--ink); }}
  .grid {{
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(17rem, 1fr));
    gap: 1rem;
    padding: 0;
    margin: 0;
    list-style: none;
  }}
  .card {{
    display: flex;
    flex-direction: column;
    gap: .5rem;
    padding: 1.1rem 1.15rem 1.25rem;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 8px;
    color: inherit;
    text-decoration: none;
    transition: border-color .12s ease, transform .12s ease;
  }}
  .card:hover, .card:focus-visible {{ border-color: var(--accent); transform: translateY(-1px); }}
  .card:focus-visible {{ outline: 2px solid var(--accent); outline-offset: 2px; }}
  .tag {{
    font: 600 .7rem/1 ui-monospace, SFMono-Regular, Menlo, monospace;
    letter-spacing: .08em;
    text-transform: uppercase;
    color: var(--accent);
  }}
  .card h2 {{ font-size: 1.05rem; margin: 0; }}
  .card p {{ margin: 0; font-size: .9rem; color: var(--muted); }}
  .keys {{ font-size: .82rem !important; color: #b9c0cc !important; }}
  .play {{ margin-top: auto; padding-top: .35rem; font-weight: 600; font-size: .9rem; }}
  footer {{ margin-top: 2.5rem; color: var(--muted); font-size: .85rem; }}
  footer a {{ color: inherit; }}
</style>
</head>
<body>
<header>
  <img src="pr32_logo.png" alt="PixelRoot32 logo">
  <div>
    <h1>PixelRoot32 in the browser</h1>
    <p class="lede">Nine examples from the engine repository, compiled to WebAssembly.
    Each one isolates a single capability and stops there: read the source next to the
    page, then copy the folder.</p>
  </div>
</header>
<p class="note"><strong>Click a demo to play it.</strong> The engine's browser build needs
no cross-origin isolation, so these pages run on plain static hosting. Audio starts
after a click.</p>
<main>
  <ul class="grid">
{cards}
  </ul>
</main>
<footer>
  Built from <code>{revision}</code>. Source and build recipe:
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine">PixelRoot32-Game-Engine</a>
  — see <code>docs/guide/web-demos.md</code>.
</footer>
</body>
</html>
"""

CARD = """    <li>
      <a class="card" href="{slug}.html">
        <span class="tag">{tag}</span>
        <h2>{title}</h2>
        <p>{summary}</p>
        <p class="keys">{controls}</p>
        <span class="play">Play &rarr;</span>
      </a>
    </li>"""


def artifacts(build_dir: Path, slug: str) -> list[Path]:
    """The three files Emscripten writes for one demo."""
    return [build_dir / f"{slug}{suffix}" for suffix in (".html", ".js", ".wasm")]


def missing(build_dir: Path) -> list[str]:
    """Demo slugs whose build is absent or empty, in catalogue order."""
    return [
        demo.slug
        for demo in DEMOS
        if any(not path.is_file() or path.stat().st_size == 0 for path in artifacts(build_dir, demo.slug))
    ]


def render(revision: str) -> str:
    cards = "\n".join(
        CARD.format(
            slug=html.escape(demo.slug),
            tag=html.escape(demo.tag),
            title=html.escape(demo.title),
            summary=html.escape(demo.summary),
            controls=html.escape(demo.controls),
        )
        for demo in DEMOS
    )
    return PAGE.format(cards=cards, revision=html.escape(revision))


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--build-dir", default="build/web_engine", help="where build_web.py wrote the demos")
    parser.add_argument("--out", default="build/web_engine/site", help="site directory to write")
    parser.add_argument("--revision", default=os.environ.get("GITHUB_SHA", "")[:7], help="commit the site was built from")
    parser.add_argument("--matrix", action="store_true", help="print the demo list as a JSON array and exit")
    parser.add_argument("--list", action="store_true", help="print the demo slugs, one per line, and exit")
    args = parser.parse_args()

    if args.matrix:
        print(json.dumps([demo.slug for demo in DEMOS]))
        return 0
    if args.list:
        print("\n".join(demo.slug for demo in DEMOS))
        return 0

    build_dir = Path(args.build_dir)
    out_dir = Path(args.out)
    # The output directory is deleted before it is written, so a mistyped --out
    # must not be able to take the repository with it.
    if out_dir.resolve() in (ROOT, ROOT.parent) or out_dir.resolve() in ROOT.parents:
        print(f"refusing to write the gallery to {out_dir.resolve()}: pick a directory inside the project", file=sys.stderr)
        return 1

    absent = missing(build_dir)
    if absent:
        print(f"{build_dir} has no complete build for: {', '.join(absent)}", file=sys.stderr)
        print("build them first, e.g. python3 tools/web/build_web.py --example sprites", file=sys.stderr)
        return 1

    # Rebuilt from scratch: a demo removed from DEMOS must not survive in the
    # deployed site because a previous run left its files behind.
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    for demo in DEMOS:
        for path in artifacts(build_dir, demo.slug):
            shutil.copy2(path, out_dir / path.name)

    logo = ROOT / "assets" / "pr32_logo.png"
    if not logo.is_file():
        print(f"missing gallery logo at {logo.relative_to(ROOT)}", file=sys.stderr)
        return 1
    shutil.copy2(logo, out_dir / "pr32_logo.png")

    (out_dir / "index.html").write_text(render(args.revision or "local"), encoding="utf-8")

    total = sum(path.stat().st_size for path in out_dir.iterdir())
    print(f"gallery -> {out_dir} ({len(DEMOS)} demos, {total} bytes)")
    print("serve it with: python3 -m http.server 8000 --directory " + str(out_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
