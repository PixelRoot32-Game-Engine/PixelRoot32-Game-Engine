# Sprite Compiler Installation

This guide walks you through installing the PixelRoot32 Sprite Compiler on your system.

## Prerequisites

### Required software

- **Python**: Version 3.8 or higher
- **pip**: Usually included with Python

### Verify prerequisites

```bash
python --version
# Should show 3.8.0 or higher
```

```bash
pip --version
```

If Python is missing, install it from [python.org](https://www.python.org/).

## Installation methods

### Method 1: From source

Run the tool from a clone of the repository; the CLI entry point is **`python main.py`**.

#### Step 1: Clone repository

```bash
git clone https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler.git
cd PixelRoot32-Sprite-Compiler
```

#### Step 2: Install dependencies

```bash
pip install -r requirements.txt
```

#### Step 3: Verify installation

```bash
python main.py --help
```

Work from this directory (or call `python` with the full path to `main.py`) when building sprites.

### Method 2: Pre-built binaries (no Python)

The **[Releases](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler/releases)** page may ship standalone builds so you do **not** need to install Python. Exact file names change per release; pick the asset for your OS.

#### Windows

- Typically a **`.exe`** installer or portable executable — follow the release notes.

#### Linux

- Often an **AppImage** (chmod +x, then run), or a **`.deb`** / distro package.

#### macOS

- Often a **`.dmg`** or **`.app`**. If Gatekeeper blocks the app, allow it under **System Settings → Privacy & Security**.

> **Note:** If there is no binary for your platform yet, use **Method 1**. From the Python tree, the GUI is usually started with `python main.py` (see the repository README).

## Verification

### Test conversion

1. Create a small **PNG** (e.g. 8×8 or 16×16).
2. Run (adjust `--grid` / `--sprite` to match):

```bash
python main.py test.png --grid 8x8 --sprite 0,0,1,1 --out test_output.h
```

3. Confirm `test_output.h` exists and contains data arrays.

### Help

```bash
python main.py --help
```

## Updating (source install)

```bash
cd PixelRoot32-Sprite-Compiler
git pull
pip install -r requirements.txt
```

## Uninstallation

- **Source:** delete the clone (and any venv you used).
- **Pre-built:** remove the downloaded installer / AppImage / app bundle.

## Troubleshooting

**`python` not found**

- On Windows, try `py` instead of `python`.
- Ensure Python is on `PATH`; restart the terminal.

**Import errors after `pip install`**

- Use `python -m pip install -r requirements.txt` with the same interpreter you use to run `main.py`.

**Cannot write output file**

- Ensure the output directory exists and is writable.

### Getting help

- [Usage guide](/tools/sprite-compiler/usage-guide) — CLI patterns (`python main.py …`).
- Open an issue on the [Sprite Compiler](ttps://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler/PixelRoot32-Sprite-Compiler) repository if something fails.

## Next steps

- [Usage guide](/tools/sprite-compiler/usage-guide)
- [Advanced features](/tools/sprite-compiler/advanced-features)

## See also

- [Overview](/tools/sprite-compiler/overview)
