#!/usr/bin/env python3
"""Prepare Product Home presentation art used by hardware builds.

Trainer sprites are fetched from named Pokemon Showdown resources. The eight user-supplied
Kanto-through-Galar backdrops remain branch-owned presentation assets. Hisui and Paldea are fetched
from fixed real-region artwork URLs because those later-region assets were not supplied with that set.
The build never generates fake scenery.
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
USER_AGENT = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 PokeBank-NX-hardware-build/1.0"

TRAINER_URLS = {
    "red.png": "https://play.pokemonshowdown.com/sprites/trainers/red.png",
    "leaf.png": "https://play.pokemonshowdown.com/sprites/trainers/leaf-gen3.png",
    "gold.png": "https://play.pokemonshowdown.com/sprites/trainers/ethan-gen2.png",
    "kris.png": "https://play.pokemonshowdown.com/sprites/trainers/kris-gen2.png",
    "brendan.png": "https://play.pokemonshowdown.com/sprites/trainers/brendan.png",
    "may.png": "https://play.pokemonshowdown.com/sprites/trainers/may.png",
    "lucas.png": "https://play.pokemonshowdown.com/sprites/trainers/lucas.png",
    "dawn.png": "https://play.pokemonshowdown.com/sprites/trainers/dawn.png",
    "ethan.png": "https://play.pokemonshowdown.com/sprites/trainers/ethan.png",
    "lyra.png": "https://play.pokemonshowdown.com/sprites/trainers/lyra.png",
    "chase.png": "https://play.pokemonshowdown.com/sprites/trainers/chase.png",
    "elaine.png": "https://play.pokemonshowdown.com/sprites/trainers/elaine.png",
    "victor.png": "https://play.pokemonshowdown.com/sprites/trainers/victor.png",
    "gloria.png": "https://play.pokemonshowdown.com/sprites/trainers/gloria.png",
    "rei.png": "https://play.pokemonshowdown.com/sprites/trainers/rei.png",
    "akari.png": "https://play.pokemonshowdown.com/sprites/trainers/akari.png",
    "florian.png": "https://play.pokemonshowdown.com/sprites/trainers/florian-s.png",
    "juliana.png": "https://play.pokemonshowdown.com/sprites/trainers/juliana-s.png",
    "paxton.png": "https://play.pokemonshowdown.com/sprites/trainers/paxton.png",
    "harmony.png": "https://play.pokemonshowdown.com/sprites/trainers/harmony.png",
}

BRANCH_OWNED_REGIONS = (
    "kanto.png",
    "johto.png",
    "hoenn.png",
    "sinnoh.png",
    "unova.png",
    "kalos.png",
    "alola.png",
    "galar.png",
)

REGION_URLS = {
    # Real in-game / official-region artwork, not generated substitutes.
    "hisui.png": "https://archives.bulbagarden.net/media/upload/5/5b/Hisui.png",
    "paldea.png": "https://archives.bulbagarden.net/media/upload/f/fd/Paldea_artwork.png",
}

REQUIRED_REGIONS = BRANCH_OWNED_REGIONS + tuple(REGION_URLS)


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
            headers = {"User-Agent": USER_AGENT}
            # Bulbagarden Archives rejects anonymous hotlink-style clients. These are fixed file
            # URLs chosen above, so send its own site as Referer rather than weakening validation.
            if "archives.bulbagarden.net/" in url:
                headers["Referer"] = "https://archives.bulbagarden.net/"
                headers["Accept"] = "image/avif,image/webp,image/apng,image/png,image/*,*/*;q=0.8"
            request = urllib.request.Request(url, headers=headers)
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

        # Preserve the exact branch-owned region set supplied for Kanto through Galar.
        for name in BRANCH_OWNED_REGIONS:
            path = REGIONS / name
            data = path.read_bytes()
            width, height = png_size(data)
            print(f"VERIFIED {path.relative_to(ROOT)} {width}x{height} {len(data)} bytes")

        # Hisui and Paldea were missing from that set. Fetch fixed real-region artwork instead of
        # synthesizing scenery or reusing Sinnoh/Kalos as misleading substitutes.
        for name, url in REGION_URLS.items():
            fetch(url, REGIONS / name)
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
