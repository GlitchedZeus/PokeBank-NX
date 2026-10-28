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
# Owner Oct 28: the prompt may go through UI::ClassicJapaneseNames::prompt(screen.trainer, ...), which
# opens the kana keyboard on Japanese saves and is the ordinary promptText otherwise; same limit rule.
PROMPT = re.compile(r'(?:promptText\(|ClassicJapaneseNames::prompt\(\s*screen\.trainer\s*,)'
                    r'\s*"(?:Gen II )?OT Name"\s*,\s*"([^"]*)"\s*,\s*[^,]+,\s*([^;]+?)\)\s*;', re.S)


def main() -> int:
    failures = []
    header = (ROOT / "include/Integration/Gen2/Gen2StagedEditor.h").read_text()
    if "inline constexpr std::size_t kInternationalTrainerNameMax = 7;" not in header:
        failures.append("kInternationalTrainerNameMax is no longer 7")
    if "kInternationalTrainerNameMax" not in (ROOT / "src/Integration/Gen2/Gen2StagedEditor.cpp").read_text().split(
            "bool StagedEditor::encodeOTName", 1)[1].split("}", 1)[0]:
        failures.append("encodeOTName no longer uses kInternationalTrainerNameMax")
    seen = 0
    for name in FILES:
        text = (ROOT / name).read_text()
        for guide, limit in PROMPT.findall(text):
            seen += 1
            if "kInternationalTrainerNameMax" not in limit:
                failures.append(f"{name}: OT prompt limit is {limit.strip()!r}")
            if "1-7" not in guide:
                failures.append(f"{name}: OT prompt guide says {guide!r}")
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
