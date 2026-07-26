#!/usr/bin/env python3
"""Build modular_size_bench A/B configs and report Flash/RAM deltas (ESP32)."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

BENCH_DIR = Path(__file__).resolve().parent / "modular_size_bench"
OUT_JSON = Path(__file__).resolve().parent / "modular_size_results.json"

ENVS = [
    "measure_full",
    "measure_no_audio",
    "measure_no_physics",
    "measure_no_ui",
    "measure_no_particles",
    "measure_all_off",
]

# GNU size output: text / data / bss / dec / hex / filename
SIZE_GNU_RE = re.compile(
    r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+[0-9a-fA-F]+\s+\S+",
    re.M,
)
SIZE_RAM_FLASH_RE = re.compile(
    r"RAM:.*?used\s+(\d+)\s+bytes.*?Flash:.*?used\s+(\d+)\s+bytes",
    re.S,
)


def run_pio(env: str) -> str:
    cmd = ["pio", "run", "-e", env, "-t", "size"]
    print(f"\n=== Building {env} ===", flush=True)
    proc = subprocess.run(
        cmd,
        cwd=str(BENCH_DIR),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    out = (proc.stdout or "") + "\n" + (proc.stderr or "")
    if proc.returncode != 0:
        print(out[-8000:])
        raise RuntimeError(f"pio failed for {env} (exit {proc.returncode})")
    return out


def parse_size(output: str) -> tuple[int, int, dict[str, int]]:
    """Return (ram_bytes, flash_bytes, details). RAM=data+bss, Flash=text+data."""
    m = SIZE_RAM_FLASH_RE.search(output)
    if m:
        ram = int(m.group(1))
        flash = int(m.group(2))
        return ram, flash, {"ram": ram, "flash": flash}

    matches = SIZE_GNU_RE.findall(output)
    if matches:
        # Prefer the firmware.elf line if present; else last numeric row.
        chosen = matches[-1]
        for row in matches:
            # filename is not captured; use last match after "Calculating size"
            pass
        text, data, bss, dec = (int(x) for x in chosen)
        # Re-scan with filename to prefer firmware.elf
        for m2 in re.finditer(
            r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+[0-9a-fA-F]+\s+(\S+)",
            output,
            re.M,
        ):
            if m2.group(5).endswith("firmware.elf"):
                text, data, bss, dec = (int(m2.group(i)) for i in range(1, 5))
                break
        ram = data + bss
        flash = text + data
        return ram, flash, {"text": text, "data": data, "bss": bss, "dec": dec}

    raise RuntimeError("Could not parse size output from pio:\n" + output[-4000:])


def run_sizeof_probe() -> dict[str, int]:
    print("\n=== Sizeof probe (expect compile errors with sizes) ===", flush=True)
    proc = subprocess.run(
        ["pio", "run", "-e", "measure_sizeof_probe"],
        cwd=str(BENCH_DIR),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    out = (proc.stdout or "") + "\n" + (proc.stderr or "")
    sizes: dict[str, int] = {}
    for name, pattern in [
        ("ESP32AudioScheduler", r"PixelRoot32SizeofProbe<(\d+)>.*audioSchedulerSize|incomplete type.*PixelRoot32SizeofProbe<(\d+)>"),
        ("CollisionSystem", r"PixelRoot32SizeofProbe<(\d+)>"),
    ]:
        pass
    # Collect all incomplete-type sizes in order of appearance in forceSizeofProbe
    found = re.findall(r"PixelRoot32SizeofProbe<(\d+)>", out)
    labels = [
        "ESP32AudioScheduler",
        "CollisionSystem",
        "ParticleEmitter",
        "UIManager",
        "UILabel",
        "UIButton",
    ]
    for label, value in zip(labels, found):
        sizes[label] = int(value)
        print(f"  sizeof({label}) = {value}")
    if "ESP32AudioScheduler" not in sizes:
        print(out[-6000:])
        print("WARNING: sizeof probe did not yield ESP32AudioScheduler", file=sys.stderr)
    return sizes


def kb(n: int) -> float:
    return round(n / 1024.0, 2)


def round_display_kb(bytes_val: int) -> str:
    """Landing-friendly ~N KB (nearest KB, minimum 1 if > 256 B)."""
    if bytes_val <= 0:
        return "~0 KB"
    nearest = int(round(bytes_val / 1024.0))
    if nearest == 0:
        nearest = 1
    return f"~{nearest} KB"


def main() -> int:
    results: dict[str, dict] = {}
    for env in ENVS:
        out = run_pio(env)
        ram, flash, details = parse_size(out)
        results[env] = {"ram_bytes": ram, "flash_bytes": flash, "details": details}
        print(f"{env}: RAM={ram} ({kb(ram)} KB)  Flash={flash} ({kb(flash)} KB)  {details}")

    sizeofs = run_sizeof_probe()

    full = results["measure_full"]
    deltas = {}
    mapping = {
        "audio": "measure_no_audio",
        "physics": "measure_no_physics",
        "ui": "measure_no_ui",
        "particles": "measure_no_particles",
        "all": "measure_all_off",
    }
    for key, env in mapping.items():
        r = results[env]
        static_ram = full["ram_bytes"] - r["ram_bytes"]
        flash = full["flash_bytes"] - r["flash_bytes"]
        # Audio scheduler (incl. ApuCore) is heap-allocated in Engine ctor; add to RAM.
        ram_total = static_ram
        if key == "audio" and "ESP32AudioScheduler" in sizeofs:
            ram_total += sizeofs["ESP32AudioScheduler"]
        # FreeRTOS audio task stack is created by I2S/DAC backend when audio starts.
        # Constant from ESP32_*_AudioBackend.cpp (4096 words? or bytes?) — verified in probe notes.
        if key == "audio":
            # xTaskCreatePinnedToCore stack depth is in words on ESP32? Actually Arduino-ESP32 uses bytes.
            # Docs in backend use 4096 — treat as bytes unless map says otherwise.
            pass
        deltas[key] = {
            "ram_static_bytes": static_ram,
            "ram_bytes": ram_total,
            "flash_bytes": flash,
            "ram_display": round_display_kb(ram_total),
            "flash_display": round_display_kb(flash),
        }

    # all_off audio heap too
    if "ESP32AudioScheduler" in sizeofs:
        deltas["all"]["ram_bytes"] = deltas["all"]["ram_static_bytes"] + sizeofs["ESP32AudioScheduler"]
        deltas["all"]["ram_display"] = round_display_kb(deltas["all"]["ram_bytes"])

    payload = {
        "target": "esp32dev",
        "resolution": "128x128",
        "methodology": {
            "flash": "pio size delta: measure_full vs flag-disabled build (code linked via BenchScene refs)",
            "ram_static": "pio size .data+.bss delta",
            "ram_audio_heap": "sizeof(ESP32AudioScheduler) always allocated when AUDIO=1",
            "note": "ESP32 base_esp32 uses reduced physics grid (4/4/64). Particles RAM includes 1 ParticleEmitter member pool.",
        },
        "builds": results,
        "sizeofs": sizeofs,
        "deltas": deltas,
    }

    OUT_JSON.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print("\n=== DELTAS (full - disabled) ===")
    for key, d in deltas.items():
        print(
            f"{key:10} RAM {d['ram_display']:>8} ({d['ram_bytes']} B)  "
            f"Flash {d['flash_display']:>8} ({d['flash_bytes']} B)"
        )
    print(f"\nWrote {OUT_JSON}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
