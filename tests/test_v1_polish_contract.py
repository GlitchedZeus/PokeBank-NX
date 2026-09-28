#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


trainer = read("src/UI/TrainerViewScreenBase.inc")
save_select = read("src/UI/SaveSelectScreen.cpp")
chrome = read("include/UI/ScreenChrome.h")
makefile = read("Makefile")

# Settings owns build/safety identity; the global nav bar owns controls.
require("A: toggle / info" not in trainer,
        "Settings must not duplicate the global control legend")
require('"PokeBank NX v" + VERSION_STRING + "  /  build " + BUILD_COMMIT' in trainer,
        "Settings must expose offline version + application SHA")
require("External writes: LOCKED" in trainer,
        "Settings must surface external-write safety state")
require("Cross-game True Move: LOCKED" in trainer,
        "Settings must surface the cross-game True Move lock")

# Game-card art stays clean. Save Instances owns provider/count/path detail.
start = save_select.index("for (int i = firstIdx; i < count && i < lastIdx; i++)")
end = save_select.index("titleRects.push_back", start)
card_draw = save_select[start:end]
require("tileY + 8, TILE_W - 24, 5" not in card_draw,
        "game cards must not restore the thin accent strip")
require("const std::string& sourceLabel" not in card_draw,
        "game cards must not overlay source labels on artwork")
require("locationLabel" not in card_draw,
        "game cards must not mix source-count/path metadata conventions")
require("Save Instances owns source count, provider and path metadata" in card_draw,
        "Save Instances must remain the documented metadata owner")

# The shared legend is one implementation for all screens.
for token in (
    "Color(62, 166, 96)",
    "Color(210, 72, 78)",
    "Color(70, 132, 214)",
    "Color(226, 176, 54)",
):
    require(token in chrome, "A/B/X/Y shared glyph colors are incomplete")
require('centred("LS", lsX, lsW, ink)' in chrome,
        "shared navigation glyph must communicate Left Stick parity")

# User-visible NRO metadata is PokeBank NX-owned. Internal compatibility symbols may retain
# historical names; this contract intentionally checks only the metadata fields.
require("APP_TITLE   :=  PokeBank NX" in makefile,
        "NRO title must remain PokeBank NX")
author_line = next(line for line in makefile.splitlines() if line.startswith("APP_AUTHOR"))
require("PKSE" not in author_line and "PokeBank NX" in author_line,
        "NRO author metadata must not expose stale PKSE-era branding")

print("v1 polish contract PASS")
