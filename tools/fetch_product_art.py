#!/usr/bin/env python3
"""Prepare Product Home presentation art used by hardware builds.

Trainer sprites are fetched from the named Pokemon Showdown resources. Region backdrops are
project-owned presentation assets supplied for PokeBank NX and MUST already be present in RomFS.
The build validates those exact files and never downloads, generates, or substitutes different
region scenery behind the user's back.
"""
from __future__ import annotations

import struct
import sys
import time
import urllib.error
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

REQUIRED_REGIONS = (
    "kanto.png",
    "johto.png",
    "hoenn.png",
    "sinnoh.png",
    "unova.png",
    "kalos.png",
    "alola.png",
    "galar.png",
)


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
    # Public art CDNs can briefly return 429 while several exact-head Actions jobs start together.
    # Retry the same immutable URL with bounded exponential backoff; never fall back to fake art.
    last_error: Exception | None = None
    for attempt in range(6):
        try:
            request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
            with urllib.request.urlopen(request, timeout=45) as response:
                data = response.read()
            width, height = png_size(data)
            destination.parent.mkdir(parents=True, exist_ok=True)
            tmp = destination.with_suffix(destination.suffix + ".tmp")
            tmp.write_bytes(data)
            tmp.replace(destination)
            print(f"FETCHED {destination.relative_to(ROOT)} {width}x{height} {len(data)} bytes")
            return
        except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError) as exc:
            last_error = exc
            retryable = not isinstance(exc, urllib.error.HTTPError) or exc.code in (408, 429, 500, 502, 503, 504)
            if not retryable or attempt == 5:
                break
            delay = min(30, 2 ** (attempt + 1))
            print(f"RETRY {destination.name} in {delay}s after {exc}", file=sys.stderr)
            time.sleep(delay)
    raise RuntimeError(f"could not fetch {destination.name}: {last_error}")


def main() -> int:
    try:
        for name, url in TRAINER_URLS.items():
            fetch(url, TRAINERS / name)
        # Region art is intentionally NOT fetched. Validate the exact branch-owned assets that
        # Product Home is expected to show on hardware.
        for name in REQUIRED_REGIONS:
            path = REGIONS / name
            data = path.read_bytes()
            width, height = png_size(data)
            print(f"VERIFIED {path.relative_to(ROOT)} {width}x{height} {len(data)} bytes")
    except Exception as exc:
        print(f"PRODUCT ART FETCH FAILED: {exc}", file=sys.stderr)
        return 1

    expected_trainers = set(TRAINER_URLS)
    expected_regions = set(REQUIRED_REGIONS)
    if not expected_trainers.issubset({p.name for p in TRAINERS.glob("*.png")}):
        print("PRODUCT ART FETCH FAILED: trainer set incomplete", file=sys.stderr)
        return 1
    if not expected_regions.issubset({p.name for p in REGIONS.glob("*.png")}):
        print("PRODUCT ART FETCH FAILED: region set incomplete", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
