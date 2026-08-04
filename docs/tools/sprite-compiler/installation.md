# Sprite Compiler Installation

This guide covers all installation paths for the PixelRoot32 Sprite Compiler.

## Method 1: Install the Tool Suite (recommended)

The Sprite Compiler is integrated as a **native GUI module** inside the **PixelRoot32 Tool Suite**.

1. **Download** the Tool Suite from [pixelroot32.com](https://pixelroot32.com)
2. **Install** the platform package (Windows installer, Linux AppImage, macOS bundle)
3. **Launch** the Tool Suite and click **"Sprite Compiler"** on the launcher

The module discovers the `pr32-sprite-compiler` CLI automatically:
- Looked for alongside the Tool Suite executable (bundled in the installer).
- Alternatively, available on your system `PATH`.

If the CLI is missing, the module prints an error to the console and exports will fail. Re-run the Tool Suite installer or add `pr32-sprite-compiler` ([releases](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler/releases)) to your `PATH`.

## Method 2: Standalone CLI (Python source)

> For CI scripts, build automation, or development environments without the Tool Suite.

Run the tool from a clone of the repository; the CLI entry point is **`python main.py`**.

### Prerequisites

- **Python**: Version 3.8 or higher
- **pip**: Usually included with Python

```bash
python --version   # Should show 3.8.0 or higher
pip --version
```

### Step 1: Clone repository

```bash
git clone https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler.git
cd PixelRoot32-Sprite-Sheet-Compiler
```

### Step 2: Install dependencies

```bash
pip install -e .
```

The project uses `pyproject.toml` (single dependency: Pillow). There is no `requirements.txt`.

### Step 3: Verify installation

```bash
python main.py --help
```

Work from this directory (or call `python` with the full path to `main.py`) when building sprites.

## Method 3: Pre-built native binary

The **[Releases](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler/releases)** page may ship standalone builds so you do **not** need to install Python. Exact file names change per release; pick the asset for your OS.

#### Windows

- Typically a **`.exe`** installer or portable executable — follow the release notes.

#### Linux

- Often an **AppImage** (chmod +x, then run), or a **`.deb`** / distro package.

#### macOS

- Often a **`.dmg`** or **`.app`**. If Gatekeeper blocks the app, allow it under **System Settings → Privacy & Security**.

> **Note:** If there is no binary for your platform yet, use **Method 2** (Python source).

## Verification (Standalone CLI)

1. Create a small **PNG** (e.g. 8×8 or 16×16).
2. Run (adjust `--grid` / `--sprite` to match):

```bash
python main.py test.png --grid 8x8 --sprite 0,0,1,1 --out test_output.h
```

3. Confirm `test_output.h` exists and contains data arrays.

```bash
python main.py --help
```

## Updating (source install)

```bash
cd PixelRoot32-Sprite-Sheet-Compiler
git pull
pip install -e .
```

## Uninstallation

- **Tool Suite**: uninstall the Tool Suite application (platform-specific).
- **Source:** delete the clone (and any venv you used).
- **Pre-built:** remove the downloaded installer / AppImage / app bundle.

## Troubleshooting

**`python` not found**

- On Windows, try `py` instead of `python`.
- Ensure Python is on `PATH`; restart the terminal.

**Import errors after install**

- Use `pip install -e .` with the same interpreter you use to run `main.py`.

**Cannot write output file**

- Ensure the output directory exists and is writable.

**Tool Suite: `pr32-sprite-compiler` not found**

- The CLI binary must be alongside the Tool Suite executable or on `PATH`. Download from the [releases](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler/releases) page.

### Getting help

- [Usage guide](/tools/sprite-compiler/usage-guide)
- Open an issue on the [Sprite Compiler](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler) repository if something fails.

## Next steps

- [Usage guide](/tools/sprite-compiler/usage-guide)
- [Advanced features](/tools/sprite-compiler/advanced-features)

## See also

- [Overview](/tools/sprite-compiler/overview)
