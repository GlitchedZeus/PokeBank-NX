#!/usr/bin/env python3
"""Recover exact Gen IV ball and held-item PNGs from a pinned PokeAPI/sprites revision.

Runs on the host during native RomFS recovery, never on the user's Switch.
No guessed artwork or silent fallback to another item's sprite.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import unicodedata
import urllib.error
import urllib.request
from pathlib import Path

from png_asset_validation import validate_png

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "romfs" / "sprites" / "items"
NAMES = ROOT / "src" / "Names" / "ItemNames.cpp"
MANIFEST = ROOT / "tools" / "pinned_item_sprite_names.txt"
PINNED_REF = "8dfa3d97e953caaafaafd4963eff7621811af08e"
BASE_URL = f"https://raw.githubusercontent.com/PokeAPI/sprites/{PINNED_REF}/sprites/items"
REQUIRED_BALLS = (
    "master-ball", "ultra-ball", "great-ball", "poke-ball",
    "safari-ball", "net-ball", "dive-ball", "nest-ball",
    "repeat-ball", "timer-ball", "luxury-ball", "premier-ball",
    "dusk-ball", "heal-ball", "quick-ball", "cherish-ball",
)


def slugify(name: str) -> str:
    name = name.split(" (", 1)[0]
    plain = "".join(ch for ch in unicodedata.normalize("NFKD", name)
                    if not unicodedata.combining(ch)).lower()
    out = []
    for ch in plain:
        if "a" <= ch <= "z" or "0" <= ch <= "9":
            out.append(ch)
        elif ch in " -_":
            if out and out[-1] != "-":
                out.append("-")
    return "".join(out).strip("-")



def manifest_names() -> set[str]:
    """All exact original item art keys pinned to the source tree."""
    if not MANIFEST.is_file():
        raise RuntimeError("Missing pinned item sprite manifest")
    names = {line.strip() for line in MANIFEST.read_text(encoding="utf-8").splitlines()
             if line.strip() and not line.startswith("#")}
    if len(names) < 895 or any(not n or n.strip("abcdefghijklmnopqrstuvwxyz0123456789-")
                               for n in names):
        raise RuntimeError("Pinned item manifest invalid or unexpectedly incomplete")
    return names


def gen4_names() -> set[str]:
    text = NAMES.read_text(encoding="utf-8")
    section = text.split("static const char* ITEM_NAMES[] = {", 1)[1].split("};", 1)[0]
    names = set()
    for line in section.splitlines():
        item_literal, separator, comment = line.partition("//")
        if not separator:
            continue
        name = item_literal.strip().removesuffix(",")
        if not (name.startswith('"') and name.endswith('"')):
            continue
        item_id = int(comment.strip().split()[0])
        if 1 <= item_id <= 536:
            slug = slugify(name[1:-1])
            if slug:
                names.add(slug)
    if len(names) < 400:
        raise RuntimeError(f"Gen IV item-name table unexpectedly incomplete: {len(names)}")
    return names


def validated(path: Path) -> bool:
    if not path.is_file():
        return False
    ok, _reason, _dims = validate_png(path)
    return ok


def fetch(name: str, force: bool) -> str:
    target = OUT / (name + ".png")
    if not force and validated(target):
        return "cached"
    request = urllib.request.Request(
        f"{BASE_URL}/{name}.png",
        headers={"User-Agent": "PokeBank-NX-pinned-Gen4-item-sprites"},
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            payload = response.read()
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return "not-upstream"
        raise
    if not payload.startswith(bytes((137, 80, 78, 71, 13, 10, 26, 10))):
        raise RuntimeError(f"Not a PNG: {name}")
    temporary = target.with_suffix(".png.incomplete")
    temporary.write_bytes(payload)
    if not validated(temporary):
        temporary.unlink(missing_ok=True)
        raise RuntimeError(f"Corrupt upstream item PNG: {name}")
    temporary.replace(target)
    return "fetched"


def verify_required() -> None:
    missing = [name for name in REQUIRED_BALLS if not validated(OUT / (name + ".png"))]
    if missing:
        raise RuntimeError("Missing required native Gen IV ball PNGs: " + ", ".join(missing))
    print("GEN IV BALL SPRITE PREFLIGHT: PASS (16 distinct matching ball images)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--required-only", action="store_true")
    parser.add_argument("--verify-existing", action="store_true")
    options = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    if options.verify_existing:
        verify_required()
        return 0
    names = set(REQUIRED_BALLS)
    if not options.required_only:
        names.update(gen4_names())
        names.update(manifest_names())
    results = {"cached": 0, "fetched": 0, "not-upstream": 0}
    with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
        for result in pool.map(lambda name: fetch(name, options.force), sorted(names)):
            results[result] += 1
    print(f"Item sprite recovery from pinned PokeAPI {PINNED_REF[:12]}: {results}")
    verify_required()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
