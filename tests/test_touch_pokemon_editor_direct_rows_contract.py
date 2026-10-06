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
        "touch.justReleased() && touch.dragged()",
        "screen.touchButtons.push_back({3000 + i")

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
        "screen.touchButtons.push_back({4000 + i",
        "screen.touchButtons.push_back({31500 + speciesStart + i",
        "actionDown >= 31600 && actionDown < 31600 + static_cast<int>(actions.count)",
        "actionTap >= 31600 && actionTap < 31600 + static_cast<int>(actions.count)",
        "screen.touchButtons.push_back({31600 + static_cast<int>(i)",
        "state.reviewScroll = std::clamp",
        "touch.justReleased() && touch.dragged()",
        "SharedPokemonShell::drawVerticalScrollIndicator")

# Gen III's accepted .inc helper returns true for any direct-row id, even if the exact-format
# adapter refuses to move focus because that row is read-only. The composite include guard must
# therefore reject those non-editable direct taps before the caller can synthesize A against the
# previously focused field. Passive View remains allowed to focus informational rows.
composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp")
require(composite, "src/UI/TrainerViewScreenCompositeOverlay.cpp",
        "#define applyDirectFocus(idArg)",
        "passiveView || detailEditable(state, (idArg) - 31000)",
        "passiveView || pidLinkedRowEditable(state, (idArg) - 31200)",
        "? false : applyDirectFocus(idArg)",
        '#include "Gen3SharedPokemonSurface.inc"',
        "#undef applyDirectFocus")

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
        "screen.touchButtons.push_back({6000 + i",
        "screen.touchButtons.push_back({41000 + row",
        "screen.touchButtons.push_back({41100 + row",
        "screen.touchButtons.push_back({41120 + row",
        "screen.touchButtons.push_back({41300 + row",
        "actionDown >= 41600 && actionDown < 41600 + static_cast<int>(actions.count)",
        "actionTap >= 41600 && actionTap < 41600 + static_cast<int>(actions.count)",
        "screen.touchButtons.push_back({41600 + static_cast<int>(i)",
        "touch.justReleased() && touch.dragged()")

print("touch Pokémon editor direct-row/action contract: PASS")
