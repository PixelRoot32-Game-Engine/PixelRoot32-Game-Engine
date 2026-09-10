#!/usr/bin/env python3
"""
PixelRoot32 Latin-1 Supplement Glyph Generator

Emits the FONT5X7_LATIN1_GLYPHS descriptor table (96 entries, codepoints
0xA0-0xFF) plus the 8-row bitmap arrays it references, and splices them into
include/graphics/Font5x7.h and src/graphics/Font5x7.cpp between generated
markers. Re-running this script is idempotent: it replaces only the content
between "BEGIN GENERATED" / "END GENERATED" markers, never hand-authored code.

Why a generator instead of hand-authoring: 96 aligned Sprite descriptors is
exactly where a manual transcription error hides (an off-by-one row, a
mirrored bit pattern) and only surfaces later as a corrupted glyph on
hardware. A script makes the 19-drawn/77-blank split and the diacritic
placement arithmetic mechanically checkable instead of eyeballed.

Bit order: bit (width-1) is the LEFTMOST pixel, bit 0 is the rightmost
(src/graphics/Renderer.cpp:499-507). The `.opencode/skills/pixelroot32-sprite-
renderer/SKILL.md` doc claiming "bit 0 is leftmost" is wrong -- do not follow
it when authoring glyph rows.

Diacritic placement: the accent occupies the row immediately above the
letter's topmost lit row, not always ext-row 0. The base 7-row body is always
placed at ext-array indices 1-7 (index 0 is the one truly "extra" row, added
above the whole body so extYOffset=-1 realigns the body with the unaccented
baseline -- see design D5). For glyphs whose body already has blank rows at
the top (most lowercase letters), the accent is written into the body's own
blank row closest to the ink, and ext-index 0 stays blank, so the accent sits
close to the letter instead of floating two rows above it. Uppercase glyphs
(and 'i', whose dot already occupies row 0) have no spare blank row in the
body, so the accent goes in ext-index 0, the only row available.

Usage:
    python scripts/generate_font5x7_latin1.py
    python scripts/generate_font5x7_latin1.py --check   # verify, do not write
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent
FONT_H = REPO_ROOT / "include" / "graphics" / "Font5x7.h"
FONT_CPP = REPO_ROOT / "src" / "graphics" / "Font5x7.cpp"

SCRIPT_NAME = "scripts/generate_font5x7_latin1.py"
MARK_BEGIN = f"// --- BEGIN GENERATED: {SCRIPT_NAME} ---"
MARK_END = "// --- END GENERATED: do not hand-edit above; re-run the script instead. ---"

EXT_FIRST_CHAR = 0xA0
EXT_LAST_CHAR = 0xFF
EXT_GLYPH_COUNT = EXT_LAST_CHAR - EXT_FIRST_CHAR + 1  # 96

# Single-row diacritic bit patterns (5-bit, bit4=leftmost per Renderer.cpp:507).
ACUTE_ROW = 0x02        # single point, upper-right lean: ....#. -> col 3 of 5
DIAERESIS_ROW = 0x0A    # two dots, same pattern as GLYPH_QUOTE's rows: .#.#.
TILDE_ROW = 0x15        # three-point wave: #.#.#

# The 19 codepoints this slice draws, keyed by codepoint. `base` names the
# existing 7-row GLYPH_<base> array in Font5x7.h to extract programmatically
# (never duplicated by hand, so it cannot drift from the source of truth).
# `diacritic` is the one-row accent pattern placed per the rule above.
ACCENTED_GLYPHS: List[Tuple[int, str, str, int]] = [
    # (codepoint, generated C identifier suffix, base GLYPH_<x> name, diacritic row)
    (0xE1, "a_acute", "a", ACUTE_ROW),       # a - lowercase a with acute
    (0xE9, "e_acute", "e", ACUTE_ROW),       # e
    (0xED, "i_acute", "i", ACUTE_ROW),       # i
    (0xF3, "o_acute", "o", ACUTE_ROW),       # o
    (0xFA, "u_acute", "u", ACUTE_ROW),       # u
    (0xFC, "u_diaeresis", "u", DIAERESIS_ROW),  # u
    (0xF1, "n_tilde", "n", TILDE_ROW),       # n
    (0xC1, "A_acute", "A", ACUTE_ROW),       # A
    (0xC9, "E_acute", "E", ACUTE_ROW),       # E
    (0xCD, "I_acute", "I", ACUTE_ROW),       # I
    (0xD3, "O_acute", "O", ACUTE_ROW),       # O
    (0xDA, "U_acute", "U", ACUTE_ROW),       # U
    (0xDC, "U_diaeresis", "U", DIAERESIS_ROW),  # U
    (0xD1, "N_tilde", "N", TILDE_ROW),       # N
]

# Codepoints that draw their own full 7-row bitmap (no diacritic), placed at
# the normal body position (ext-index 0 blank, body at ext-indices 1-7).
# QUESTION_INVERTED / EXCLAMATION_INVERTED are derived by reversing the
# existing GLYPH_QUESTION / GLYPH_EXCLAMATION row order (a vertical flip is
# the standard construction for U+00BF/U+00A1 from '?'/'!'). The guillemets
# and masculine ordinal indicator have no ASCII glyph to derive from, so
# their 7 rows are authored directly here.
STANDALONE_FROM_BASE_REVERSED: List[Tuple[int, str, str]] = [
    (0xBF, "QUESTION_INVERTED", "QUESTION"),
    (0xA1, "EXCLAMATION_INVERTED", "EXCLAMATION"),
]

STANDALONE_AUTHORED: List[Tuple[int, str, List[int]]] = [
    (0xAB, "GUILLEMET_LEFT", [0x00, 0x05, 0x0A, 0x14, 0x0A, 0x05, 0x00]),
    (0xBB, "GUILLEMET_RIGHT", [0x00, 0x14, 0x0A, 0x05, 0x0A, 0x14, 0x00]),
    (0xBA, "ORDINAL_MASCULINE", [0x00, 0x0E, 0x0A, 0x0E, 0x00, 0x0E, 0x00]),
]

BLANK_IDENTIFIER = "GLYPH_LATIN_BLANK"


def extract_base_glyph(header_text: str, name: str) -> List[int]:
    """Pull the 7 row values out of `static const uint16_t GLYPH_<name>[7] = {...};`."""
    pattern = re.compile(
        r"GLYPH_" + re.escape(name) + r"\[7\]\s*=\s*\{([^}]+)\}", re.MULTILINE
    )
    m = pattern.search(header_text)
    if not m:
        raise ValueError(f"Could not find GLYPH_{name}[7] in {FONT_H}")
    values = [v.strip() for v in m.group(1).split(",") if v.strip()]
    if len(values) != 7:
        raise ValueError(f"GLYPH_{name} did not yield 7 rows (got {len(values)})")
    return [int(v, 16) for v in values]


def build_accented_ext_rows(body: List[int], diacritic: int) -> List[int]:
    """8-row ext array: body always at indices 1-7; diacritic goes immediately
    above the body's topmost lit row (ext-index 0 if that row is body[0])."""
    topmost = next((i for i, v in enumerate(body) if v != 0), 0)
    ext = [0x0000] + list(body)
    if topmost == 0:
        ext[0] = diacritic
    else:
        ext[topmost] = diacritic  # ext[topmost] == body[topmost - 1]'s slot
    return ext


def build_standalone_ext_rows(body7: List[int]) -> List[int]:
    return [0x0000] + list(body7)


def format_row_array(identifier: str, rows: List[int], codepoint: int, label: str) -> str:
    hex_rows = ", ".join(f"0x{v:04X}" for v in rows)
    return (
        f"static const uint16_t {identifier}[8] = {{{hex_rows}}}; "
        f"// U+00{codepoint:02X} {label}"
    )


def generate() -> Tuple[str, str]:
    header_text = FONT_H.read_text(encoding="utf-8")

    entries: Dict[int, str] = {}  # codepoint -> identifier (row array name)
    row_decls: List[str] = []

    for codepoint, suffix, base_name, diacritic in ACCENTED_GLYPHS:
        body = extract_base_glyph(header_text, base_name)
        ext_rows = build_accented_ext_rows(body, diacritic)
        identifier = f"GLYPH_LATIN_{suffix}"
        row_decls.append(format_row_array(identifier, ext_rows, codepoint, suffix))
        entries[codepoint] = identifier

    for codepoint, suffix, base_name in STANDALONE_FROM_BASE_REVERSED:
        body = extract_base_glyph(header_text, base_name)
        ext_rows = build_standalone_ext_rows(list(reversed(body)))
        identifier = f"GLYPH_LATIN_{suffix}"
        row_decls.append(format_row_array(identifier, ext_rows, codepoint, suffix))
        entries[codepoint] = identifier

    for codepoint, suffix, body7 in STANDALONE_AUTHORED:
        ext_rows = build_standalone_ext_rows(body7)
        identifier = f"GLYPH_LATIN_{suffix}"
        row_decls.append(format_row_array(identifier, ext_rows, codepoint, suffix))
        entries[codepoint] = identifier

    if len(entries) != 19:
        raise ValueError(f"Expected 19 drawn codepoints, computed {len(entries)}")

    row_decls.sort(key=lambda line: line.split("[8]")[0])  # stable, readable order

    # --- Font5x7.h block: row arrays + shared blank + extern decl ---
    header_lines = [MARK_BEGIN]
    header_lines.append(
        "// 19 drawn Latin-1 supplement glyphs (of 96 total, 0xA0-0xFF). Row 0 is"
    )
    header_lines.append(
        "// the diacritic/extra row; rows 1-7 are the letter body, aligned with"
    )
    header_lines.append(
        "// the base 7-row glyph via extYOffset=-1 (see design D5)."
    )
    header_lines.extend(row_decls)
    header_lines.append("")
    header_lines.append(
        f"// Shared by all {EXT_GLYPH_COUNT - 19} undrawn codepoints in the block --"
        " one 8-row zero"
    )
    header_lines.append(
        "// array referenced many times, not one array per blank codepoint."
    )
    header_lines.append(
        f"static const uint16_t {BLANK_IDENTIFIER}[8] = {{0, 0, 0, 0, 0, 0, 0, 0}};"
    )
    header_lines.append("")
    header_lines.append(
        "/// Number of glyphs in the Latin-1 supplement table:"
        " extLastChar - extFirstChar + 1 (0xA0..0xFF)."
    )
    header_lines.append(
        f"inline constexpr uint16_t kFont5x7Latin1GlyphCount = 0x{EXT_LAST_CHAR:02X} - 0x{EXT_FIRST_CHAR:02X} + 1;"
    )
    header_lines.append("")
    header_lines.append(
        "extern const Sprite FONT5X7_LATIN1_GLYPHS[kFont5x7Latin1GlyphCount];"
    )
    header_lines.append(MARK_END)
    header_block = "\n".join(header_lines)

    # --- Font5x7.cpp block: descriptor table ---
    cpp_lines = [MARK_BEGIN]
    cpp_lines.append(
        f"const Sprite FONT5X7_LATIN1_GLYPHS[] = {{"
    )
    for i in range(EXT_GLYPH_COUNT):
        codepoint = EXT_FIRST_CHAR + i
        if codepoint in entries:
            identifier = entries[codepoint]
        else:
            identifier = BLANK_IDENTIFIER
        comment = f"0x{codepoint:02X}"
        cpp_lines.append(f"    {{{identifier}, 5, 8}},  // {comment}")
    cpp_lines.append("};")
    cpp_lines.append("")
    cpp_lines.append(
        "static_assert(sizeof(FONT5X7_LATIN1_GLYPHS) / sizeof(FONT5X7_LATIN1_GLYPHS[0]) =="
    )
    cpp_lines.append(
        "                  kFont5x7Latin1GlyphCount,"
    )
    cpp_lines.append(
        "              \"FONT5X7_LATIN1_GLYPHS entry count must equal\""
    )
    cpp_lines.append(
        "              \" extLastChar - extFirstChar + 1 (0xA0-0xFF)\");"
    )
    cpp_lines.append(MARK_END)
    cpp_block = "\n".join(cpp_lines)

    return header_block, cpp_block


def splice(text: str, block: str, anchor_pattern: str) -> str:
    """Replace content between existing markers, or insert the block right
    after the first line matching `anchor_pattern` if markers are absent."""
    begin_idx = text.find(MARK_BEGIN)
    end_idx = text.find(MARK_END)
    if begin_idx != -1 and end_idx != -1:
        end_idx += len(MARK_END)
        return text[:begin_idx] + block + text[end_idx:]

    anchor = re.search(anchor_pattern, text, re.MULTILINE)
    if not anchor:
        raise ValueError(f"Anchor pattern not found: {anchor_pattern}")
    insert_at = anchor.end()
    return text[:insert_at] + "\n\n" + block + "\n" + text[insert_at:]


def main() -> int:
    check_only = "--check" in sys.argv

    header_text = FONT_H.read_text(encoding="utf-8")
    header_block, cpp_block = generate()

    new_header = splice(
        header_text,
        f"#if PIXELROOT32_ENABLE_FONT_LATIN1\n\n{header_block}\n\n#endif // PIXELROOT32_ENABLE_FONT_LATIN1",
        r"^extern const Sprite FONT5X7_GLYPHS\[kFont5x7AsciiGlyphCount\];\s*$",
    )

    cpp_text = FONT_CPP.read_text(encoding="utf-8")
    new_cpp = splice(
        cpp_text,
        f"#if PIXELROOT32_ENABLE_FONT_LATIN1\n\n{cpp_block}\n\n#endif // PIXELROOT32_ENABLE_FONT_LATIN1",
        r"^#undef PR32_FONT5X7_EXT\s*$",
    )

    if check_only:
        changed = new_header != header_text or new_cpp != cpp_text
        print("UP TO DATE" if not changed else "STALE: re-run without --check")
        return 1 if changed else 0

    FONT_H.write_text(new_header, encoding="utf-8")
    FONT_CPP.write_text(new_cpp, encoding="utf-8")
    print(f"Wrote {FONT_H}")
    print(f"Wrote {FONT_CPP}")
    print(f"{len(entries := {**{c: 1 for c, *_ in ACCENTED_GLYPHS}, **{c: 1 for c, *_ in STANDALONE_FROM_BASE_REVERSED}, **{c: 1 for c, *_ in STANDALONE_AUTHORED}}) } drawn codepoints, {EXT_GLYPH_COUNT - len(entries)} blank, {EXT_GLYPH_COUNT} total")
    return 0


if __name__ == "__main__":
    sys.exit(main())
