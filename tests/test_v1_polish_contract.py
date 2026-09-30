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
system_icons = read("src/UI/SystemIcons.cpp")
backup_selection = read("src/UI/BackupSelectionScreen.cpp")
picker_dialog = read("src/UI/Dialogs/PickerDialog.cpp")
classic_inventory = read("src/UI/ClassicInventoryOverlay.cpp")
gen1_overlay = read("src/UI/Gen1PokemonEditorOverlay.cpp")
classic_release = read("src/UI/ClassicReleaseActionFix.inc")
gen1_ux = read("src/UI/Gen1PokemonEditorOverlayUX.inc")
gen1_ux2 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc")
gen1_ux3 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc")

# Settings owns build/safety identity; the global nav bar owns controls.
require("A: toggle / info" not in trainer,
        "Settings must not duplicate the global control legend")
require('"PokeBank NX v" + VERSION_STRING + "  /  build " + BUILD_COMMIT' in trainer,
        "Settings must expose offline version + application SHA")
require("External writes: LOCKED" in trainer,
        "Settings must surface external-write safety state")
require("Cross-game True Move: LOCKED" in trainer,
        "Settings must surface the cross-game True Move lock")

# Game artwork stays clean. The HOME-style list may identify the source family beside the game
# name, but physical provider/count/path detail remains inside Save Instances.
start = save_select.index("// Right: selected-game hero card.")
end = save_select.index("// Right side: compact Vault/Pokédex row", start)
hero_draw = save_select[start:end]
require("locationLabel" not in hero_draw,
        "selected-game hero must not mix physical source paths/counts into cover presentation")
require("normalizedPath" not in hero_draw and "sourceIdentity" not in hero_draw,
        "selected-game hero must not expose source diagnostics over normal game presentation")
require("SAVE INSTANCES / " in save_select and "providerSummary(parent.legacyInstances)" in save_select,
        "Save Instances must remain the detailed physical-source presentation")
require("title.sourceLabel" in save_select,
        "HOME-style game list should still identify the source family without covering artwork")

# Source-browser wording must distinguish the synthetic Game Sources view from a Switch user.
require("No validated Pokémon game sources found" in save_select,
        "Game Sources needs an accurate empty state")
for stale in (
    '"Pokemon " + parent.label + " — Save Instances"',
    '"Pokemon " + entry.title + " — "',
    '"Pokemon " + title,',
    '"Pokemon " + title + " — Save Instances"',
):
    require(stale not in save_select,
            "visible source-browser headings must use the product's Pokémon spelling")

# Optional artwork failure must degrade to a deliberate shared placeholder rather than a blank card.
require("IconImage makeGameCardFallback()" in system_icons,
        "game-card artwork needs a generated missing-art fallback")
require("if (systemIcon.valid()) return systemIcon;" in system_icons,
        "installed-title icons must fall through to packaged/fallback artwork on decode failure")
require("img = makeGameCardFallback();" in system_icons,
        "missing/corrupt packaged artwork must activate the generated fallback")

# Save Backups uses the same shared controller glyph language as the rest of PokeBank NX.
for stale in (
    '"A: Ownership Info  |  B: Back"',
    '"A: Select  |  X: Delete  |  B: Back"',
    '"A: Select  |  B: Back"',
):
    require(stale not in backup_selection,
            "Save Backups must not bypass the shared controller glyph legend")
require('{{"Up/Down", "Choose"}, {"A", "Select"}, {"X", "Delete"}, {"B", "Back"}}' in backup_selection,
        "Save Backups must advertise D-pad/Left Stick selection alongside active actions")
require("Colors::Warning, 2" in backup_selection and "Colors::Orange, 2" not in backup_selection,
        "transient backup failures must use the semantic theme warning color")

# Pickers and Classic Inventory overlays must use the shared colored controller bar, not embedded
# plain-text footer instructions that duplicate/contradict the app-wide legend.
require("D-pad/Stick Navigate   A Select   B Cancel   L/R Page" not in picker_dialog,
        "generic picker must not render the old embedded text control footer")
require('{{"Up/Down", "Navigate"}, {"A", "Select"}, {"B", "Cancel"}, {"L/R", "Page"}}' in picker_dialog,
        "generic picker must publish its controls through the shared glyph bar")
for stale in (
    "A Confirm    B Cancel",
    "A Discard Staged Changes    B Cancel",
    "D-pad/Stick Navigate     A Add/Select",
    "B Cancel                  L/R Page",
    "B Close Help",
    "D-pad / Left Stick Select    A Open    B Close",
    "D-pad / Left Stick Browse    B Back to Options",
):
    require(stale not in classic_inventory,
            "Classic Inventory overlays must not restore old embedded controller footers")
require('{{"Up/Down", "Navigate"}, {"A", "Add / Select"}' in classic_inventory,
        "Classic Inventory picker must use the shared glyph bar")
require('{{"Up/Down", "Choose"}, {"A", "Open"}, {"B", "Close"}}' in classic_inventory,
        "Classic Inventory options must use the shared glyph bar")

# Reachable Gen I overlays follow the same shared control-bar contract.
for stale in (
    "Up/Down 1    Left/Right 10    A Select    B Cancel",
    "A Select    B Cancel    No live RetroArch or installed-game writes",
    "A/B Back to Actions",
    "Left/Right Section    A Edit    B Actions",
    "Left/Right Step    Up/Down Row    A Edit/Stage",
    "A Export edited copy + original backup",
    "Left/Right Destination Box    A Stage Clone    B Cancel",
    "A Stage Remove    B Cancel",
):
    require(stale not in gen1_overlay,
            "Gen I overlays must not render legacy embedded controller footers")
require("drawNavBar(fb" in gen1_overlay,
        "Gen I overlays must publish controls through the shared glyph bar")
require('drawFooter(fb, "A Confirm Release   B Cancel")' not in classic_release,
        "Gen I release confirmation must use the shared glyph bar")
require('drawFooter(fb, "D-pad/Stick Navigate   A Select   B Back")' not in classic_release,
        "Gen I actions must use the shared glyph bar")

# The current Gen I UX stack centralizes footer hints; that helper must route through shared chrome.
require("drawNavBar(fb, text);" in gen1_ux,
        "Gen I footer helper must use the shared controller bar")
for source in (gen1_ux, gen1_ux2, gen1_ux3):
    for stale in (
        "D-pad Navigate   ",
        "D-pad/Stick Navigate   ",
        "A/B Back",
        "A Confirm Remove   B Cancel",
    ):
        require(stale not in source,
                "Gen I UX footer strings must use shared parseable controller hints")

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
require('btn == "D-pad/Stick"' in chrome and 'btn == "D-pad"' in chrome,
        "shared navigation glyph must accept legacy D-pad token aliases")

# User-visible NRO metadata is PokeBank NX-owned. Internal compatibility symbols may retain
# historical names; this contract intentionally checks only the metadata fields.
require("APP_TITLE   :=  PokeBank NX" in makefile,
        "NRO title must remain PokeBank NX")
author_line = next(line for line in makefile.splitlines() if line.startswith("APP_AUTHOR"))
require("PKSE" not in author_line and "PokeBank NX" in author_line,
        "NRO author metadata must not expose stale PKSE-era branding")

print("v1 polish contract PASS")
