#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing touch contract source: {rel}")
    return path.read_text(encoding="utf-8")

def require_all(rel: str, *needles: str) -> None:
    text = read(rel)
    for needle in needles:
        if needle not in text:
            raise AssertionError(f"{rel}: missing app-wide touch contract token: {needle}")

SURFACES = {
    "main / selected-game home / party strip / feature placeholders": (
        "src/UI/SaveSelectScreen.cpp",
        ("headerRects", "dockRects", "featureRects", "titleRects",
         "touch.justTouchedDown()", "touch.justTapped()",
         "MainMenuDestination::MasterVault", "MainMenuDestination::Pokedex"),
    ),
    "game library / quick games / profile / save instances / source setup": (
        "src/UI/SaveSelectScreen.cpp",
        ("overlayRects", "Overlay::GamesDrawer", "Overlay::ProfilePicker",
         "Overlay::GameWorkspace", "Overlay::LegacyInstances",
         "Overlay::LegacyAssignment", "Overlay::Gen4Setup",
         "Overlay::Gen4Candidates", "Overlay::Options"),
    ),
    "settings / themes / diagnostics / about / more": (
        "src/UI/AppShellScreen.cpp",
        ("Overlay::Settings", "settingsCategoryRects", "settingsRects",
         "Overlay::Diagnostics", "Overlay::More", "overlayRects",
         "touch.justTouchedDown()", "touch.justTapped()"),
    ),
    "items / inventory": (
        "src/UI/ClassicInventoryOverlay.cpp",
        ("TouchInput& touch", "screen.touchButtons.push_back",
         "touch.justTouchedDown()", "touch.justReleased() && touch.dragged()"),
    ),
    "storage / party movement": (
        "src/UI/ClassicPackedMoveOverlay.inc",
        ("cellAtPoint", "touchArmed", "touchSelecting", "touch.dragged()"),
    ),
    "backup selection": (
        "src/UI/BackupSelectionScreen.cpp",
        ("touch.justReleased() && touch.dragged()",
         "touch.justReleased() && !touch.dragged()"),
    ),
    "save confirmation": (
        "src/UI/Dialogs/SaveConfirmDialog.cpp",
        ("\"B\", \"Cancel\", 90", "\"A\", \"Save\", 91"),
    ),
    "view / legality / ribbons & marks": (
        "src/UI/Modals/PokemonDetailsModal.cpp",
        ("legalityScroll", "ribbonScroll", "setClipRect",
         "screen.touchButtons.push_back({96"),
    ),
    "Gen I editor": (
        "src/UI/Gen1PokemonEditorOverlayFoundation.inc",
        ("foundationDirectTouchInput", "touchedButtonDownId(touch)",
         "touchedButtonId(touch)"),
    ),
    "Gen II editor": (
        "src/UI/Gen2HardwareWorkspaceFix.inc",
        ("handleUnifiedTouch", "focusUnifiedTouchPoint",
         "touch.justTouchedDown()", "touch.justTapped()"),
    ),
    "Gen III editor": (
        "src/UI/Gen3SharedPokemonSurface.inc",
        ("handleWorkspace", "touchedButtonDownId(touch)",
         "touchedButtonId(touch)", "touch.justReleased() && touch.dragged()"),
    ),
    "Gen IV editor / party & storage views": (
        "src/UI/Gen4SharedPokemonSurface.inc",
        ("partyEntrySurface", "boxEntrySurface", "handleWorkspace",
         "touchedButtonDownId(touch)", "touchedButtonId(touch)",
         "touch.justReleased() && touch.dragged()"),
    ),
}

for name, (rel, needles) in SURFACES.items():
    try:
        require_all(rel, *needles)
    except AssertionError as exc:
        raise AssertionError(f"{name}: {exc}") from exc

chrome = read("include/UI/ScreenChrome.h")
for glyph in ("+", "-", "L", "R", "ZL", "ZR", "Left", "Right", "Up", "Down"):
    if f'btn == "{glyph}"' not in chrome:
        raise AssertionError(f"ScreenChrome: missing tappable controller glyph mapping for {glyph}")
for face in ("A", "B", "X", "Y"):
    if f"btn[0] == '{face}'" not in chrome:
        raise AssertionError(f"ScreenChrome: missing face-button glyph support for {face}")
require_all("include/UI/ScreenChrome.h",
            "g_touchGlyphHits", "navTouchButton",
            "release-confirmed tap",
            "Content cards/rows are owned by the screen that draws them.")

print(f"touch app-wide surface inventory: PASS ({len(SURFACES)} surface families)")
