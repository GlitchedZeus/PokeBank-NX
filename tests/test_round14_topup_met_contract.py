#!/usr/bin/env python3
"""Round 14 source contract (owner Round 13 H07 / H08 Top Up, R06 / H05 met locations; shared-UX audit
P0-9 and P1-9): every generation's surface uses the ONE shared component.

* Bag Top Up / Add Missing: the Gen I-III classic overlay, the Gen IV Bag and the Gen V Bag all show the
  shared RESULT (UI/BagTopUpModel.h showResult) after A, so what was / was not staged is listed;
  UI/SharedBagTopUp.h routes and draws that step.
* Met Location: the Gen IV / V shared editor surface, the Gen III surface and the Crystal overlay all
  take their DEFAULT caption from Integration/Encounter/MetListScope.h, and Gen IV / V take the
  unverified list's order from it (stored, "level too low" places first, then every other place).
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    "include/UI/SharedBagTopUp.h": ["Model::Step::Result", "Model::resultInput(", "Result: Top Up / Add Missing"],
    "src/UI/ClassicInventoryOverlay.cpp": ["Model::showResult(ui.model, std::move(lines), applied);"],
    "src/UI/Gen4SharedPokemonSurface.inc": ["Model::showResult(ui.model,std::move(lines),again);"],
    "src/UI/Gen5SharedReadOnlySurface.inc": ["Model::showResult(ui.model,std::move(lines),again);"],
    "src/UI/SharedPokemonEditorSurface.inc": [
        '#include "Integration/Encounter/MetListScope.h"',
        "state.pickerNativeValues = PokeVault::Integration::MetListScope::unverifiedChoices(context);",
        "state.pickerValues = PokeVault::Integration::MetListScope::unverifiedChoices(state.pickerMetContext);",
        "PokeVault::Integration::MetListScope::defaultScope(state.pickerMetContext)",
    ],
    "src/UI/Gen3SharedPokemonSurface.inc": ["PokeVault::Integration::MetListScope::defaultScope("],
    "src/UI/Gen2PokemonPickerOverlay.inc": ["MetListScope::defaultScope(model.encounterChoices.size(), model.levelGap"],
}


def main():
    failures = []
    for rel, tokens in REQUIRED.items():
        path = ROOT / rel
        text = path.read_text(encoding="utf-8") if path.exists() else ""
        for token in tokens:
            if token not in text:
                failures.append(f"{rel}: missing {token!r}")
    if failures:
        print("Round 14 Top Up / Met Location shared-component contract: FAIL")
        for f in failures:
            print("  " + f)
        return 1
    print("Round 14 Top Up / Met Location shared-component contract: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
