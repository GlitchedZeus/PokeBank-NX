#!/usr/bin/env python3
"""Recover original ball and held-item PNGs from PokeAPI/sprites pinned to the
same commit as the app's HOME renders.

ROMFS result: romfs/sprites/items/<canonical-item-name>.png

No artwork is guessed or fabricated. An absent upstream icon stays absent,
while all 16 native Gen IV ball sprites are a strict required baseline. This
tool runs at BUILD/RECOVERY time, never on the user's Switch. All downloaded
assets remain generated/gitignored, like the existing Pokémon renders.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import re
import unicodedata
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "romfs" / "sprites" / "items"
NAMES = ROOT / "src" / "Names" / "ItemNames.cpp"
PINNED_REF = "8dfa3d97e953caaafaafd4963eff7621811af08e"
BASE_URL = f"https://raw.githubusercontent.com/PokeAPI/sprites/{PINNED_REF}/sprites/items"
REQUIRED_BALLS = (
    "master-ball", "ultra-ball", "great-ball", "poke-ball",
    "safari-ball", "net-ball", "dive-ball", "nest-ball",
    "repeat-ball", "timer-ball", "luxury-ball", "premier-ball",
    "dusk-ball", "heal-ball", "quick-ball", "cherish-ball",
)


def slugify(name: str) -> str:
    # Mirrors SpriteManager::getItemSprite: remove punctuation, collapse
    # whitespace/hyphens, normalize 'Poké' to 'poke'.
    name = name.split(" (", 1)[0]
    name = "".join(c for c in unicodedata.normalize("NFKD", name)
                   if not unicodedata.combining(c))
    name = name.lower()
    name = re.sub(r"[^a-z0-9\s_-]", "", name)
    return re.sub(r"-+", "-", re.sub(r"[\s_-]+", "-", name)).strip("-")


def gen4_names() -> set[str]:
    source = NAMES.read_text(encoding="utf-8")
    pattern = re.compile(r'^\s*"([^"\\]*(?:\\.[^"\\]*)*)",\s*//\s*(\d+)\s*$', re.M)
    names = set()
    for match in pattern.finditer(source):
        item_id = int(match.group(2))
        if 1 <= item_id <= 536:
            name = slugify(match.group(1))
            if name and name != "none":
                names.add(name)
    if len(names) < 400:
        raise RuntimeError(f"Gen IV item-name table unexpectedly incomplete: {len(names)} keys")
    return names


def valid_png(raw: bytes) -> bool:
    return raw.startswith(b"\x89PNG\r\n\x1a\n") and raw[-12:-8] == b"\x00\x00\x00\x00" and raw[-8:-4] == b"IEND"


def fetch(slug: str, force: bool) -> tuple[str, str]:
    target = OUT / (slug + ".png")
    if not force and target.is_file() and valid_png(target.read_bytes()):
        return slug, "cached"
    request = urllib.request.Request(f"{BASE_URL}/{slug}.png",
                                    headers={"User-Agent": "PokeBank-NX-item-sprite-recovery"})
    try:
        with urllib.request.urlopen(request, timeout=22) as response:
            payload = response.read()
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return slug, "not-upstream"
        raise
    if not valid_png(payload):
        raise RuntimeError(f"Rejected malformed upstream PNG: {slug}")
    tmp = target.with_suffix(".png.incomplete")
    tmp.write_bytes(payload)
    tmp.replace(target)
    return slug, "fetched"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--required-only", action="store_true",
                        help="fast path for validating the sixteen exact Gen IV balls")
    parser.add_argument("--verify-existing", action="store_true",
                        help="offline check; never downloads")
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    names = set(REQUIRED_BALLS) if args.required_only else gen4_names() | set(REQUIRED_BALLS)
    if args.verify_existing:
        missing = [name for name in REQUIRED_BALLS
                   if not (OUT / f"{name}.png").is_file() or
                   not valid_png((OUT / f"{name}.png").read_bytes())]
        if missing:
            raise SystemExit("Missing required ball artwork: " + ", ".join(missing))
        print("GEN IV BALL SPRITE PREFLIGHT: PASS (16 distinct item images)")
        return 0
    results = {"cached": 0, "fetched": 0, "not-upstream": 0}
    with concurrent.futures.ThreadPoolExecutor(max_workers=12) as workers:
        for slug, status in workers.map(lambda name: fetch(name, args.force), sorted(names)):
            results[status] += 1
    print(f"Item icon recovery from pinned PokeAPI {PINNED_REF[:12]}: {results}")
    missing = [name for name in REQUIRED_BALLS
               if not (OUT / f"{name}.png").is_file() or
               not valid_png((OUT / f"{name}.png").read_bytes())]
    if missing:
        raise SystemExit("Missing REQUIRED ball-specific sprites: " + ", ".join(missing))
    print("GEN IV BALL SPRITE PREFLIGHT: PASS (16 distinct item images)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
