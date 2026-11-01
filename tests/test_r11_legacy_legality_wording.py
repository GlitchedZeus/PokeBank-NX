#!/usr/bin/env python3
"""Round 11 (CG48, intake section 3 lead "misleading legacy green legality wording").

The generic Pokemon details modal (src/UI/Modals/PokemonDetailsModal.cpp) summarises
Legality::analyze(). In a Gen I-V session (a Bank Pokemon opened from the Storage view reaches it)
the legacy checker's coverage is incomplete (#329), so a report with no issues must read as the
shared "Incomplete" state in the warning colour, never a green "no problems found" verdict. Gen VI+
sessions keep their wording. UI wording only: Legality::analyze() itself is untouched.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODAL = (ROOT / "src/UI/Modals/PokemonDetailsModal.cpp").read_text(encoding="utf-8")


def fail(message: str) -> None:
    print("FAIL:", message)
    sys.exit(1)


def check(source: str) -> list[str]:
    problems = []
    if "ClassicGenericUi::classicSession(screen.sourceGameId)" not in source:
        problems.append("the modal never asks whether this is a Gen I-V session")
    if "legalityLabel(PokeBank::UIModel::SharedEditorPresentation::LegalityState::Incomplete)" not in \
            re.sub(r"\s+", "", source):
        problems.append("a Gen I-V clean report is not the shared Incomplete state")
    # Every green clean-verdict literal must sit in the Gen VI+ (full coverage) arm of a ternary
    # or an if on partialLegalityCoverage, never unconditionally.
    for m in re.finditer(r'"(?:Legality: )?[Nn]o problems found\.?"', source):
        window = source[max(0, m.start() - 400):m.start()]
        if "partialLegalityCoverage" not in window:
            line = source.count("\n", 0, m.start()) + 1
            problems.append(f"line {line}: clean verdict {m.group(0)} is not gated on partialLegalityCoverage")
    return problems


def self_test() -> None:
    bad = 'fb.drawText(x, y, "Legality: no problems found", Color(120, 205, 140), TextStyle::Caption);'
    assert check(bad), "the contract must reject an ungated green clean verdict"


self_test()
problems = check(MODAL)
if problems:
    fail("; ".join(problems))
print("Round 11 CG48: Gen I-V legacy legality summary never shows a clean green verdict PASS")
