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
         "dockRects", "headerRects", "featureRects", "overlayRects",
         "gamesDrawerTouchScroll.updateVertical(",
         "classicGamesTouchScroll.updateVertical(",
         "profileTouchScroll.updateVertical(",
         "launchFileTouchScroll.updateVertical(",
         "legacyInstanceTouchScroll.updateVertical(",
         "gen4CandidateTouchScroll.updateVertical("),
    ),
    "App shell / Settings / More": (
        "src/UI/AppShellScreen.cpp",
        ("touch.justTouchedDown()", "touch.justTapped()", "cardRects",
         "settingsCategoryRects", "settingsRects", "overlayRects",
         "liveVerticalListVisual(", "optionVisual.offset"),
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
        ("touch.justTouchedDown()", "touch.justTapped()", "touch.justReleased()",
         "liveVerticalListVisual(", "visual.offset",
         "touchButtons.push_back({1000 + i", "touchButtons.push_back({1100 + i"),
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
         "touchedButtonId(touch)", "pickerScroll.updateVertical(",
         "pickerScroll.offset()", "reviewTouchScroll.updateVertical(",
         "reviewTouchScroll.offset()"),
    ),
    "Gen IV editor / party & storage views": (
        "src/UI/Gen4SharedPokemonSurface.inc",
        ("partyEntrySurface", "boxEntrySurface", "handleWorkspace",
         "touchedButtonDownId(touch)", "touchedButtonId(touch)",
         "pickerScroll.updateVertical(", "pickerScroll.offset()"),
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
            "? touch.deltaX() : 0;",
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
            "Quick Games owns true vertical pixel scrolling in SaveSelectScreen",
            "Full Games owns vertical pixel scrolling",
            "SaveSelect's long profile/file/save-source lists own true pixel-scrolling",
            "g_contentSwipeMask = HidNpadButton_Left | HidNpadButton_Right")
require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
            "state.pickerScroll.updateVertical(",
            "state.editorScroll.updateVertical(",
            "state.reviewScroll.updateVertical(",
            "state.pickerScroll.offset()",
            "state.editorScroll.offset()",
            "state.reviewScroll.offset()")
require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
            "void ux2ResetTouchMotion(UX2State& state) noexcept",
            "state.editorScroll.reset();",
            "state.pickerScroll.reset();",
            "state.reviewScroll.reset();",
            "case UX::Action::Edit:",
            "case UX::Action::ReviewPendingChanges:")
require_all("src/UI/Gen2HardwarePickerFix.inc",
            "liveVerticalListVisual(",
            "visual.offset",
            "fb.setClipRect(")
require_all("include/UI/SharedPokemonShell.h",
            "int pixelOffset = 0",
            "static_cast<int>(i) * 38 + pixelOffset")
require_all("src/UI/Gen2HardwareFinalFix.inc",
            "const auto detailVisual = liveVerticalListVisual(",
            "+ detailVisual.offset")
require_all("src/UI/Gen2PokemonEditorFoundation.inc",
            "int stickX, int stickY, const TouchInput& touch)",
            "return handleInput(screen, down, held, stickX, stickY, noTouch);")
require_all("src/UI/Gen2UnifiedPokemonWorkspace.inc",
            "handleItemPicker(screen, state, down, held, stickX, stickY, touch)")
composite_touch = read("src/UI/TrainerViewScreenCompositeOverlay.cpp")
for dead_macro in ("#define touch kLegacyNoTouch",
                   "#define handleItemPicker(",
                   "#define touchedButtonDownId(",
                   "#define touchedButtonId(",
                   "#define layout "):
    if dead_macro in composite_touch:
        raise AssertionError(f"Composite touch glue regressed to macro adapter: {dead_macro}")
require_all("src/UI/Gen3SharedPokemonSurface.inc",
            "const auto detailVisual = liveVerticalListVisual(",
            "detailVisual.offset")
require_all("src/UI/Gen4SharedPokemonSurface.inc",
            "const auto detailVisual = liveVerticalListVisual(",
            "detailVisual.offset")
require_all("src/UI/SaveSelectScreen.cpp",
            "gamesDrawerTouchScroll.updateVertical(",
            "classicGamesTouchScroll.updateVertical(",
            "profileTouchScroll.updateVertical(",
            "launchFileTouchScroll.updateVertical(",
            "legacyInstanceTouchScroll.updateVertical(",
            "gen4CandidateTouchScroll.updateVertical(",
            "Touch carousel gestures resolve through ScreenChrome")

require_all("include/UI/TouchScroll.h",
            "class TouchScrollState",
            "offset_ += delta",
            "const int requested = (-offset_) / step",
            "const int available = std::max(0, (count - 1 - index) / stride)",
            "const bool atTrailingEdge = index + stride >= count",
            "coasting_ = std::abs(velocity_) >= 2")
if "liveHorizontalListVisual" in read("include/UI/TouchScroll.h"):
    raise AssertionError("TouchScroll: dead horizontal draw helper was reintroduced without a caller")
require_all("include/UI/TouchGesture.h",
            "struct TouchGestureSnapshot",
            "const TouchGestureSnapshot& latestTouchGesture() noexcept")
if '#include <switch.h>' in read("include/UI/TouchGesture.h"):
    raise AssertionError("TouchGesture: renderer-facing gesture snapshot must remain host-portable")
if '#include "UI/TouchInput.h"' in read("include/UI/SharedSpeciesPicker.h"):
    raise AssertionError("SharedSpeciesPicker: draw-only species helper must not pull libnx TouchInput")
if '#include "UI/TouchInput.h"' in read("include/UI/Gen2HeldItemPicker.h"):
    raise AssertionError("Gen2HeldItemPicker: host-testable model/presentation must not pull libnx TouchInput")
if "latestTouchGesture()" in read("include/UI/Gen2HeldItemPicker.h"):
    raise AssertionError("Gen2HeldItemPicker: touch mutation shim was reintroduced")
gesture = read("include/UI/TouchGesture.h")
for dead_field in ("released", "dragged", "deltaX", "int x =", "int y ="):
    if dead_field in gesture:
        raise AssertionError(f"TouchGesture: unused renderer snapshot field reintroduced: {dead_field}")
scroll = read("include/UI/TouchScroll.h")
for dead_api in ("updateHorizontal(", "dragging() const", "moving() const", "bool tracking"):
    if dead_api in scroll:
        raise AssertionError(f"TouchScroll: unused API/state reintroduced: {dead_api}")
require_all("src/UI/SaveSelectScreen.cpp",
            "void SaveSelectScreen::syncTouchScrollSurface()",
            "launchFileTouchScroll.reset();",
            "gamesDrawerTouchScroll.reset();",
            "classicGamesTouchScroll.reset();",
            "syncTouchScrollSurface();")
require_all("src/UI/BackupSelectionScreen.cpp",
            "backupScroll.stop();",
            "showDeleteConfirmation = true;")
require_all("include/UI/SharedSpeciesPicker.h",
            "const TouchGestureSnapshot* gesture = nullptr",
            "gesture->deltaY / rowStep",
            "liveOffset = gesture->deltaY + appliedRows * rowStep",
            "fb.setClipRect(listX, listY, listW, listH - 2)")
if "latestTouchGesture()" in read("include/UI/SharedSpeciesPicker.h"):
    raise AssertionError("SharedSpeciesPicker: renderer must receive touch explicitly, not pull global state")

print(f"touch app-wide surface inventory: PASS ({len(SURFACES)} surface families)")
