from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def require_all(rel: str, *needles: str) -> None:
    text = read(rel)
    for needle in needles:
        if needle not in text:
            raise AssertionError(f"{rel}: missing app-wide touch contract token: {needle}")


# App-wide surface ownership. These are deliberately implementation-level static contracts:
# native builds still decide whether a code path compiles/runs, while this list prevents future
# presentation cleanup from silently dropping touch from one whole screen family.
SURFACES = {
    "Product Home / Games / save source flows": (
        "src/UI/SaveSelectScreen.cpp",
        ("touch.justTouchedDown()", "touch.justTapped()", "titleRects",
         "dockRects", "headerRects", "featureRects", "overlayRects"),
    ),
    "App shell / Settings / More": (
        "src/UI/AppShellScreen.cpp",
        ("touch.justTouchedDown()", "touch.justTapped()", "cardRects",
         "settingsCategoryRects", "settingsRects", "overlayRects"),
    ),
    "Backup selection": (
        "src/UI/BackupSelectionScreen.cpp",
        ("backupScroll.updateVertical(", "backupScroll.offset()", "backupScroll.stop()"),
    ),
    "Classic inventory": (
        "src/UI/ClassicInventoryOverlay.cpp",
        ("touch.justTouchedDown()", "pickerScroll.updateVertical(",
         "reviewScroll.updateVertical(", "touchedButtonId(touch)"),
    ),
    "Packed move selection": (
        "src/UI/ClassicPackedMoveOverlay.inc",
        ("touch.justTouchedDown()", "touch.isDown()", "touch.justReleased()"),
    ),
    "Release/action surfaces": (
        "src/UI/ClassicReleaseActionFix.inc",
        ("touchedButtonId(touch)", "touch.justTouchedDown()", "touch.justReleased()"),
    ),
    "Save confirmation": (
        "src/UI/Dialogs/SaveConfirmDialog.cpp",
        ("TouchTargetMin", "touchButtons.push_back"),
    ),
    "Home menu / party entry destinations": (
        "src/UI/Panels/HomeMenuPanel.cpp",
        ("touchButtons.push_back",),
    ),
    "Storage / modern boxes": (
        "src/UI/Panels/StoragePanel.cpp",
        ("touchButtons.push_back", "currentlySelecting"),
    ),
    "Pokémon details / Legality / Ribbons": (
        "src/UI/Modals/PokemonDetailsModal.cpp",
        ("legalityScroll", "ribbonScroll", "drawScrollbar", "drawGlyphButton"),
    ),
    "Gen II details": (
        "src/UI/Modals/Gen2PokemonDetailsModal.cpp",
        ("leftScroll", "drawScrollbar"),
    ),
    "Trainer view composite / party touch": (
        "src/UI/TrainerViewScreenCompositeOverlay.cpp",
        ("publishBasePartyTouchTargets", "handleBasePartyTouch", "TouchInput"),
    ),
    "GSC legacy editor": (
        "src/UI/TrainerViewScreenGSCOverlay.inc",
        ("touch.justTouchedDown()", "touch.justTapped()", "touch.justReleased()"),
    ),
    "Gen I editor foundation": (
        "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
        ("touchButtons.push_back({5000 + i", "touchButtons.push_back({5100 + r * 10 + c", "ux3DrawTouchNavBar"),
    ),
    "Gen I editor overlay": (
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
            "Content cards/rows are owned by the screen that draws them.",
            "constexpr int kLiveDragStep = 24",
            "g_contentDragVisualX = touch.x() - touch.startX()",
            "g_contentDragVisualY = touch.y() - touch.startY()",
            "const bool productHomeHero =",
            "g_contentSwipeReleaseOnly = true;",
            "g_productHeroDragXForDraw",
            "g_contentSwipeX = 24",
            "g_contentSwipeY = 78",
            "g_contentSwipeW = 720",
            "g_contentSwipeH = 548",
            "if (contentDragMoved && !releaseOnly) return 0;",
            "constexpr int kContentSwipeDistance = 72",
            "g_quickGamesDrawerSwipe = true",
            "const bool profilePicker =",
            "const bool gameFilePicker =",
            "const bool saveInstanceList =",
            "const bool saveAssignmentList =",
            "g_contentSwipeMask = verticalDirections")
require_all("include/UI/TouchScroll.h",
            "class TouchScrollState",
            "offset_ += delta",
            "rebalance(step, index, count, stride)",
            "coasting_ = std::abs(velocity_) >= 2")
require_all("include/UI/SharedSpeciesPicker.h",
            "latestTouchGesture()",
            "touch.deltaY / rowStep",
            "liveOffset = touch.deltaY + appliedRows * rowStep",
            "fb.setClipRect(listX, listY, listW, listH - 2)")

print(f"touch app-wide surface inventory: PASS ({len(SURFACES)} surface families)")
