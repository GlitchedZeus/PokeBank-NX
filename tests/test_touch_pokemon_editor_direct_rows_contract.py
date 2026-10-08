#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing touch contract source: {rel}")
    return path.read_text(encoding="utf-8")

def require(text: str, rel: str, *needles: str) -> None:
    for needle in needles:
        if needle not in text:
            raise AssertionError(f"{rel}: missing touch contract token: {needle}")

gen1_draw = read("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc")
gen1_input = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc")
require(gen1_draw, "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
        'leftLabels{"Species", "Nickname", "Level", "EXP", "OT", "Trainer ID"}',
        "screen.touchButtons.push_back({5000 + i",
        "screen.touchButtons.push_back({5100 + r * 10 + c",
        "screen.touchButtons.push_back({5190",
        "screen.touchButtons.push_back({5200 + i")
require(gen1_input, "src/UI/Gen1PokemonEditorOverlayFoundation.inc",
        "uint64_t foundationDirectTouchInput",
        "id >= 5000 && id < 5006",
        "Foundation::Panel::Identity",
        "id >= 5100 && id < 5150",
        "Foundation::Panel::Values",
        "id == 5190",
        "id >= 5200 && id < 5204",
        "Foundation::Panel::Moves",
        "if (tapped == 5900) down |= HidNpadButton_B;",
        "else if (tapped == 5901) down |= HidNpadButton_Y;",
        "else if (tapped == 5902) down |= HidNpadButton_A;")

classic_actions = read("src/UI/ClassicReleaseActionFix.inc")
require(classic_actions, "src/UI/ClassicReleaseActionFix.inc",
        "hit.id < 300 || hit.id >= 300 + rows",
        "touchedAction >= 300 && touchedAction < 300 + rows",
        "screen.touchButtons.push_back({300 + static_cast<int>(i)",
        "screen.touchButtons.push_back({398",
        "screen.touchButtons.push_back({399",
        "hit.id < 500 || hit.id >= 500 + static_cast<int>(actions.count)",
        "touchedAction >= 500 && touchedAction < 500 + static_cast<int>(actions.count)",
        "screen.touchButtons.push_back({500 + static_cast<int>(i)",
        "screen.touchButtons.push_back({498",
        "screen.touchButtons.push_back({499")

gen2 = read("src/UI/Gen2PokemonPickerOverlay.inc")
require(gen2, "src/UI/Gen2PokemonPickerOverlay.inc",
        "screen.touchedButtonDownId(touch)",
        "screen.touchedButtonId(touch)",
        "picker.model.index = std::clamp(touchDown - 2000, 1, 251) - 1;",
        "picker.model.index = std::clamp(touchTap - 2000, 1, 251) - 1;",
        "touch.justReleased() && touch.dragged()")
assert "std::clamp(touchDown - 2000, 1, 251);" not in gen2
assert "std::clamp(touchTap - 2000, 1, 251);" not in gen2

gen2_item = read("src/UI/Gen2PokemonEditorFoundation.inc")
require(gen2_item, "src/UI/Gen2PokemonEditorFoundation.inc",
        "screen.touchedButtonDownId(touch)",
        "screen.touchedButtonId(touch)",
        "state.itemChoiceIndex = touchTap - 3000;",
        "state.itemScroll.updateVertical(",
        "state.itemScroll.offset()",
        "TouchPickerViewport itemViewport;",
        "state.itemViewport.firstRow, maxFirstRow + 1",
        "state.itemViewport.reveal(state.itemChoiceIndex, count, Model::rows, Model::columns)",
        "state.itemViewport.containsSelection(",
        "const int firstRow = state.itemViewport.firstRow;",
        "appendClippedTouchButton(screen.touchButtons, 3000 + i")

# Finger-down may focus an item, but must not cancel the very same scrolling
# gesture that updateVertical just captured for a row-starting swipe.
gen2_contact = gen2_item.split("const int touchDown = screen.touchedButtonDownId(touch);", 1)[1].split(
    "const int touchTap = screen.touchedButtonId(touch);", 1)[0]
assert "state.itemChoiceIndex = touchDown - 3000;" in gen2_contact
assert "state.itemScroll.stop()" not in gen2_contact, (
    "Gen II picker: touch-down focus must preserve the active drag capture"
)

gen3 = read("src/UI/Gen3SharedPokemonSurface.inc")
for token in (
    "PickerTarget::HeldItem", "PickerTarget::Language", "PickerTarget::Ball",
    "PickerTarget::MetLocation", "PickerTarget::Nature", "PickerTarget::Ability",
    "PickerTarget::Move"
):
    require(gen3, "src/UI/Gen3SharedPokemonSurface.inc", token)
require(gen3, "src/UI/Gen3SharedPokemonSurface.inc",
        "touchDown >= 4000 && touchDown < 4000 + count",
        "touchTap >= 4000 && touchTap < 4000 + count",
        "state.pickerRow = touchTap - 4000;",
        "appendClippedTouchButton(screen.touchButtons, 4000 + i",
        "screen.touchButtons.push_back({31500 + speciesStart + i",
        "actionDown >= 31600 && actionDown < 31600 + static_cast<int>(actions.count)",
        "actionTap >= 31600 && actionTap < 31600 + static_cast<int>(actions.count)",
        "screen.touchButtons.push_back({31600 + static_cast<int>(i)",
        "state.reviewScroll = std::clamp",
        "state.reviewTouchScroll.updateVertical(",
        "state.reviewTouchScroll.offset()",
        "const auto detailVisual = liveVerticalListVisual(",
        "SharedPokemonShell::drawVerticalScrollIndicator")

# Gen III direct-row focus must reject read-only rows in the helper itself. Returning false is
# essential: the caller synthesizes A only when direct focus was actually accepted. Passive View
# still focuses informational rows, while Create/Edit cannot fall through to a stale prior field.
require(gen3, "src/UI/Gen3SharedPokemonSurface.inc",
        "if (!(passiveView || detailEditable(state, row))) return false;",
        "if (!(passiveView || pidLinkedRowEditable(state, row))) return false;")
composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp")
if "#define applyDirectFocus" in composite:
    raise AssertionError("Gen III touch guard regressed to a fragile include-time macro shim")

gen4 = read("src/UI/Gen4SharedPokemonSurface.inc")
for token in (
    "PickerTarget::Species", "PickerTarget::HeldItem", "PickerTarget::Language",
    "PickerTarget::Ball", "PickerTarget::MetLocation", "PickerTarget::Pokerus",
    "PickerTarget::Form", "PickerTarget::Move", "PickerTarget::Nature",
    "PickerTarget::Ability"
):
    require(gen4, "src/UI/Gen4SharedPokemonSurface.inc", token)
require(gen4, "src/UI/Gen4SharedPokemonSurface.inc",
        "touchDown >= 6000 && touchDown < 6000 + count",
        "touchTap >= 6000 && touchTap < 6000 + count",
        "state.pickerRow = touchTap - 6000;",
        "appendClippedTouchButton(screen.touchButtons, 6000 + i",
        "screen.touchButtons.push_back({41000 + row",
        "screen.touchButtons.push_back({41100 + row",
        "screen.touchButtons.push_back({41120 + row",
        "screen.touchButtons.push_back({41300 + row",
        "actionDown >= 41600 && actionDown < 41600 + static_cast<int>(actions.count)",
        "actionTap >= 41600 && actionTap < 41600 + static_cast<int>(actions.count)",
        "screen.touchButtons.push_back({41600 + static_cast<int>(i)",
        "const auto detailVisual = liveVerticalListVisual(",
        "state.pickerScroll.updateVertical(")


# Shared picker clipping must trim touch hitboxes to the same actual viewport
# used by framebuffer drawing, preventing hidden rows from becoming tappable.
chrome = read("include/UI/ScreenChrome.h")
require(chrome, "include/UI/ScreenChrome.h",
        "inline void appendClippedTouchButton(",
        "const int left = std::max(x, clipX);",
        "const int top = std::max(y, clipY);",
        "const int right = std::min(x + w, clipX + clipW);",
        "const int bottom = std::min(y + h, clipY + clipH);")
for rel in ("src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc"):
    src = read(rel)
    if src.count("appendClippedTouchButton(screen.touchButtons,") < 3:
        raise AssertionError(f"{rel}: picker move/item/value touch targets must be clipped")
    for expected in (
        "x + 24, viewportTop, w - 48, visible * moveLayout.rowStep",
        "gridX, gridY - 3, w - 44, HeldItemGrid::rows * cellHeight",
        "x + 22, viewportTop - 4, w - 44, visible * rowStep",
    ):
        assert expected in src, f"{rel}: picker hitbox viewport mismatch: {expected}"


# Fixed action/move dialogs are fully visible. They should remain direct-tap/controller surfaces,
# not pretend scroll views whose release position silently changes focus.
for rel, forbidden in (
    ("src/UI/Gen3SharedPokemonSurface.inc", (
        "state.moveEditorRow = std::clamp(state.moveEditorRow + (touch.deltaY()",
        "state.actionRow = std::clamp(state.actionRow + (touch.deltaY()",
    )),
    ("src/UI/Gen4SharedPokemonSurface.inc", (
        "state.moveEditorRow = std::clamp(state.moveEditorRow + (touch.deltaY()",
        "state.actionRow = std::clamp(state.actionRow + (touch.deltaY()",
    )),
):
    source = read(rel)
    for token in forbidden:
        if token in source:
            raise AssertionError(f"{rel}: non-overflowing fixed menu regained swipe-selection: {token}")

print("touch Pokémon editor direct-row/action contract: PASS")
