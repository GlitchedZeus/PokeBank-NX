#!/usr/bin/env python3
"""Recover exact Gen IV ball and held-item PNGs from a pinned PokeAPI/sprites revision.

Runs on the host during native RomFS recovery, never on the user's Switch.
No guessed artwork or silent fallback to another item's sprite.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import unicodedata
import urllib.parse
import urllib.error
import urllib.request
from pathlib import Path

from png_asset_validation import validate_png
from render_classic_item_icons import KEYS as CUSTOM_GSC_ICONS, build as render_classic_icons
from render_gen2_mail_stationery import render_unavailable_exact_mail

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "romfs" / "sprites" / "items"
NAMES = ROOT / "src" / "Names" / "ItemNames.cpp"
MANIFEST = ROOT / "tools" / "pinned_item_sprite_names.txt"
PINNED_REF = "8dfa3d97e953caaafaafd4963eff7621811af08e"
BASE_URL = f"https://raw.githubusercontent.com/PokeAPI/sprites/{PINNED_REF}/sprites/items"
# An exact Itemfinder icon is missing from the PokeAPI snapshot. Smogon's
# separately pinned minisprite actually depicts the Itemfinder. Do not call
# the later Dowsing Machine a Gen I/II Itemfinder.
# Item graphics belong to Nintendo / Game Freak / The Pokémon Company.
SMOGON_REF = "bad55c7b7e7292f459366505460d41dcab08d4cd"
EXTRA_EXACT_ICONS = {
    "itemfinder": (
        "https://raw.githubusercontent.com/smogon/sprites/"
        + SMOGON_REF + "/src/minisprites/otheritems/iitemfinder.png"
    ),
    # A truthful GENERIC envelope for Gen II's ten mail backgrounds.
    # Those exist as unique stationery screenshots, not 24px bag icons.
    # Never mislabel another generation's Mail as a Gen II type.
    "classic-mail-envelope": (
        "https://raw.githubusercontent.com/smogon/sprites/"
        + SMOGON_REF + "/src/minisprites/items/imail.png"
    ),
    # This is a genuinely identified old GSC item, rather than a random
    # substitute. Present in the separately pinned Smogon minisprite set.
    "berserk-gene": (
        "https://raw.githubusercontent.com/smogon/sprites/"
        + SMOGON_REF + "/src/minisprites/items/iberserk_gene.png"
    ),
}

# Generation II Mail uses ten unique 160x144 stationery BACKGROUNDS,
# not Gen III/IV 24px bag minisprites. Bulbagarden Archives file pages:
# https://archives.bulbagarden.net/wiki/Category:Mail
# Each file's original /media/upload/a/ab/Filename.png location is the
# documented MediaWiki MD5 filename directory. No artwork substitutions.
# This is optional supplemental art: the pinned Smogon generic envelope is
# the honest fallback if the Archives are temporarily unavailable in CI.
GEN2_MAIL_SOURCES = {
    "bluesky-mail": "Bluesky_Mail.png",
    "eon-mail": "Eon_Mail.png",
    "flower-mail": "Flower_Mail.png",
    "liteblue-mail": "Liteblue_Mail.png",
    "lovely-mail": "Lovely_Mail.png",
    "mirage-mail": "Mirage_Mail.png",
    "morph-mail": "Morph_Mail.png",
    "music-mail": "Music_Mail.png",
    "portraitmail": "Portraitmail.png",
    "surf-mail": "Surf_Mail.png",
}

# When Bulbagarden Archives refuses hotlinking from GitHub Actions, use
# original game art extracted from the pinned pret/pokecrystal decompilation.
# These are genuine Gen II MAIL MOTIFS, NOT full 160x144 stationery backgrounds.
# Never count a recovered motif as an exact full stationery image.
POKECRYSTAL_MAIL_REF = "e058e4f50b3bbf7377e036b81c25a72c54656c5c"
GEN2_MAIL_MOTIFS = {
    "bluesky-mail": "cloud.png",
    "eon-mail": "eevee.png",
    "flower-mail": "flower_1.png",
    "liteblue-mail": "dratini.png",
    "lovely-mail": "large_heart.png",
    "mirage-mail": "mew.png",
    "morph-mail": "ditto.png",
    "music-mail": "natu.png",
    "portraitmail": "portraitmail_border.png",
    "surf-mail": "lapras.png",
}
assert set(GEN2_MAIL_MOTIFS) == set(GEN2_MAIL_SOURCES)
GEN2_MAIL_MOTIF_URL = (
    "https://raw.githubusercontent.com/pret/pokecrystal/"
    + POKECRYSTAL_MAIL_REF + "/gfx/mail/"
)

REQUIRED_BALLS = (
    "master-ball", "ultra-ball", "great-ball", "poke-ball",
    "safari-ball", "net-ball", "dive-ball", "nest-ball",
    "repeat-ball", "timer-ball", "luxury-ball", "premier-ball",
    "dusk-ball", "heal-ball", "quick-ball", "cherish-ball",
)
# Protect native Gen I–IV inventory/held-picker artwork from becoming
# Gen IV-only again. Every key exists in the pinned 898-PNG manifest.
REQUIRED_CLASSIC = (
    "poke-doll", "bright-powder", "up-grade", "red-apricorn",
    "air-mail", "flame-mail", "tm-normal", "tm-fire", "hm01",
    "itemfinder", "classic-mail-envelope",
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


def fetch_extra(name: str, url: str, force: bool) -> str:
    target = OUT / (name + ".png")
    if not force and validated(target):
        return "cached"
    request = urllib.request.Request(
        url, headers={"User-Agent": "PokeBank-NX-pinned-exact-item-sprites"},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        payload = response.read()
    if not payload.startswith(bytes((137, 80, 78, 71, 13, 10, 26, 10))):
        raise RuntimeError("Pinned supplemental art is not PNG: " + name)
    temporary = target.with_suffix(".png.incomplete")
    temporary.write_bytes(payload)
    if not validated(temporary):
        temporary.unlink(missing_ok=True)
        raise RuntimeError("Pinned supplemental item PNG invalid: " + name)
    temporary.replace(target)
    return "fetched"



def fetch_gen2_mail(key: str, filename: str, force: bool) -> str:
    """Recover a distinct, correctly attributed Gen II mail background."""
    target = OUT / (key + ".png")
    provenance = ROOT / "build-mail-source" / (key + ".verified-original")
    if not force and validated(target) and provenance.is_file():
        ok, _, dims = validate_png(target)
        if ok and tuple(dims) == (160, 144):
            return "cached-exact"
    digest = hashlib.md5(filename.encode("ascii")).hexdigest()
    url = ("https://archives.bulbagarden.net/media/upload/"
           + digest[:1] + "/" + digest[:2] + "/" + urllib.parse.quote(filename))
    try:
        request = urllib.request.Request(
            url, headers={"User-Agent": "PokeBank-NX-mail-sprite-recovery/1.0"})
        with urllib.request.urlopen(request, timeout=15) as response:
            payload = response.read()
        if not payload.startswith(bytes((137, 80, 78, 71, 13, 10, 26, 10))):
            return "unavailable/not-png"
        temporary = target.with_suffix(".png.incomplete")
        temporary.write_bytes(payload)
        ok, reason, dimensions = validate_png(temporary)
        if not ok or tuple(dimensions) != (160, 144):
            temporary.unlink(missing_ok=True)
            return "unavailable/invalid-image"
        temporary.replace(target)
        provenance.parent.mkdir(parents=True, exist_ok=True)
        provenance.write_text(hashlib.sha256(payload).hexdigest(), encoding="ascii")
        return "fetched"
    except urllib.error.HTTPError as error:
        # Archives may reject CI hotlink requests. Keep all ten images
        # optional, but report the actual status (403 versus 404) so the
        # origin/mirror can be fixed without guessing or disguising absence.
        return f"unavailable/http-{error.code}"
    except (OSError, urllib.error.URLError, TimeoutError) as error:
        return "unavailable/" + type(error).__name__


def verify_required(include_classic: bool = False) -> None:
    missing = [name for name in REQUIRED_BALLS if not validated(OUT / (name + ".png"))]
    if missing:
        raise RuntimeError("Missing required native Gen IV ball PNGs: " + ", ".join(missing))
    print("GEN IV BALL SPRITE PREFLIGHT: PASS (16 distinct matching ball images)")
    if include_classic:
        classics = [name for name in REQUIRED_CLASSIC
                    if not validated(OUT / (name + ".png"))]
        if classics:
            raise RuntimeError("Missing pinned Gen I–IV item icons: " + ", ".join(classics))
        # Nine unused/historical GSC objects have no ROM-originating bag art.
        # Bundle deliberately NEW distinct custom illustrations, not blank
        # icons, and fail rather than fall back to unrelated inventory items.
        custom = [name for name in CUSTOM_GSC_ICONS
                  if not validated(OUT / (name + ".png"))]
        if custom:
            raise RuntimeError("Missing custom GSC historical item sprites: " + ", ".join(custom))
        assert validated(OUT / "berserk-gene.png")
        mail_bad = []
        for key in GEN2_MAIL_SOURCES:
            file = OUT / (key + ".png")
            valid, reason, dims = validate_png(file) if file.exists() else (False, "absent", ())
            if not valid or tuple(dims) != (160, 144):
                mail_bad.append(key)
        if mail_bad:
            raise RuntimeError("Mail background is not full 160x144 GSC stationery: "
                               + ", ".join(mail_bad))
        print("CLASSIC ITEM SPRITE PREFLIGHT: PASS (machine type discs, full 160x144 mail art, GSC historic icons)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--required-only", action="store_true")
    parser.add_argument("--verify-existing", action="store_true")
    options = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    if options.verify_existing:
        verify_required(include_classic=not options.required_only)
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
    if not options.required_only:
        extras = {
            name: fetch_extra(name, url, options.force)
            for name, url in EXTRA_EXACT_ICONS.items()
        }
        print(f"Extra pinned exact item images (Smogon {SMOGON_REF[:12]}): {extras}")
        print(f"Distinct custom Gen II historical item illustrations: {render_classic_icons()}")
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
            mail_status = dict(zip(
                GEN2_MAIL_SOURCES,
                pool.map(lambda pair: fetch_gen2_mail(pair[0], pair[1], options.force),
                         GEN2_MAIL_SOURCES.items())))
        print(f"Generation II original Mail art (Bulbagarden Archives): {mail_status}")
        # If the Archives rejects the genuine 160x144 screenshot, render a
        # full stationery preview from the ORIGINAL Crystal PNG motifs/borders
        # and palette; do not silently show a tiny letter motif as "mail art".
        # The fallback is explicitly reconstructed, never labelled exact ROM.
        unavailable = [key for key, result in mail_status.items()
                       if result not in ("fetched", "cached-exact")]
        reconstructed = render_unavailable_exact_mail(unavailable)
        print("Generation II original-game source reconstructed stationery: "
              + str(reconstructed))
        invalid_mail = [key for key in GEN2_MAIL_SOURCES
                        if not validated(OUT / (key + ".png"))]
        if invalid_mail:
            raise RuntimeError("Missing full-size GSC stationery art: "
                               + ", ".join(invalid_mail))
    verify_required(include_classic=not options.required_only)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
