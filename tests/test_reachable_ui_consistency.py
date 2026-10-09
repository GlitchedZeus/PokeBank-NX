#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

held = read("include/UI/Gen2HeldItemPicker.h")
assert "Colors::SurfaceSelected" in held
# After app-wide item picker contrast polish, all row labels use the theme-aware
# foreground text (the selection itself uses the contrasted surface/background).
assert "Colors::Text, TextStyle::Caption" in held
assert "ItemPickerArtwork::draw(" in held

picker = read("src/UI/Gen2PokemonPickerOverlay.inc")
assert "selected ? Colors::SelectedText : textColor" in picker
assert "selected ? Colors::Accent : textColor" not in picker

unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc")
assert '{{"Up/Down", "Field"}' not in unified
assert '{{"D-pad", "Navigate"}' not in unified
assert '{{"D-pad/Stick", "Field"}' in unified

parity = read("src/UI/Gen2SharedSurfaceParity.inc")
assert '{{"D-pad", "Navigate"}' not in parity
assert '{{"D-pad/Stick", "Navigate"}' in parity

save_confirm = read("src/UI/Dialogs/SaveConfirmDialog.cpp")
assert "Up/Down: Choose" not in save_confirm
assert "D-pad/Stick: Choose" in save_confirm

save_select = read("src/UI/SaveSelectScreen.cpp")
assert '{{"Up/Down", "Choose"}' not in save_select
assert '{{"Up/Down", "Choose Save"}' not in save_select
assert '{{"D-pad/Stick", "Choose"}' in save_select
assert '{{"D-pad/Stick", "Choose Save"}' in save_select

print("reachable UI consistency contract PASS")
