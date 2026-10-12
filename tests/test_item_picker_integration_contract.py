#!/usr/bin/env python3
"""Fast regression that all Gen I-IV item surfaces use native ID + game evidence.

The C++ test_item_picker_sort.cpp checks actual sorting, selected ID retention,
case folding and original vector immutability. This test ensures each real
hardware/menu surface WIREs that tested behavior and real machine art.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
def source(path):
    return (ROOT / path).read_text(encoding="utf-8")

catalog=source("src/Inventory/ClassicInventoryCatalog.cpp")
classic=source("src/UI/ClassicInventoryOverlay.cpp")
gen2=source("src/UI/Gen2PokemonEditorFoundation.inc")
gen3=source("src/UI/Gen3SharedPokemonSurface.inc")
gen4=source("src/UI/Gen4SharedPokemonSurface.inc")
art=source("include/UI/ClassicMachineArtwork.h")
sort=source("include/UI/ItemPickerSort.h")

assert "pocket == ClassicPocket::PCItems" in catalog
assert "526,241" not in catalog  # Incorrect later-generation Electric TM.
assert "263,521" not in catalog  # Incorrect later-generation Fire TM.
assert "503,315" not in catalog  # Incorrect later-generation Water TM.
assert "249,127,431," not in catalog  # Gen III HM08 is Dive, 291.
assert "classical" not in art  # Do not silently assign a generic placeholder.

# Every user-facing Add/Held list has an X switch and native-order snapshot.
for name,body,native in [
    ("Gen I-III Add/PC",classic,"pickerNativeItems"),
    ("Gen II Held",gen2,"itemNativeChoices"),
    ("Gen III Held",gen3,"pickerNativeValues"),
    ("Gen IV Held",gen4,"pickerNativeValues"),
    ("Gen IV Add",gen4,"itemAddNativeChoices"),
]:
    assert native in body, (name,"native sort snapshot missing")
    assert "ItemPickerSort::reorder(" in body, (name,"reorder not wired")
    assert "ItemPickerSort::next(" in body, (name,"sort X cycle missing")
    assert '{"X", "Sort"}' in body or '{"X","Sort"}' in body, (name,"controller help missing")
    assert "ItemPickerSort::label(" in body, (name,"sort label missing")

# Original game item IDs must be mapped to move type before asking for art.
assert "ClassicMachineArtwork::artKey(group, itemId)" in classic
assert "ItemPickerArtwork::draw(fb, x + width - 31" in classic
assert "ClassicMachineArtwork::artKey(" in gen3
assert "ClassicMachineArtwork::artKey(" in gen4
assert "ItemPickerArtwork::drawNativeItem(" in gen4

# PC / Gen IV Add labels must show the taught move, not just "TM43".
assert "classicMachineMove(game, itemId)" in catalog
assert "Names::machineDisplayLabel(" in gen4
assert 'std::string("Supported native items | Sort: ")' in classic
assert "itemAddPickerActive" in gen4

print("Gen I-IV item/PC/held machine art, move labels and sort wiring: PASS")
