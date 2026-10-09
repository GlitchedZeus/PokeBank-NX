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
         "Every Settings category currently fits in the pane (max 3 options)."),
    ),
    "Backup selection": (
        "src/UI/BackupSelectionScreen.cpp",
        ("backupScroll.updateVertical(", "backupScroll.offset()", "backupScroll.stop()",
         "backupFirstRow, maxFirstRow + 1",
         "insideTile(touch.startX(), touch.startY())",
         "insideTile(touch.x(), touch.y())"),
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
         "struct Gen2EditorOverlayGeometry",
         "fieldViewport.contains(touch.startX(), touch.startY())",
         "reviewViewport.contains(touch.startX(), touch.startY())",
         "static constexpr int ReviewRowStep = 56;",
         "liveVerticalListVisual(", "visual.offset",
         "touchButtons.push_back({1000 + i, fieldViewport.x",
         "touchButtons.push_back({1100 + i, x + 30, rowY, w - 60, actionRowStep"),
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

# Inventory row focus must stay stable on touch-down; only a completed tap
# may select an item/review line. Otherwise a swipe changes highlighted rows.
inventory_input = read("src/UI/ClassicInventoryOverlay.cpp")
inventory_contact = inventory_input.split("if (touch.justTouchedDown()) {", 1)[1].split(
    "if (!state.presentationInitialized", 1)[0]
if "state.pickerRow = hit.id" in inventory_contact or "state.reviewRow = hit.id" in inventory_contact:
    raise AssertionError("Classic Inventory: contact must not steal picker/review focus")
require_all("src/UI/ClassicInventoryOverlay.cpp",
            "state.pickerRow = touchId;", "state.reviewRow = touchId;")
# Backup selection must scroll its viewport, not re-center to its changing selection.
require_all("include/UI/BackupSelectionScreen.h", "int backupFirstRow = 0;")
if "firstVisibleRow(" in read("src/UI/BackupSelectionScreen.cpp"):
    raise AssertionError("Backup Selection: stale selection-centered scroll helper returned")

chrome = read("include/UI/ScreenChrome.h")
for glyph in ("+", "-", "L", "R", "ZL", "ZR", "Left", "Right", "Up", "Down"):
    if f'btn == "{glyph}"' not in chrome:
        raise AssertionError(f"ScreenChrome: missing tappable controller glyph mapping for {glyph}")
for face in ("A", "B", "X", "Y"):
    if f"btn[0] == '{face}'" not in chrome:
        raise AssertionError(f"ScreenChrome: missing face-button glyph support for {face}")
require_all("include/UI/ScreenChrome.h",
            "g_touchGlyphHits", "navTouchButton",
            "!navContains(touch.x(), touch.y(), h.x, h.y, h.w, h.h)",
            "if (ay < 24 || ay <= ax) return 0;",
            "if (ax < 24 || ax <= ay) return 0;",
            "if (touch.dragged() && (ax < 24 || ax <= ay)) return 0;",
            "release-confirmed tap",
            "Content cards/rows are owned by the screen that draws them.",
            "g_productHeroDragActive",
            "g_productHeroDragXForDraw",
            # An active drag belongs only to the Product Home surface where it began.
            "if (productHeroDrag && g_productHeroSwipeRegistered && touch.dragged())",
            "constexpr int kProductHeroX = 24;",
            "constexpr int kProductHeroY = 78;",
            "constexpr int kProductHeroW = 720;",
            "constexpr int kProductHeroH = 548;",
            "constexpr int kCarouselCommitDistance = 72",
            "g_quickGamesDrawerSwipe = true",
            "Shared chrome keeps only its deliberate inner-edge close gesture.",
            "Shared chrome intentionally does not emulate",
            "content D-pad movement for them.")
chrome = read("include/UI/ScreenChrome.h")
for dead in (
    "g_contentSwipeMask", "g_contentSwipeUsesShoulders", "g_contentSwipeReleaseOnly",
    "g_contentDragMoved", "g_contentDragLastX", "g_contentDragLastY",
    "kLiveDragStep", "classicGamesBrowser", "currentGameGrid",
    "contentSwipeContains", "horizontalContentButton",
):
    if dead in chrome:
        raise AssertionError(f"ScreenChrome: deleted generic content-stepper state returned: {dead}")

if '#include "UI/TouchScroll.h"' in read("src/UI/AppShellScreen.cpp"):
    raise AssertionError("AppShell: Settings has no overflow; fake TouchScroll dependency was reintroduced")
if "liveVerticalListVisual(" in read("src/UI/AppShellScreen.cpp"):
    raise AssertionError("AppShell: non-overflowing Settings rows must stay anchored, not fake-scroll")
app_shell = read("src/UI/AppShellScreen.cpp")
if "touch.justReleased() && touch.dragged()" in app_shell:
    raise AssertionError("AppShell: fixed fully-visible card grids must not emulate D-pad on swipe")

require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
            "state.pickerScroll.updateVertical(",
            "state.editorScroll.updateVertical(",
            "state.reviewScroll.updateVertical(",
            "state.pickerScroll.offset()",
            "state.editorScroll.offset()",
            "state.reviewScroll.offset()")
# Gen I's three moving vertical surfaces share the same clipped-hitbox contract as
# later generations. A row outside a framebuffer clip must not remain tappable.
require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
            "appendClippedTouchButton(screen.touchButtons, 2000 + field,",
            "centerX + 8, listY, centerW - 16, viewportH",
            "appendClippedTouchButton(screen.touchButtons, 4000 + idx,",
            "x + 24, listTop, w - 48, visible * rowStep",
            "appendClippedTouchButton(screen.touchButtons, 1300 + row,",
            "x + 24, listTop, w - 48, h - 108")

# Long Gen I Species and Move pickers must not recenter on the contact edge.
# Scrolling and tapping are separate gestures; only a clean release may select.
gen1_direct = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc").split(
    "uint64_t ux3DirectTouchInput(", 1)[1]
species_input = gen1_direct.split("if (state.picker == UX2Picker::Species) {", 1)[1].split(
    "if (state.picker == UX2Picker::Move) {", 1)[0]
move_input = gen1_direct.split("if (state.picker == UX2Picker::Move) {", 1)[1].split(
    "if (state.mode == UX2Mode::Edit", 1)[0]
for picker_name, input_body in (("Species", species_input), ("Move", move_input)):
    if "pressed" in input_body:
        raise AssertionError(f"Gen I {picker_name}: touch-down must not jump picker selection")
    if "selectedId(" not in input_body or "tapped" not in input_body:
        raise AssertionError(f"Gen I {picker_name}: clean tap selection missing")
# Gen I Move follows the shared independent picker viewport contract. Dragging
# may move the first visible row but must not change the selected Move itself.
require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
            "TouchPickerViewport pickerViewport;",
            "state.pickerViewport.reset();")
require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
            "state.pickerViewport.ensure(selectedIndex, count, visible);",
            "const int start = state.pickerViewport.firstRow;",
            "state.pickerViewport.firstRow, maxFirst + 1);",
            "state.pickerViewport.reveal(index, count, visible);",
            "state.pickerViewport.containsSelection(index, count, visible)",
            "Focused move brought into view; press A again or tap a visible row")
if "state.pickerValue = choices[static_cast<size_t>(current)]" in move_input:
    raise AssertionError("Gen I Move picker: finger scrolling still changes the highlighted Move")
require_all("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
            "void ux2ResetTouchMotion(UX2State& state) noexcept",
            "state.editorScroll.reset();",
            "state.pickerScroll.reset();",
            "state.reviewScroll.reset();",
            "case UX::Action::Edit:",
            "case UX::Action::ReviewPendingChanges:")
require_all("src/UI/Gen2HardwarePickerFix.inc",
            "picker.moveViewport.ensure(selectedPos, count, visible);",
            "const int first = overlay.moveViewport.firstRow;",
            "const int liveOffset = overlay.moveScroll.offset();",
            "picker.moveScroll.updateVertical(",
            "picker.moveViewport.firstRow, maxFirstRow + 1",
            "picker.moveViewport.containsSelection(",
            "picker.moveViewport.reveal(focusedPos, count, visible);",
            "fb.setClipRect(",
            "appendClippedTouchButton(",
            "x + 24, viewportTop, panelW - 48, viewportH")
require_all("src/UI/Gen2PokemonPickerOverlay.inc",
            "appendClippedTouchButton(",
            "x + 20, listY - 5, panelW - 40, visible * rowStep + 8",
            "TouchPickerViewport locationViewport;",
            "picker.locationViewport.ensure(picker.model.index, pickerCount, visible);",
            "picker.locationViewport.firstRow, maxFirst + 1",
            "const int first = location ? overlay.locationViewport.firstRow :",
            "picker.locationViewport.reveal(picker.model.index, pickerCount, visible);",
            "!picker.locationViewport.containsSelection(",
            "Focused encounter brought into view; press A again or tap a visible row")
picker_hit = read("src/UI/Gen2PokemonPickerOverlay.inc")
assert "screen.touchButtons.push_back({2000 + i" not in picker_hit, (
    "Gen II picker should not publish unclipped hitboxes for hidden rows"
)
require_all("src/UI/TrainerViewScreenBase.inc",
            "std::abs(touch.deltaY()) >= 24 &&",
            "std::abs(touch.deltaY()) > std::abs(touch.deltaX()))")
# Two read-only report panels plus the detail side pane must reject sideways gestures.
trainer_scroll = read("src/UI/TrainerViewScreenBase.inc")
report_start = trainer_scroll.index("if (details.legalityOverlay) {")
report_end = trainer_scroll.index("// View is a genuinely read-only summary.", report_start)
report_gestures = trainer_scroll[report_start:report_end]
assert report_gestures.count(
    "std::abs(touch.deltaY()) > std::abs(touch.deltaX())") == 3, (
    "Legality, ribbons and details must each require vertical finger intent"
)
assert report_gestures.count("std::abs(touch.deltaY()) >= 24") == 3
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
            "gamesDrawerScroll, maxFirstRow + 1);",
            "scrollRow, maxFirstRow + 1);",
            "The viewport first row, not titleIndex, owns the physical scroll.",
            "profileTouchScroll.updateVertical(",
            "profilePickerScroll, maxFirstRow + 1);",
            "launchFileTouchScroll.updateVertical(",
            "legacyInstanceTouchScroll.updateVertical(",
            "gen4CandidateTouchScroll.updateVertical(",
            "Touch carousel gestures resolve through ScreenChrome")

# Gen III/IV picker scrolling owns an independent first-visible row, not the selected item.
# The renderer must keep that same first row throughout finger motion, even after crossing rows.
require_all("include/UI/TouchScroll.h",
            "struct TouchPickerViewport",
            "void ensure(int selected, int count, int visibleRows, int columns = 1)",
            "void reveal(int selected, int count, int visibleRows, int columns = 1)",
            "bool containsSelection(int selected, int count, int visibleRows,",
            "firstRow = std::clamp(firstRow, 0, maximum);")
for rel in ("src/UI/Gen3SharedPokemonSurface.inc",
            "src/UI/Gen4SharedPokemonSurface.inc"):
    source = read(rel)
    for token in (
        "TouchPickerViewport pickerViewport;",
        "state.pickerViewport.reset();",
        "state.pickerViewport.ensure(state.pickerRow, count, visibleRows, columns);",
        "state.pickerViewport.firstRow, maxFirstRow + 1);",
        "state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);",
        "const int first = state.pickerViewport.firstRow;",
        "const int firstRow = state.pickerViewport.firstRow;",
        "const int firstVisibleItem = state.pickerViewport.firstRow * HeldItemGrid::columns + 1;",
        "const int lastVisibleItem = std::min(",
        "if (maxFirstRow == 0)",
        "state.pickerViewport.containsSelection(",
        "state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);",
        "Focused choice brought into view; press A again or tap a visible row",
    ):
        if token not in source:
            raise AssertionError(f"{rel}: picker viewport/selection ownership regressed: {token}")
    value_input = source.split("bool handleValuePicker(", 1)[1].split("\n}", 1)[0]
    if "state.pickerRow, count);" in value_input or (
            "state.pickerRow, count, HeldItemGrid::columns);" in value_input):
        raise AssertionError(f"{rel}: pixel scrolling must not mutate selected picker row")

require_all("include/UI/TouchScroll.h",
            "class TouchScrollState",
            "offset_ += delta",
            "const int maxVelocity = std::max(12, std::min(96, step * 2));",
            "const int velocitySample = std::clamp(delta, -maxVelocity, maxVelocity);",
            "const int requested = (-offset_) / step",
            "const int available = std::max(0, (count - 1 - index) / stride)",
            "const bool atTrailingEdge = index + stride >= count",
            "coasting_ = std::abs(velocity_) >= 2")
if "liveHorizontalListVisual" in read("include/UI/TouchScroll.h"):
    raise AssertionError("TouchScroll: dead horizontal draw helper was reintroduced without a caller")
# Restarting contact during inertia must not reposition list contents mid-touch.
scroll_source = read("include/UI/TouchScroll.h")
touch_down_path = scroll_source.split("if (touch.justTouchedDown()) {", 1)[1].split(
    "if (active_ && touch.isDown())", 1)[0]
tap_release_path = scroll_source.split("if (active_ && touch.justReleased()) {", 1)[1].split(
    "coasting_ = std::abs(velocity_)", 1)[0]
if "offset_ = 0;" in touch_down_path or "offset_ = 0;" in tap_release_path:
    raise AssertionError("TouchScroll: new contact or a tap must not snap existing residual pixels")
require_all("include/UI/TouchScroll.h",
            "const bool reversed =",
            "(velocity_ < 0 && velocitySample > 0)",
            "(velocity_ > 0 && velocitySample < 0)",
            "velocity_ = reversed ? velocitySample : std::clamp(")
require_all("src/UI/Gen2HardwarePickerFix.inc",
            "picker.moveScroll.updateVertical(",
            "touch, listX, listY, listW, listH, kHardwareMovePickerLayout.rowStep,",
            "Focused move brought into view; press A again or tap a visible row")
gen2_move_overlay = read("src/UI/Gen2PokemonPickerOverlay.inc")
gen2_teardown = """        if (picker.model.active()) picker.model.close();
        picker.scroll.reset();
        picker.moveScroll.reset();
        picker.moveViewport.reset();
        picker.locationViewport.reset();
        return false;"""
assert gen2_move_overlay.count(gen2_teardown) == 2, (
    "Gen II picker View/None teardown must release all in-flight move/location motion"
)
require_all("src/UI/Gen2PokemonPickerOverlay.inc",
            "TouchScrollState moveScroll;",
            "TouchPickerViewport moveViewport;",
            "picker.moveScroll.reset();",
            "picker.moveViewport.reset();")
if "stepHardwareMovePicker(screen, state, picker, dy < 0" in read("src/UI/Gen2HardwarePickerFix.inc"):
    raise AssertionError("Gen II Move picker must never change chosen Move directly on drag release")
# A touch-down can become a drag. Wait for a clean release before changing the
# highlighted Gen II species, move or encounter and before synthesizing A.
for picker_path in ("src/UI/Gen2HardwarePickerFix.inc",
                    "src/UI/Gen2PokemonPickerOverlay.inc"):
    picker_source = read(picker_path)
    if "screen.touchedButtonDownId(touch)" in picker_source:
        raise AssertionError(f"{picker_path}: picker selection may change on drag contact")
    require_all(picker_path, "screen.touchedButtonId(touch)",
                "down |= HidNpadButton_A;")
require_all("src/UI/Gen2HardwareWorkspaceFix.inc",
            "const bool started = focusUnifiedTouchPoint(",
            "screen, touch.startX(), touch.startY(), false, down",
            "const bool ended = focusUnifiedTouchPoint(screen, touch.x(), touch.y(), false, down);",
            "startFocus.panel == endFocus.panel",
            "startFocus.row == endFocus.row",
            "startFocus.column == endFocus.column",
            "startMoveRow == endMoveRow",
            "startProgressionRow == endProgressionRow",
            "if (sameTarget)")
# Gen II Details/review panels only consume vertical drags. A sideways swipe that crossed
# touch slop must never become an upward row jump through max(1, abs(dy) / rowH).
gen2_workspace_touch = read("src/UI/Gen2HardwareWorkspaceFix.inc")
if gen2_workspace_touch.count(
    "std::abs(touch.deltaY()) > std::abs(touch.deltaX())") < 2:
    raise AssertionError("Gen II details/review: both drag paths need vertical direction gating")

gsc_editor = read("src/UI/TrainerViewScreenGSCOverlay.inc")
require_all("src/UI/TrainerViewScreenGSCOverlay.inc",
            "const auto tappedHit = [&](const auto& hit)",
            "touch.startX() >= hit.x", "touch.startY() >= hit.y",
            "std::abs(touch.deltaY()) >= 24",
            "std::abs(touch.deltaY()) > std::abs(touch.deltaX())")
if gsc_editor.count("if (tappedHit(hit))") != 3:
    raise AssertionError("GSC staged editor: all three direct-row actions must require same-target taps")
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
for dead_field in ("released", "dragged", "int x =", "int y ="):
    if dead_field in gesture:
        raise AssertionError(f"TouchGesture: unused renderer snapshot field reintroduced: {dead_field}")
require_all("include/UI/TouchGesture.h",
            "int deltaX = 0;", "permitsVerticalPreview(")
require_all("src/UI/TouchInput.cpp", "gLatestTouch.deltaX = curX - begX;")
require_all("include/UI/SharedSpeciesPicker.h", "permitsVerticalPreview(*gesture)")
scroll = read("include/UI/TouchScroll.h")
for dead_api in ("updateHorizontal(", "dragging() const", "moving() const", "bool tracking"):
    if dead_api in scroll:
        raise AssertionError(f"TouchScroll: unused API/state reintroduced: {dead_api}")
# A clean tap must begin AND end inside one rendered target. This guards adjacent
# footer/cards/list rows against accidental activation during a sub-slop finger wobble.
require_all("src/UI/SaveSelectScreen.cpp",
            "auto tappedHit = [&](const HitRect& r) -> bool",
            "touch.startX() >= r.x", "touch.startY() >= r.y",
            "touch.x() >= r.x", "touch.y() >= r.y",
            "if (tappedHit(r))",
            "const int idx = tappedRect(titleRects);",
            "const int idx = tappedRect(overlayRects);")
require_all("src/UI/SaveSelectScreen.cpp",
            "void SaveSelectScreen::syncTouchScrollSurface()",
            "launchFileTouchScroll.reset();",
            "gamesDrawerTouchScroll.reset();",
            "classicGamesTouchScroll.reset();",
            "syncTouchScrollSurface();")
require_all("src/UI/AppShellScreen.cpp",
            "auto tappedHit = [&](const HitRect& rect)",
            "contains(rect, touch.startX(), touch.startY())",
            "if (tappedHit(rect))")
for surface in (
    "launchFileScroll, maxFirstRow + 1);",
    "legacyAssignmentScroll, maxFirstRow + 1);",
    "legacyInstanceScroll, maxFirstRow + 1);",
    "gen4CandidateScroll, maxFirstRow + 1);",
):
    if surface not in read("src/UI/SaveSelectScreen.cpp"):
        raise AssertionError(f"SaveSelect: list scrolling must advance viewport, not selected save: {surface}")
require_all("src/UI/SaveSelectScreen.cpp",
            "void pushViewportHit(std::vector<Hit>& hits,",
            "const int right = std::min(x + w, clipX + clipW);",
            "const int bottom = std::min(y + h, clipY + clipH);",
            "pushViewportHit(titleRects,",
            "pushViewportHit(overlayRects,")
save_select_hit_source = read("src/UI/SaveSelectScreen.cpp")
if save_select_hit_source.count("pushViewportHit(") < 8:
    raise AssertionError("SaveSelect: scrollable list hitboxes must be clipped to visible viewports")
# Backup tile hits use the current pixel-scrolled tile geometry, then require
# contact AND release inside that same clipped tile before issuing A. Unlike
# the older visIdx/startedRow check, this also rejects taps in inter-row gaps.
require_all("src/UI/BackupSelectionScreen.cpp",
            "backupScroll.stop();",
            "showDeleteConfirmation = true;",
            "touch.startX() >= b.x", "touch.startY() >= b.y",
            "const int liveOffset = backupScroll.offset();",
            "const int top = std::max(listY, itemY);",
            "const int bottom = std::min(listBottom, itemY + LIST_ROW_H - 10);",
            "insideTile(touch.startX(), touch.startY())",
            "insideTile(touch.x(), touch.y())")
backup_touch = read("src/UI/BackupSelectionScreen.cpp")
assert "if (touch.justTapped())" in backup_touch
assert "insideList(touch.startX(), touch.startY())" not in backup_touch
# Moving the viewport must never steal selection or make a delete target a
# different backup. The action must first reveal its original focus.
scroll_start = backup_touch.index("backupScroll.updateVertical(")
scroll_end = backup_touch.index("if (touch.justTapped())", scroll_start)
assert "selectedIndex = backupFirstRow" not in backup_touch[scroll_start:scroll_end]
assert "selectedIndex = std::min(count - 1, backupFirstRow" not in backup_touch[scroll_start:scroll_end]
require_all("src/UI/BackupSelectionScreen.cpp",
            "selectedIndex < backupFirstRow ||",
            "selectedIndex >= backupFirstRow + LIST_MAX_VISIBLE",
            "selectedIndex - LIST_MAX_VISIBLE / 2",
            "Focused backup brought into view; press A or X again")
# Delete confirmation has exclusive touch ownership: only its displayed,
# same-target A/B rectangles can produce actions, not the footer behind it.
require_all("src/UI/BackupSelectionScreen.cpp",
            "const u64 footerTouch = showDeleteConfirmation ? 0 : navTouchButton(touch);",
            "if (showDeleteConfirmation) {",
            "if (in(deleteDeleteBtn))",
            "else if (in(deleteCancelBtn))")

# All generations share one clipped Species-picker gesture origin. Horizontal drags
# and drags starting on the preview panel cannot advance the species on release.
require_all("include/UI/SharedSpeciesPicker.h",
            "inline bool gestureStartedInList(int sx, int sy",
            "sy >= modalY + listY && sy < modalY + listY + visible * rowStep")
for rel in (
    "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
    "src/UI/Gen2PokemonPickerOverlay.inc",
    "src/UI/Gen3SharedPokemonSurface.inc",
    "src/UI/Gen4SharedPokemonSurface.inc",
):
    require_all(rel,
                "SharedSpeciesPicker::gestureStartedInList(touch.startX(), touch.startY())",
                "std::abs(dy) >= 24 && std::abs(dy) > std::abs(touch.deltaX())")
require_all("include/UI/SharedSpeciesPicker.h",
            "const TouchGestureSnapshot* gesture = nullptr",
            "gesture->deltaY / rowStep",
            "liveOffset = gesture->deltaY + appliedRows * rowStep",
            "fb.setClipRect(listX, listY, listW, listH - 2)")
if "latestTouchGesture()" in read("include/UI/SharedSpeciesPicker.h"):
    raise AssertionError("SharedSpeciesPicker: renderer must receive touch explicitly, not pull global state")

print(f"touch app-wide surface inventory: PASS ({len(SURFACES)} surface families)")
