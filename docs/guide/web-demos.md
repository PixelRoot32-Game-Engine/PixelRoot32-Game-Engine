# The examples in the browser

Every example in `examples/` compiles to WebAssembly and runs in a browser. That
is not a separate port: it is the same engine, the same `Scene` code and the same
`PIXELROOT32_ENABLE_*` flags, with Emscripten's SDL2 port standing in for the
host's. Phase 2 of the multiplatform port measured the renderer and phase 3
removed the audio thread, so nothing here needs `SharedArrayBuffer`,
cross-origin isolation or a special server header: the pages are plain static
files and run on any static host.

## Try one locally

The web build takes its defines and include paths from PlatformIO's compile
database, so the database comes first:

```bash
cd examples/sprites
pio run -e native -t compiledb
cd ../..
python3 tools/web/build_web.py --example sprites
python3 -m http.server 8000 --directory build/web_engine
```

Open <http://localhost:8000/sprites.html>. Serving over HTTP is not optional: a
`.wasm` fetched from `file://` is blocked by the browser.

`build/web_engine/` is ignored by git. The script and this page are not.

## The gallery

`tools/web/build_gallery.py` copies the built demos into one directory with an
`index.html` in front of them:

```bash
# after building the demos you want
python3 tools/web/build_gallery.py --build-dir build/web_engine --out build/web_engine/site
python3 -m http.server 8000 --directory build/web_engine/site
```

Its `DEMOS` table is the catalogue: title, one-line summary, controls and the
order they appear in. The script fails when a listed demo has no complete
`html`/`js`/`wasm` build, so the page cannot link to something CI never
produced. Controls are copied from each example's own README rather than
invented, because a second manual is a manual that drifts.

The page itself is static: no JavaScript beyond the Emscripten glue of the pages
it links to, no external font, no analytics. It carries the engine logo and the
commit it was built from.

Each demo page is written around `tools/web/shell.html`, not Emscripten's default
shell: that one is a compiler demo's page, with Emscripten's logo, a "powered by
emscripten" banner and pointer-lock/resize controls. Ours keeps the runtime
contract (`Module`, the canvas, the status line) and adds a bar with the example's
name and a link back to the gallery. The canvas sizing there is deliberate too —
Emscripten's SDL reads the canvas's CSS box back as the window size, so
`max-width` on the canvas or a flex container around it collapses it to a few
pixels.

## What CI does

`.github/workflows/web.yml` builds one demo per matrix entry — the list comes
from `build_gallery.py --matrix`, not from a second copy in YAML — then:

- each job asserts the page, its glue and its wasm are non-empty, and that
  `persistence`'s glue contains `IDBFS`;
- a `gallery` job collects the builds, assembles `_site` and uploads it as the
  **`web-demos`** artifact on every push and pull request;
- a `deploy` job publishes that artifact to GitHub Pages, but **only when the
  workflow is started by hand**. No push deploys anything.

### Trying the demos

The artifact is the distribution. The port is a closed beta under observation,
so there is no published site to compare against; what CI assembled is what you
test:

1. Open the `web` workflow run for the commit you care about.
2. Download the `web-demos` artifact.
3. Unzip it and serve the folder:

   ```bash
   unzip web-demos.zip -d web-demos
   python3 -m http.server 8000 --directory web-demos
   ```

The pages are then exactly what the runner built, which is the property a local
build does not have.

### Publishing to Pages, by hand

Nothing publishes on a push. The beta is closed and a published site is public
over HTTPS, so `deploy` runs only from a `workflow_dispatch` run of this
workflow: starting it by hand is the publish decision. Until then the
`web-demos` artifact above is the distribution.

If the port is ever published, two things are needed:

1. **Settings → Pages → Source: GitHub Actions**, once in the repository: the
   job is inert without a Pages site.
2. **Actions → `web` → Run workflow**, on the branch whose build you want to
   serve. The run rebuilds the demos and deploys what it built, not an older
   artifact.

Pages is one site per repository, so that run replaces whatever the site served
before. Nothing else is configured: no `gh-pages` branch, no token, no custom
domain. `actions/deploy-pages` uses the workflow's own OIDC token.

## What the browser build does, and does not, prove

What it proves: the tree compiles to a page on a machine that is not the
author's, the renderer draws, the input shim maps keyboard and pointer to the
engine's buttons and touch events, audio starts from a user gesture without a
second thread, and `PersistenceService` keeps a save in IndexedDB across a
reload.

What it does not prove: that a human heard the audio, that the frame time holds
on a slow machine, or that Safari behaves like Chrome. CI builds and assembles
the pages; it does not drive a browser through them.
