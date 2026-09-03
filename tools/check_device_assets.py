#!/usr/bin/env python3
"""Fail-loud preflight for PokeBank NX device-test/release assets.

This check is intentionally offline and stdlib-only. It does not download or modify anything.
Run tools/gen_hdsprites.py first when HD Pokémon renders are missing.

Why this exists:
    romfs/ is generated locally and gitignored. A clean checkout can therefore build a valid .nro
    while silently omitting the HD Pokémon renders used by View Pokémon / Summary. Device-test and
    release builds must prove the expected local RomFS assets exist before packaging.

Run:
    python tools/check_device_assets.py

Exit status:
    0 = required device-build assets look present
    1 = one or more required asset checks failed
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROMFS = ROOT / "romfs"
HD_DIR = ROMFS / "sprites" / "pokemon_hd"
TYPE_DIR = ROMFS / "sprites" / "types"
FONT_DIR = ROMFS / "fonts"
GENERATOR = ROOT / "tools" / "gen_hdsprites.py"

DEX_MAX = 1025
REQUIRED_FONTS = (
    "Nunito.ttf",
    "NotoSansSymbols.ttf",
    "NotoSansSymbols2.ttf",
)


def fail(errors: list[str], message: str) -> None:
    errors.append(message)
    print(f"FAIL  {message}")


def ok(message: str) -> None:
    print(f"PASS  {message}")


def pinned_sprite_ref() -> str:
    try:
        text = GENERATOR.read_text(encoding="utf-8")
    except OSError:
        return "unknown"
    match = re.search(r'^PINNED_REF\s*=\s*[\"\']([^\"\']+)[\"\']', text, re.MULTILINE)
    return match.group(1) if match else "unknown"


def main() -> int:
    errors: list[str] = []
    print("PokeBank NX device-build asset preflight")
    print(f"repo: {ROOT}")
    print(f"PokeAPI sprite ref: {pinned_sprite_ref()}")
    print()

    if not ROMFS.is_dir():
        fail(errors, "romfs/ is missing")
    else:
        ok("romfs/ exists")

    if not HD_DIR.is_dir():
        fail(errors, "romfs/sprites/pokemon_hd/ is missing; run: python tools/gen_hdsprites.py")
    else:
        pngs = [p for p in HD_DIR.iterdir() if p.is_file() and p.suffix.lower() == ".png"]
        total_bytes = sum(p.stat().st_size for p in pngs)
        ok(f"HD sprite directory exists ({len(pngs)} PNGs, {total_bytes / (1024 * 1024):.1f} MiB)")

        names = {p.name for p in pngs}
        missing_base = [n for n in range(1, DEX_MAX + 1) if f"{n}.png" not in names]
        if missing_base:
            preview = ", ".join(str(n) for n in missing_base[:20])
            suffix = " ..." if len(missing_base) > 20 else ""
            fail(errors, f"{len(missing_base)} base-species HOME renders missing: {preview}{suffix}")
        else:
            ok(f"all {DEX_MAX} base-species HOME renders are present")

        # Representative capability checks. These catch a common partial-copy failure where only
        # base normals were staged into romfs and shiny/form assets were dropped.
        if any(re.fullmatch(r"\d+s\.png", name) for name in names):
            ok("at least one shiny HOME render is present")
        else:
            fail(errors, "no shiny HOME renders found")

        alt_numeric = []
        for name in names:
            match = re.fullmatch(r"(\d+)(?:s|f|fs)?\.png", name)
            if match and int(match.group(1)) > 10000:
                alt_numeric.append(name)
        if alt_numeric:
            ok("alternate-form HOME render IDs are present")
        else:
            fail(errors, "no numeric alternate-form HOME render IDs (>10000) found")

        for sample in ("1.png", "25.png"):
            if sample in names:
                ok(f"representative render {sample} exists")
            else:
                fail(errors, f"representative render {sample} is missing")

    if not TYPE_DIR.is_dir():
        fail(errors, "romfs/sprites/types/ is missing; run: make types")
    else:
        missing_types = [i for i in range(18) if not (TYPE_DIR / f"{i}.png").is_file()]
        if missing_types:
            fail(errors, f"type sprites missing for internal IDs: {missing_types}")
        else:
            ok("all 18 type sprites are present")

    if not FONT_DIR.is_dir():
        fail(errors, "romfs/fonts/ is missing; run: make fonts")
    else:
        missing_fonts = [name for name in REQUIRED_FONTS if not (FONT_DIR / name).is_file()]
        if missing_fonts:
            fail(errors, f"required UI fonts missing: {', '.join(missing_fonts)}")
        else:
            ok("required UI fonts are present")

    print()
    if errors:
        print(f"DEVICE ASSET PREFLIGHT: FAIL ({len(errors)} problem(s))")
        print("Do not hand off a visual-acceptance/device-test .nro until this passes.")
        return 1

    print("DEVICE ASSET PREFLIGHT: PASS")
    print("This proves required local source assets exist before packaging; it does not by itself prove")
    print("the final .nro contains or renders them. Verify the built artifact on-device as well.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
