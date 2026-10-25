#!/usr/bin/env python3
"""Negative controls for the round 5 cross-gen contracts (tests/test_qa_cross_gen_contract.py).

The unmutated tree must pass each enforced contract (CG43, CG64), and each deliberately broken
source path must be reported by its contract. CG61-CG63 stay XFAIL until their lane lands (the
pre-fix code is their own negative control); here they only need to exist.
"""
from __future__ import annotations

import sys
from dataclasses import replace
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_qa_cross_gen_contract as qa  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SOURCES, _ = qa.load_sources(ROOT)


def result(sources, cid):
    return next(r for r in qa.contracts(sources) if r.cid == cid)


def mutated(rel, old, new):
    out, found = [], False
    for s in SOURCES:
        if s.rel == rel:
            assert old in s.code, (rel, old)
            out.append(replace(s, code=s.code.replace(old, new, 1)))
            found = True
        else:
            out.append(s)
    assert found, rel
    return out


# (description, contract, file, old, new)
MUTATIONS = [
    ("Nature colours swapped", "CG43-nature-stat-colours", "include/UI/NatureStatPresentation.h",
     "modifier > 0 ? Colors::Success : modifier < 0 ? Colors::Error", "modifier > 0 ? Colors::Error : modifier < 0 ? Colors::Success"),
    ("Gen III stat rows ignore statColor", "CG43-nature-stat-colours", "src/UI/Gen3SharedPokemonSurface.inc",
     "NatureStats::statColor(p.nature, row, selected ? Colors::SelectedText : Colors::TextDim)",
     "(selected ? Colors::SelectedText : Colors::TextDim)"),
    ("theme success colour no longer green", "CG43-nature-stat-colours", "include/UI/Common.h",
     "Color(82, 205, 137)", "Color(205, 82, 137)"),
    ("Gen III gender offered for any stored byte", "CG64-trainer-playtime-gender-staged",
     "include/Integration/Gen3/Gen3StagedTrainerEditor.h",
     "return originalGender_ <= 1;", "return true;"),
    ("Gen II play time skips Review", "CG64-trainer-playtime-gender-staged", "src/Integration/Gen2/Gen2StagedEditor.cpp",
     'setChange("play_time"', 'noteChange("play_time"'),
    ("Gen II gender no longer Crystal-only", "CG64-trainer-playtime-gender-staged", "src/Integration/Gen2/Gen2StagedEditor.cpp",
     "return metadata_.family == VersionFamily::Crystal && original_.size() > kCrystalGenderOffset &&",
     "return original_.size() > kCrystalGenderOffset &&"),
    ("Gen IV gender stager set without the native-byte check", "CG64-trainer-playtime-gender-staged",
     "src/UI/Gen4SharedPokemonSurface.inc",
     "if (trainer.genderEditable())\n        fields.stageGender", "if (true)\n        fields.stageGender"),
    ("Gen I offers a gender edit", "CG64-trainer-playtime-gender-staged", "src/UI/Gen1PokemonEditorOverlayFoundation.inc",
     "fields.nameEditable = editor->nameEditable();",
     "fields.nameEditable = editor->nameEditable();\n    fields.stageGender = nullptr;"),
    ("Gen IV play time written straight to a file", "CG64-trainer-playtime-gender-staged",
     "src/Integration/Gen4/Gen4StagedPokemonEditor.cpp",
     "return stageTrainerBytes(kTrainerPlayTimeField, field,",
     "std::fopen(\"x\", \"wb\");\n    return stageTrainerBytes(kTrainerPlayTimeField, field,"),
]


def main() -> int:
    failures = 0
    base = {r.cid: r for r in qa.contracts(SOURCES)}
    for cid in ("CG43-nature-stat-colours", "CG64-trainer-playtime-gender-staged"):
        if base[cid].violations:
            print(f"FAIL  unmutated tree: {cid}: {base[cid].violations[:2]}")
            failures += 1
    for cid in ("CG61-switch-backup-opens-staged-copy", "CG62-met-location-rows-fit", "CG63-help-names-real-controls",
                "CG68-gen1-review-swipe-one-page"):
        if cid not in base:
            print(f"FAIL  {cid} is missing")
            failures += 1
    for desc, cid, rel, old, new in MUTATIONS:
        r = result(mutated(rel, old, new), cid)
        if r.violations:
            print(f"PASS  {desc}: {cid} fails ({r.violations[0][:100]})")
        else:
            print(f"FAIL  {desc}: {cid} did not report it")
            failures += 1
    print(f"{'FAIL' if failures else 'PASS'}: round 5 contract mutations ({failures} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
