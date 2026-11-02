#!/usr/bin/env python3
"""Round 6 E05: every footer and Help key maps to a real input handler.

Owner (Round 5, 1ca2faec, E05): controller glyphs missing or inconsistent, stale Help lines.
Rule: a footer badge or Help row may name a button only where that screen's input code reads
that button. This contract extracts every literal key/label pair from the footers and Help rows
of the screens in the sweep (HOME and game selection, Game Browser, Party, Boxes, Trainer, Bag,
Create, Edit, Review, Backups, menus and warnings) and requires, for each key, the matching
HidNpadButton_* (or the screen's own input enum) in that screen's handler sources. It also
rejects a duplicate key within one footer and a key with a blank description.

It is a static source contract: the Switch build decides what compiles; this stops a footer or
Help row naming a button that its screen never handles.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Screen -> (files whose footers / Help rows are checked, files that hold that screen's input).
# A footer drawn in one layer is handled by the screen's input layer, so both are listed.
SCREENS = {
    "HOME / game selection / Game Browser": (
        ["src/UI/SaveSelectScreen.cpp"],
        ["src/UI/SaveSelectScreen.cpp"]),
    # Round 11 H39: the Import from file dialog (every Gen I-V session) names '-' Cancel.
    "Import from file dialog": (
        ["src/UI/PokemonFileDialog.cpp"],
        ["src/UI/PokemonFileDialog.cpp", "include/UI/PokemonFileDialogModel.h"]),
    "App shell / Settings": (
        ["src/UI/AppShellScreen.cpp"],
        ["src/UI/AppShellScreen.cpp"]),
    "Backups": (
        ["src/UI/BackupSelectionScreen.cpp"],
        ["src/UI/BackupSelectionScreen.cpp"]),
    "Game Browser Party / Boxes / Trainer / Bag (Gen I-IV)": (
        ["src/UI/TrainerViewScreenBase.inc", "include/UI/ClassicBrowseFooter.h", "src/UI/ClassicPartyActions.inc",
         "src/UI/ClassicPackedMoveOverlay.inc", "src/UI/ClassicReleaseActionFix.inc"],
        ["src/UI/TrainerViewScreenBase.inc", "src/UI/ClassicPartyActions.inc",
         "src/UI/ClassicPackedMoveOverlay.inc", "src/UI/ClassicReleaseActionFix.inc",
         "include/UI/ClassicTrainerSessionInput.h", "include/UI/ClassicStagedExit.h"]),
    "Bag (Gen I-III)": (
        ["src/UI/ClassicInventoryOverlay.cpp"],
        ["src/UI/ClassicInventoryOverlay.cpp", "include/UI/ClassicInventoryUIModel.h", "src/UI/ClassicInventoryUIModel.cpp"]),
    "Gen I Create / Edit / View": (
        ["src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc"],
        ["src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
         "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "src/UI/Gen1PokemonEditorOverlayUX.inc",
         "src/UI/Gen1PokemonEditorOverlayFoundation.inc", "src/UI/Gen1PokemonEditorOverlay.cpp"]),
    "Gen II Create / Edit / View / Review": (
        ["src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc",
         "src/UI/Gen2SharedSurfaceParity.inc", "src/UI/Gen2PokemonPickerOverlay.inc",
         "src/UI/Gen2HardwarePickerFix.inc", "include/UI/Gen2WorkspaceLayers.h"],
        ["src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc",
         "src/UI/Gen2SharedSurfaceParity.inc", "src/UI/Gen2PokemonPickerOverlay.inc",
         "src/UI/Gen2HardwarePickerFix.inc", "src/UI/Gen2PokemonEditorFoundation.inc",
         "src/UI/Gen2SharedPokemonSurface.inc", "include/UI/Gen2WorkspaceLayers.h",
         "include/UI/Gen2FieldSelector.h", "include/UI/SharedSpeciesPicker.h"]),
    "Gen III-V shared Create / Edit / View / Review": (
        ["src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc",
         "src/UI/Gen4SharedPokemonSurface.inc", "src/UI/Gen5SharedReadOnlySurface.inc"],
        ["src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc",
         "src/UI/Gen4SharedPokemonSurface.inc", "src/UI/Gen5SharedReadOnlySurface.inc",
         "src/UI/ClassicPackedMoveOverlay.inc", "include/UI/ClassicStagedExit.h",
         "include/UI/SharedSpeciesPicker.h", "include/UI/SharedHeldItemSearch.h"]),
}

# Key token -> every handler pattern it needs (any one alternative per inner tuple).
def requirements(key: str):
    k = key.replace(" ", "")
    nav = ("HidNpadButton_Up", "HidNpadButton_Down", "controllerNavigation", "Navigation", "nav&", "nav &")
    table = {
        "A": [("HidNpadButton_A", "Input::A", "::A)")],
        "B": [("HidNpadButton_B", "Input::B", "::B)")],
        "X": [("HidNpadButton_X", "Input::X", "::X)")],
        "Y": [("HidNpadButton_Y", "Input::Y", "::Y)")],
        "L": [("HidNpadButton_L",)], "R": [("HidNpadButton_R",)],
        "L/R": [("HidNpadButton_L",), ("HidNpadButton_R",)],
        "ZL": [("HidNpadButton_ZL",)], "ZR": [("HidNpadButton_ZR",)],
        "ZL/ZR": [("HidNpadButton_ZL",), ("HidNpadButton_ZR",)],
        "+": [("HidNpadButton_Plus", "Input::Plus")],
        "-": [("HidNpadButton_Minus", "Input::Minus")],
    }
    if k in table:
        return table[k]
    if k in ("D-pad", "D-Pad", "D-pad/Stick", "Arrows", "Up/Down", "Left/Right", "D-pad/LeftStick"):
        return [nav]
    return None  # not a controller key (a state marker or touch hint): nothing to map


PAIR = re.compile(r'\{\s*"([^"]{1,16})"\s*,\s*"([^"]*)"\s*\}')
PUSH = re.compile(r'\.push(?:_back)?\(\s*\{?\s*"([^"]{1,16})"\s*,\s*"([^"]*)"\s*\}?\s*\)')
STRING_FOOTER = re.compile(r'"((?:(?:Arrows|D-pad|D-Pad|Up/Down|Left/Right|[ABXYLR]|ZL|ZR|L/R|ZL/ZR|\+|-)'
                           r':\s*[^"|]*(?:\s*\|\s*)?){2,})"')


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def footer_calls(code: str):
    """Yield (line, [(key, label), ...]) for each footer / help-row construct."""
    for m in re.finditer(r"\b(drawNavBar|drawProductHelpOverlay)\s*\(", code):
        depth, i = 0, m.end() - 1
        while i < len(code):
            if code[i] == "(":
                depth += 1
            elif code[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        args = code[m.end():i]
        pairs = [(k, v) for k, v in PAIR.findall(args) if requirements(k) is not None]
        for s in STRING_FOOTER.findall(args):
            pairs += string_pairs(s)
        if pairs:
            yield code.count("\n", 0, m.start()) + 1, pairs
    for m in PUSH.finditer(code):
        if requirements(m.group(1)) is not None:
            yield code.count("\n", 0, m.start()) + 1, [(m.group(1), m.group(2))]
    for m in re.finditer(r"instructions\s*(?:=|\+=)\s*", code):
        rest = code[m.end():m.end() + 600]
        for s in STRING_FOOTER.findall(rest.split(";")[0]):
            yield code.count("\n", 0, m.start()) + 1, string_pairs(s)


def string_pairs(s: str):
    out = []
    for seg in s.split("|"):
        seg = seg.strip()
        if ":" not in seg:
            continue
        key, label = seg.split(":", 1)
        key = key.strip()
        if requirements(key) is not None:
            out.append((key, label.strip()))
    return out


# Paint-only layers: they repaint a screen whose input lives in the listed editor layers, so
# their footer keys are checked against those layers. Every other footer file must handle its own
# keys (strict per file), and every footer file must also be covered by its screen's handlers.
PRESENTATION_LAYERS = {
    "include/UI/ClassicBrowseFooter.h": ["src/UI/TrainerViewScreenBase.inc"],
    "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc": [
        "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen1PokemonEditorOverlayFoundation.inc"],
    # Round 6 (lane L): Cleanup3's ux3DrawTouchNavBar also paints the Gen I Create / Edit workspace
    # footer (UX2Mode::Edit / AddDraft), whose input is foundationHandleMainInput (Foundation.inc);
    # its picker footers are read by Cleanup3 itself.
    "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc": [
        "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen1PokemonEditorOverlayFoundation.inc"],
    "src/UI/Gen2HardwareFinalFix.inc": [
        "src/UI/Gen2SharedSurfaceParity.inc", "src/UI/Gen2UnifiedPokemonWorkspace.inc"],
}


def source(rel: str) -> str:
    return strip_comments((ROOT / rel).read_text(encoding="utf-8"))


def check(rel: str, screen: str, code: str, own: str, screen_code: str):
    violations, checked = [], 0
    for line, pairs in footer_calls(code):
        seen = set()
        for key, label in pairs:
            checked += 1
            norm = key.replace(" ", "")
            if not label.strip():
                violations.append(f"{rel}:{line}: key {key!r} has a blank description")
            if norm in seen:
                violations.append(f"{rel}:{line}: key {key!r} appears twice in one footer")
            seen.add(norm)
            for alternatives in requirements(key):
                for scope, text in (("its own input layer", own), (screen, screen_code)):
                    if not any(a in text for a in alternatives):
                        violations.append(f"{rel}:{line}: footer/Help names {key!r} ({label!r}) "
                                          f"but {scope} never reads {alternatives[0]}")
    return checked, violations


def self_test() -> None:
    """The contract must fail on the defects it exists to catch."""
    handled = "if (down & HidNpadButton_A) {} if (down & HidNpadButton_B) {} controllerNavigation.apply();"
    cases = {
        'drawNavBar(fb, {{"A", "Select"}, {"ZL", "Jump"}, {"B", "Back"}});': "never reads HidNpadButton_ZL",
        'drawNavBar(fb, {{"A", "Select"}, {"B", "Back"}, {"B", "Cancel"}});': "appears twice",
        'drawNavBar(fb, {{"A", ""}, {"B", "Back"}});': "blank description",
        'drawNavBar(fb, "Arrows: Navigate  |  X: Save  |  B: Back");': "never reads HidNpadButton_X",
    }
    for code, expected in cases.items():
        _, found = check("mutant", "mutant", code, handled, handled)
        assert any(expected in v for v in found), (code, found)
    _, clean = check("clean", "clean", 'drawNavBar(fb, {{"D-pad", "Move"}, {"A", "Select"}, {"B", "Back"}});',
                     handled, handled)
    assert not clean, clean


# Round 7 (B04): the ONE Inventory Help (include/UI/InventoryHelp.h, "Inventory Controls") serves the
# Gen I-V Bag. Every key its rows name must be read by EACH generation's Bag input code.
INVENTORY_HELP = "include/UI/InventoryHelp.h"
BAG_HANDLERS = {
    "Gen I-III Bag": ["src/UI/ClassicInventoryOverlay.cpp", "src/UI/ClassicInventoryUIModel.cpp"],
    "Gen IV Bag": ["src/UI/TrainerViewScreenBase.inc", "src/UI/Gen4SharedPokemonSurface.inc"],
    "Gen V Bag": ["src/UI/Gen5SharedReadOnlySurface.inc", "include/Integration/Gen5/Gen5BagScreen.h"],
}
HELP_ROW = re.compile(r'"([^"]+?)(?:\s{2,})[^"]*"')


def help_row_keys(code: str):
    """Yield (line, key) for every "KEYS   description" row literal (groups split by 5+ spaces)."""
    for m in re.finditer(r'"([^"\n]*)"', code):
        literal = m.group(1)
        line = code.count("\n", 0, m.start()) + 1
        for group in re.split(r" {5,}", literal):
            km = re.match(r"(.+?) {2,}\S", group)
            if not km:
                continue
            for token in km.group(1).split("/"):
                token = token.strip()
                if token and requirements(token) is not None:
                    yield line, token


def check_inventory_help(help_code: str, handlers: dict) -> tuple[int, list]:
    violations, checked = [], 0
    for line, key in help_row_keys(help_code):
        for bag, code in handlers.items():
            checked += 1
            for alternatives in requirements(key):
                if not any(a in code for a in alternatives):
                    violations.append(f"{INVENTORY_HELP}:{line}: Inventory Help names {key!r} but the {bag} "
                                      f"input never reads {alternatives[0]}")
    return checked, violations


def main() -> int:
    self_test()
    # The Bag Help check must fail on a row naming a key some Bag never reads.
    _, bad = check_inventory_help('"ZL   Jump pockets"', {"mutant Bag": "HidNpadButton_A"})
    assert any("never reads HidNpadButton_ZL" in v for v in bad), bad
    violations = []
    checked = 0
    for screen, (footers, handlers) in SCREENS.items():
        for rel in footers + handlers:
            if not (ROOT / rel).is_file():
                violations.append(f"{screen}: source missing: {rel}")
        if violations:
            continue
        screen_code = "".join(source(rel) for rel in handlers)
        for rel in footers:
            code = source(rel)
            own = code + "".join(source(h) for h in PRESENTATION_LAYERS.get(rel, []))
            count, found = check(rel, screen, code, own, screen_code)
            checked += count
            violations += found
    help_path = ROOT / INVENTORY_HELP
    if not help_path.is_file():
        violations.append(f"{INVENTORY_HELP}: missing")
    else:
        handlers = {bag: "".join(source(rel) for rel in rels) for bag, rels in BAG_HANDLERS.items()}
        count, found = check_inventory_help(source(INVENTORY_HELP), handlers)
        if count < 24:
            violations.append(f"{INVENTORY_HELP}: only {count} Help key checks; the row extractor is broken")
        checked += count
        violations += found
    for v in violations:
        print("VIOLATION", v)
    print(f"footer/Help keys checked: {checked}; violations: {len(violations)}")
    if checked < 300:
        print("VIOLATION: too few footer keys found; the extractor is broken")
        return 1
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main())
