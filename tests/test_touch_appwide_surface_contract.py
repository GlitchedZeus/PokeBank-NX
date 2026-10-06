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
    "product home / selected-game card / feature placeholders": (
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
    "loaded-game home destinations": (
        "src/UI/Panels/HomeMenuPanel.cpp",
        ("screen.touchButtons.clear()",
         "screen.touchButtons.push_back({ 100 + p.idx",
         "screen.touchButtons.push_back({ 100 + ic.idx"),
    ),
    "loaded-game home input": (
        "src/UI/TrainerViewScreenBase.inc",
        ("homeTap >= 100 && homeTap <= 104",
         "homeMenuIndex = homeTap - 100",
         "kDown |= HidNpadButton_A"),
    ),
    "loaded-save party cards": (
        "src/UI/TrainerViewScreenCompositeOverlay.cpp",
        ("constexpr int kPartyTouchBase = 6200",
         "publishBasePartyTouchTargets",
         "handleBasePartyTouch",
         "screen.selectedPartyIndex = downId - kPartyTouchBase",
         "screen.touchedButtonId(touch)",
         "PokeVault::UIModel::PokemonLocation::Party"),
    ),
    "modern boxes draw targets": (
        "src/UI/Panels/BoxPokemonPanel.cpp",
        ("screen.touchButtons.push_back({ kBoxNameId",
         "screen.touchButtons.push_back({ kPrevBoxId",
         "screen.touchButtons.push_back({ kNextBoxId",
         "screen.touchButtons.push_back({ slotIndex",
         "screen.touchButtons.push_back({ 2000"),
    ),
    "modern boxes input": (
        "src/UI/TrainerViewScreenBase.inc",
        ("tapped == 1000", "tapped == 1001", "tapped == 1002",
         "tapped == 2000",
         "tapped >= 0 && tapped < static_cast<int>(trainer.getSlotsPerBox())",
         "if (selectedItemIndex == tapped) kDown |= HidNpadButton_A"),
    ),
    "dual-pane storage draw targets": (
        "src/UI/Panels/StoragePanel.cpp",
        ("storageTouchTargets.clear()",
         "storageTouchTargets.push_back({savePane ? 0 : 1, boxIndex, -2",
         "storageTouchTargets.push_back({savePane ? 0 : 1, boxIndex, -3",
         "storageTouchTargets.push_back({savePane ? 0 : 1, boxIndex, -4",
         "storageTouchTargets.push_back({thisPane, boxIndex, i"),
    ),
    "dual-pane storage input": (
        "src/UI/TrainerViewScreenBase.inc",
        ("for (const auto& t : storageTouchTargets)",
         "if (touch.justPressed())",
         "if (t.slot == -2)",
         "else if (t.slot == -3)",
         "else if (t.slot == -4)",
         "kDown |= HidNpadButton_A"),
    ),
    "trainer viewer/editor and retained loaded settings": (
        "src/UI/TrainerViewScreenBase.inc",
        ("screen.touchButtons.push_back({ i, cardX, cy, cardW, rowH })",
         "tb >= 0 && tb < kEditRows",
         "trainerSelectedRow = tb",
         "screen.touchButtons.push_back({ i, rx, ry, rowW, rowH })",
         "st >= 0 && st < kSettingsRows",
         "settingsSelectedRow = st"),
    ),
    "settings / themes / diagnostics / about / more": (
        "src/UI/AppShellScreen.cpp",
        ("Overlay::Settings", "settingsCategoryRects", "settingsRects",
         "Overlay::Diagnostics", "Overlay::More", "overlayRects",
         "touch.justTouchedDown()", "touch.justTapped()"),
    ),
    "organization previews / Pokédex placeholder": (
        "src/UI/AppShellScreen.cpp",
        ("Overlay::OrganizationPreview",
         "OrganizationPreviewKind::Banks",
         "OrganizationPreviewKind::Collections",
         "OrganizationPreviewKind::Search",
         "overlayRects",
         "touch.justTouchedDown()",
         "touch.justTapped()",
         "touch.justReleased() && touch.dragged()"),
    ),
    "items / inventory": (
        "src/UI/ClassicInventoryOverlay.cpp",
        ("TouchInput& touch", "screen.touchButtons.push_back",
         "touch.justTouchedDown()", "touch.justReleased() && touch.dragged()"),
    ),
    "modern value picker": (
        "src/UI/Dialogs/PickerDialog.cpp",
        ("screen.touchButtons.clear()",
         "screen.touchButtons.push_back({ idx",
         "const bool reorder =",
         "screen.pickerOrder[idx]",
         "drawScrollbar",
         "drawNavBar"),
    ),
    "item edit / remove dialogs": (
        "src/UI/Dialogs/ItemEditDialog.cpp",
        ("screen.touchButtons.push_back({40",
         "drawEditStepRow",
         "\"B\", \"Cancel\",  0",
         "\"Y\", \"Remove\",  2",
         "\"A\", \"Confirm\", 1",
         "screen.touchButtons.push_back({0",
         "screen.touchButtons.push_back({1"),
    ),
    "stat edit dialog": (
        "src/UI/Dialogs/StatEditDialog.cpp",
        ("screen.touchButtons.push_back({10",
         "screen.touchButtons.push_back({11",
         "drawEditStepRow",
         "\"B\", \"Cancel\", 0",
         "\"A\", \"Save\", 1"),
    ),
    "custom keyboard": (
        "src/UI/Dialogs/KeyboardDialog.cpp",
        ("if (touch.justPressed())",
         "isInside(touch.x(), touch.y(), key.keyX, key.keyY, key.keyWidth, key.keyHeight)",
         "selectedKeyIndex = keyIndex",
         "if (deferredWhenTapped(key.action))",
         "pendingKeyIndex = keyIndex",
         "isInside(touch.x(), touch.y(), fieldX, fieldY, fieldWidth, fieldHeight)"),
    ),
    "shared action / group / exit popup rows": (
        "src/UI/Panels/StoragePanel.cpp",
        ("void drawPopupMenu",
         "screen.touchButtons.clear()",
         "if (!disabled)",
         "screen.touchButtons.push_back({i",
         "void drawPokemonActionSheet",
         "void drawStorageGroupMenu",
         "void drawStorageExitConfirm"),
    ),
    "storage confirmation buttons": (
        "src/UI/Panels/StoragePanel.cpp",
        ("void drawStorageReleaseConfirm",
         "void drawCreatorKeepConfirm",
         "void drawDetailsDiscardConfirm",
         "void drawGen3ConvertConfirm",
         "void drawLgpeTransferConfirm",
         "screen.touchButtons.push_back({0",
         "screen.touchButtons.push_back({1",
         "screen.touchButtons.push_back({2"),
    ),
    "popup and confirmation input consumers": (
        "src/UI/TrainerViewScreenBase.inc",
        ("if (actionSheet.isOpen())",
         "actionSheet.select(tb)",
         "if (groupMenuActive)",
         "groupMenuIndex = tb",
         "if (storageExitConfirmActive)",
         "storageExitConfirmIndex = tb",
         "if (releaseConfirmActive)",
         "if (tb == 1) kDown |= HidNpadButton_A",
         "gen3ConvertConfirmActive = false",
         "lgpeTransferConfirmActive = false",
         "if (itemEditDialogActive)",
         "case 40: kDown |= HidNpadButton_X",
         "if (statEdit.dialogActive)",
         "case 10: kDown |= HidNpadButton_Up",
         "case 11: kDown |= HidNpadButton_Down"),
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
    "backup delete confirmation": (
        "src/UI/BackupSelectionScreen.cpp",
        ("if (showDeleteConfirmation)",
         "if (touch.justPressed())",
         "if (in(deleteDeleteBtn))      kDown |= HidNpadButton_A;",
         "else if (in(deleteCancelBtn)) kDown |= HidNpadButton_B;",
         "\"B\", \"Cancel\"",
         "\"A\", \"Delete\""),
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
    "Gen I passive read-only View": (
        "src/UI/Gen1PokemonDetailsPresentation.cpp",
        ("screen.touchButtons.clear()",
         "drawNavBar(fb, {{\"B\", \"Back\"}})"),
    ),
    "Gen II passive native-data View draw": (
        "src/UI/Modals/Gen2PokemonDetailsModal.cpp",
        ("screen.details.leftScroll",
         "fb.setClipRect",
         "fb.clearClip()",
         "drawScrollbar",
         "drawNavBar(fb, {{\"B\", \"Back\"}})"),
    ),
    "Gen II passive native-data View input": (
        "src/UI/TrainerViewScreenBase.inc",
        ("const bool gen2Passive =",
         "if (touch.justReleased() && touch.dragged())",
         "const bool inGen2Data = gen2Passive",
         "details.leftScroll = std::max(0, details.leftScroll - touch.deltaY())",
         "details.leftScrollManual = true"),
    ),
    "legacy Gen II editor / review overlay": (
        "src/UI/TrainerViewScreenGSCOverlay.inc",
        ("void handleStagedEditorInput(TrainerViewScreen& screen, u64 down, const TouchInput& touch)",
         "touch.justTouchedDown()",
         "touch.justTapped()",
         "touch.justReleased() && touch.dragged()",
         "screen.touchButtons.push_back({800 + i",
         "screen.touchButtons.push_back({900 + i",
         "handleStagedEditorInput(*this, navigated, touch)"),
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
            "Content cards/rows are owned by the screen that draws them.",
            "constexpr int kLiveDragStep = 52",
            "if (contentDragMoved) return 0;",
            "constexpr int kContentSwipeDistance = 72",
            "g_quickGamesDrawerSwipe = true",
            "const bool profilePicker =",
            "const bool gameFilePicker =",
            "const bool saveInstanceList =",
            "const bool saveAssignmentList =",
            "g_contentSwipeMask = verticalDirections")

print(f"touch app-wide surface inventory: PASS ({len(SURFACES)} surface families)")
