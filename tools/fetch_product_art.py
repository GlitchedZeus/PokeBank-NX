#!/usr/bin/env python3
"""Fetch Product Home presentation art used by hardware builds.

No generated trainer/region substitutes are allowed here. A missing or invalid download is a
hard build failure so hardware can never silently receive fake scenery or a Poké Ball in place of
a trainer whose real asset is expected.

Region maps are own-work CC0 recreations from Wikimedia Commons by Ztash:
  Hoenn:  https://commons.wikimedia.org/wiki/File:Hoenn_Map.png
  Sinnoh: https://commons.wikimedia.org/wiki/File:Sinnoh_Map.png
  Kalos:  https://commons.wikimedia.org/wiki/File:Kalos_Map.png

Trainer sprites are the named Pokemon Showdown trainer sprite resources. Gold intentionally uses
the Generation II Ethan/Gold sprite and Kris uses the Generation II Kris sprite; HGSS Ethan/Lyra
remain separate keys.
"""
from __future__ import annotations

import struct
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROMFS = ROOT / "romfs"
TRAINERS = ROMFS / "trainer_portraits"
REGIONS = ROMFS / "region_backdrops"
USER_AGENT = "PokeBank-NX-hardware-build/1.0"

TRAINER_URLS = {
    "red.png": "https://play.pokemonshowdown.com/sprites/trainers/red.png",
    "gold.png": "https://play.pokemonshowdown.com/sprites/trainers/ethan-gen2.png",
    "kris.png": "https://play.pokemonshowdown.com/sprites/trainers/kris-gen2.png",
    "brendan.png": "https://play.pokemonshowdown.com/sprites/trainers/brendan.png",
    "may.png": "https://play.pokemonshowdown.com/sprites/trainers/may.png",
    "lucas.png": "https://play.pokemonshowdown.com/sprites/trainers/lucas.png",
    "dawn.png": "https://play.pokemonshowdown.com/sprites/trainers/dawn.png",
    "ethan.png": "https://play.pokemonshowdown.com/sprites/trainers/ethan.png",
    "lyra.png": "https://play.pokemonshowdown.com/sprites/trainers/lyra.png",
}

REGION_URLS = {
    "hoenn.png": "https://upload.wikimedia.org/wikipedia/commons/a/a4/Hoenn_Map.png",
    "sinnoh.png": "https://upload.wikimedia.org/wikipedia/commons/8/85/Sinnoh_Map.png",
    "kalos.png": "https://upload.wikimedia.org/wikipedia/commons/d/d8/Kalos_Map.png",
}


def png_size(data: bytes) -> tuple[int, int]:
    if len(data) < 33 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise ValueError("not a PNG")
    width, height = struct.unpack(">II", data[16:24])
    if width <= 0 or height <= 0:
        raise ValueError("invalid PNG dimensions")

    # Walk every chunk. This catches the exact failure that previously let a truncated trainer
    # atlas through CI merely because its IHDR still looked correct.
    offset = 8
    saw_iend = False
    while offset + 12 <= len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        end = offset + 12 + length
        if end > len(data):
            raise ValueError("truncated PNG chunk")
        kind = data[offset + 4:offset + 8]
        offset = end
        if kind == b"IEND":
            saw_iend = True
            break
    if not saw_iend:
        raise ValueError("PNG has no complete IEND")
    return width, height


def fetch(url: str, destination: Path) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=45) as response:
        data = response.read()
    width, height = png_size(data)
    destination.parent.mkdir(parents=True, exist_ok=True)
    tmp = destination.with_suffix(destination.suffix + ".tmp")
    tmp.write_bytes(data)
    tmp.replace(destination)
    print(f"FETCHED {destination.relative_to(ROOT)} {width}x{height} {len(data)} bytes")


def main() -> int:
    try:
        for name, url in TRAINER_URLS.items():
            fetch(url, TRAINERS / name)
        for name, url in REGION_URLS.items():
            fetch(url, REGIONS / name)
    except Exception as exc:
        print(f"PRODUCT ART FETCH FAILED: {exc}", file=sys.stderr)
        return 1

    expected_trainers = set(TRAINER_URLS)
    expected_regions = set(REGION_URLS)
    if not expected_trainers.issubset({p.name for p in TRAINERS.glob("*.png")}):
        print("PRODUCT ART FETCH FAILED: trainer set incomplete", file=sys.stderr)
        return 1
    if not expected_regions.issubset({p.name for p in REGIONS.glob("*.png")}):
        print("PRODUCT ART FETCH FAILED: region set incomplete", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
