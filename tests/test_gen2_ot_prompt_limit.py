#!/usr/bin/env python3
"""Gen II OT prompts offer the 7-character player-name limit the staged editor enforces.

The Gen II staged editor encodes an OT with kInternationalTrainerNameMax (7). Three Gen II OT
prompts still offered 10 characters, so an 8-10 character OT was typed and then refused.
(PKSE 1.2.x audit, docs/audits/PKSE_1_2_X_GEN15_GAP_MATRIX.md.)
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
FILES = [
    "src/UI/Gen2UnifiedPokemonWorkspace.inc",
    "src/UI/Gen2PokemonEditorFoundation.inc",
    "src/UI/TrainerViewScreenGSCOverlay.inc",
]
# Round 15 (shared keyboard, owner: "Never truncate ... one shared keyboard"): every Gen II OT prompt
# is UI::ClassicJapaneseNames::promptName(screen.trainer, screen.sourceGameId,
# PokeBank::Text::FieldKind::OriginalTrainer, ...), whose cap comes from the field capability
# (Text/NameFields.cpp nameMaxCharacters: Gen I / II international OT = 7, Japanese 5), checked
# numerically by tests/test_text_field_capability_matrix.cpp. This test pins that wiring and rule.
PROMPT = re.compile(r'ClassicJapaneseNames::promptName\(\s*screen\.trainer\s*,\s*screen\.sourceGameId\s*,'
                    r'\s*PokeBank::Text::FieldKind::(\w+)\s*,\s*"(?:Gen II )?OT Name"', re.S)


def main() -> int:
    failures = []
    header = (ROOT / "include/Integration/Gen2/Gen2StagedEditor.h").read_text()
    if "inline constexpr std::size_t kInternationalTrainerNameMax = 7;" not in header:
        failures.append("kInternationalTrainerNameMax is no longer 7")
    if "kInternationalTrainerNameMax" not in (ROOT / "src/Integration/Gen2/Gen2StagedEditor.cpp").read_text().split(
            "bool StagedEditor::encodeOTName", 1)[1].split("}", 1)[0]:
        failures.append("encodeOTName no longer uses kInternationalTrainerNameMax")
    rule = (ROOT / "src/Text/NameFields.cpp").read_text()
    gen12 = rule.split("case Format::Gen1:", 1)[1].split("case Format::Gen3:", 1)[0]
    if "if (c.japaneseLayout) return 5;" not in gen12 or "return pokemonName ? 10 : 7;" not in gen12:
        failures.append("NameFields.cpp no longer caps Gen I / II OT names at 7 (Japanese 5)")
    seen = 0
    for name in FILES:
        text = (ROOT / name).read_text()
        for kind in PROMPT.findall(text):
            seen += 1
            if kind != "OriginalTrainer":
                failures.append(f"{name}: OT prompt uses field kind {kind!r}")
    if seen != 3:
        failures.append(f"expected 3 Gen II OT prompts, found {seen}")
    if failures:
        print("Gen II OT prompt limit: FAIL")
        for failure in failures:
            print("  " + failure)
        return 1
    print("Gen II OT prompt limit matches the staged editor (7): PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
