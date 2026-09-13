#!/usr/bin/env python3
"""Fail-loud preflight for PokeBank NX device-test/release assets.

This check is intentionally offline and stdlib-only. It does not download or modify anything.
Run tools/gen_hdsprites.py first when HD Pokémon renders are missing.

Cleanup #3 additionally proves the exact Gen I Normal/Shiny preview contract used by SpriteManager:
base-form species IDs 001-151, HOME render first, `s` suffix for shiny, lower-resolution fallback only.

Run:
    python tools/check_device_assets.py

Exit status:
    0 = required device-build assets look present
    1 = one or more required asset checks failed
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROMFS = ROOT / "romfs"
HD_DIR = ROMFS / "sprites" / "pokemon_hd"
TYPE_DIR = ROMFS / "sprites" / "types"
FONT_DIR = ROMFS / "fonts"
GAME_CARD_ART_DIR = ROMFS / "game_cards"
GENERATOR = ROOT / "tools" / "gen_hdsprites.py"
SPRITE_MANAGER = ROOT / "src" / "UI" / "SpriteManager.cpp"
SPECIES_NAMES_CPP = ROOT / "src" / "Names" / "SpeciesNames.cpp"

DEX_MAX = 1025
GEN1_DEX_MAX = 151
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


def gen1_species_names() -> list[str]:
    """Read the same generated species-name table the C++ UI uses; no second hand-written list."""
    try:
        text = SPECIES_NAMES_CPP.read_text(encoding="utf-8")
        marker = "static const char* const SPECIES_NAMES[] = {"
        block = text.split(marker, 1)[1].split("};", 1)[0]
        names = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', block)
        if len(names) > GEN1_DEX_MAX:
            return names
    except (OSError, IndexError):
        pass
    return ["None"] + [f"Species {n}" for n in range(1, GEN1_DEX_MAX + 1)]


def verify_runtime_sprite_contract(errors: list[str]) -> None:
    """Prove the preflight's filename expectations still match SpriteManager's live path contract."""
    try:
        text = SPRITE_MANAGER.read_text(encoding="utf-8")
    except OSError:
        fail(errors, "src/UI/SpriteManager.cpp is missing; cannot verify runtime sprite mapping")
        return

    required = (
        'std::string suffix = isShiny ? "s" : "";',
        'Pokemon::getFormSpriteId(speciesId, formId)',
        'loadSprite("sprites/pokemon_hd/" + sid + suffix + ".png")',
        'loadSprite("sprites/pokemon/" + sid + suffix + ".png")',
    )
    missing = [needle for needle in required if needle not in text]
    if missing:
        fail(errors, "SpriteManager runtime mapping changed; update asset preflight with the runtime: " + "; ".join(missing))
    else:
        ok("SpriteManager HOME-first normal/shiny path + defensive lower-resolution fallback contract matches preflight")

    lowered = text.lower()
    suspicious = [token for token in ("hue_shift", "hueshift", "hue-rotate", "huerotate") if token in lowered]
    if suspicious:
        fail(errors, "runtime hue-filter shiny substitute detected; Cleanup #3 requires real bundled shiny artwork")
    else:
        ok("no runtime hue-filter/generated shiny substitute detected")


def format_missing(ids: list[int], names: list[str]) -> str:
    shown = ids[:20]
    labels = []
    for n in shown:
        name = names[n] if n < len(names) else f"Species {n}"
        labels.append(f"{n:03d} {name}")
    return ", ".join(labels) + (" ..." if len(ids) > len(shown) else "")


def main() -> int:
    errors: list[str] = []
    names = gen1_species_names()
    print("PokeBank NX device-build asset preflight")
    print(f"repo: {ROOT}")
    print(f"PokeAPI sprite ref: {pinned_sprite_ref()}")
    print()

    verify_runtime_sprite_contract(errors)

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

        file_names = {p.name for p in pngs}
        missing_base = [n for n in range(1, DEX_MAX + 1) if f"{n}.png" not in file_names]
        if missing_base:
            preview = ", ".join(str(n) for n in missing_base[:20])
            suffix = " ..." if len(missing_base) > 20 else ""
            fail(errors, f"{len(missing_base)} base-species HOME renders missing: {preview}{suffix}")
        else:
            ok(f"all {DEX_MAX} base-species HOME renders are present")

        # Cleanup #3 intentionally supports base-form Gen I species 001-151. The runtime's base-form
        # key is the National Dex species ID and shiny appends `s`; validate BOTH paths explicitly.
        missing_gen1_normal = [n for n in range(1, GEN1_DEX_MAX + 1) if f"{n}.png" not in file_names]
        missing_gen1_shiny = [n for n in range(1, GEN1_DEX_MAX + 1) if f"{n}s.png" not in file_names]
        if missing_gen1_normal:
            fail(errors, f"{len(missing_gen1_normal)} Gen I normal HOME renders missing: {format_missing(missing_gen1_normal, names)}")
        else:
            ok(f"all {GEN1_DEX_MAX} Gen I normal HOME renders are present")
        if missing_gen1_shiny:
            fail(errors, f"{len(missing_gen1_shiny)} Gen I shiny HOME renders missing: {format_missing(missing_gen1_shiny, names)}")
        else:
            ok(f"all {GEN1_DEX_MAX} Gen I shiny HOME renders are present")

        alt_numeric = []
        for name in file_names:
            match = re.fullmatch(r"(\d+)(?:s|f|fs)?\.png", name)
            if match and int(match.group(1)) > 10000:
                alt_numeric.append(name)
        if alt_numeric:
            ok("alternate-form HOME render IDs are present")
        else:
            fail(errors, "no numeric alternate-form HOME render IDs (>10000) found")

        for sample in ("1.png", "25.png", "151.png", "1s.png", "25s.png", "151s.png"):
            if sample in file_names:
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

    required_game_cards = (
        "red_gb.png", "blue_gb.png", "yellow_gb.png",
        "gold_gbc.png", "silver_gbc.png", "crystal_gbc.png",
        "firered_gba.png", "leafgreen_gba.png",
        "ruby_gba.png", "sapphire_gba.png", "emerald_gba.png",
    )
    if not GAME_CARD_ART_DIR.is_dir():
        fail(errors, "romfs/game_cards/ is missing; run: make game-card-art")
    else:
        missing_cards = [name for name in required_game_cards if not (GAME_CARD_ART_DIR / name).is_file()]
        if missing_cards:
            fail(errors, f"required game-card artwork missing: {', '.join(missing_cards)}")
        else:
            ok("Red/Blue/Yellow GB + Gold/Silver/Crystal GBC + FireRed/LeafGreen/Ruby/Sapphire/Emerald GBA game-card artwork is present")

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
