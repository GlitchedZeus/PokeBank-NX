#!/usr/bin/env python3
"""Mutation regression for tests/test_hw_round1_regression_gate.py.

Each mutation breaks one kind of guard in a symlinked copy of the tree and the gate must FAIL; the
unmutated copy must PASS. Run by `make -f Makefile.host qa-tools-test`.
"""
from __future__ import annotations

import importlib.util
import os
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("hw_gate", ROOT / "tests/test_hw_round1_regression_gate.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


def shadow_tree(dst: Path) -> None:
    for entry in ROOT.iterdir():
        if entry.name in (".git", "build-host") or entry.name.startswith("build"):
            continue
        if entry.is_dir() and entry.name in ("src", "include", "tests", "docs"):
            # Real copies (~15 MB): the contract loader resolves paths, so symlinks would escape the copy.
            shutil.copytree(entry, dst / entry.name, symlinks=True,
                            ignore=shutil.ignore_patterns("__pycache__"))
        else:
            os.symlink(entry, dst / entry.name)


def mutate(root: Path, rel: str, old: str, new: str) -> None:
    p = root / rel
    body = Path(os.path.realpath(p)).read_text(encoding="utf-8")
    assert old in body, f"mutation anchor missing in {rel}: {old!r}"
    if p.is_symlink():
        p.unlink()
    p.write_text(body.replace(old, new), encoding="utf-8")


MUTATIONS = [
    # (description, file, old, new, expected error substring)
    ("guard literal deleted", "tests/test_home_input_guard.cpp",
     "Q::decide(Q::kButtonA) == Q::Decision::Stay", "true", "C01: tests/test_home_input_guard.cpp no longer contains"),
    ("cross-gen contract broken (Make Shiny back)", "include/UI/NarrowPokemonActionMenu.h",
     "} // namespace detail", '} // namespace detail\ninline const char* kRegressed = "Make Shiny";',
     "E01: CG01-no-make-shiny-quick-action fails"),
    ("source pin changed (pad setup)", "src/UI/UI.cpp",
     "padConfigureInput(1, HidNpadStyleSet_NpadStandard);", "padConfigureInput(8, HidNpadStyleSet_NpadStandard);",
     "C09: source pin changed"),
    ("python contract fails", "tests/test_game_hub_contract.py",
     "def require(cond: bool, message: str) -> None:\n    if not cond:",
     "def require(cond: bool, message: str) -> None:\n    raise SystemExit('forced')\n    if not cond:",
     "tests/test_game_hub_contract.py fails"),
    ("round 2 accepted guard literal deleted (S01)", "tests/test_shared_editor_unit.cpp",
     "S01: PP and PP Ups captions are at least 20 px apart", "PP captions",
     "S01: tests/test_shared_editor_unit.cpp no longer contains"),
    ("round 3 E02 Gen I box sheet loses Move", "tests/test_gen12_packed_move.cpp",
     'std::vector<std::string>{"View", "Edit", "Clone", "Move", "Release", legality, "Cancel"}',
     'std::vector<std::string>{"View", "Edit", "Clone", "Release", legality, "Cancel"}',
     "E02: tests/test_gen12_packed_move.cpp no longer contains"),
    ("round 3 guard literal deleted (R2-BUG-008)", "tests/test_gen3_encounter_keep_stored.cpp",
     "assert(rows.size() == 2 && rows[0] == Picker::kKeepStoredRow && rows[1] == 0);", "assert(true);",
     "R2-BUG-008: tests/test_gen3_encounter_keep_stored.cpp no longer contains"),
    ("round 3 contract broken (Help no longer closes on B)", "src/UI/TrainerViewScreenBase.inc",
     "if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) helpOverlayActive = false;",
     "if (kDown & HidNpadButton_Minus) helpOverlayActive = false;",
     "NEW-01: CG29-help-closes-with-b fails"),
    ("round 3 sheet changed (an extra PASS without a guard)", "docs/hardware/ROUND3_RESULTS_62a2f8c3.md",
     "- **AUD-01 — FAIL — Gen V matches shared editor.**", "- **AUD-01 — PASS — Gen V matches shared editor.**",
     "owner PASS item(s) with no guard: ['AUD-01']"),
    ("round 2 record changed (S03 no longer PASS)", "docs/hardware/ROUND2_RESULTS_e10f307f.md",
     "| S03 | Gen V Create, Clone, Move and Release work (left-panel clipping, parity) | PASS |",
     "| S03 | Gen V Create, Clone, Move and Release work (left-panel clipping, parity) | FAIL |",
     "docs/hardware/ROUND2_RESULTS_e10f307f.md: PASS"),
    ("Oct 24 NEW-09 nature colours swapped (raised red)", "include/UI/NatureStatPresentation.h",
     "return modifier > 0 ? Colors::Success : modifier < 0 ? Colors::Error : normal;",
     "return modifier > 0 ? Colors::Error : modifier < 0 ? Colors::Success : normal;",
     "NEW-09: CG43-nature-stat-colours fails"),
    ("Oct 24 NEW-09 stat rows stop colouring by Nature", "src/UI/SharedPokemonEditorSurface.inc",
     "NatureStats::statColor(A::nature(*raw), row, selected ? Colors::SelectedText : Colors::TextDim)",
     "(selected ? Colors::SelectedText : Colors::TextDim)",
     "NEW-09: CG43-nature-stat-colours fails"),
    ("Oct 24 guard literal deleted (NEW-12 Walrein ancestry)", "tests/test_gen45_encounter_context.cpp",
     "assert(walrein[365] && walrein[364] && walrein[363] && !walrein[362]);", "assert(walrein[365]);",
     "NEW-12: tests/test_gen45_encounter_context.cpp no longer contains"),
    ("Oct 24 owner list changed (M05 recorded as PASS)", "docs/hardware/OCT24_REPAIRS_AND_RETEST.md",
     "- PASS: E03, L06,", "- PASS: M05, E03, L06,",
     "docs/hardware/OCT24_REPAIRS_AND_RETEST.md: owner Oct 24 PASS"),
    ("Oct 25 R2-SPECIAL: SpD copied from SpA (calculation, not layout)", "tests/test_gen2_special_stat.cpp",
     "assert(battle[4] == spa && battle[5] == spd && spa != spd);", "assert(battle[4] == spa);",
     "R2-SPECIAL: tests/test_gen2_special_stat.cpp no longer contains"),
    ("Oct 25 NEW-14: Gen V Language opens a picker again", "tests/test_gen5_language_cycle.cpp",
     'assert(branch.find("openPicker(")==std::string::npos);', "assert(true);",
     "NEW-14: tests/test_gen5_language_cycle.cpp no longer contains"),
    ("Oct 25 P3-TRAINER: Gen III gender offered for any stored byte", "include/Integration/Gen3/Gen3StagedTrainerEditor.h",
     "return originalGender_ <= 1;", "return true;",
     "P3-TRAINER: CG64-trainer-playtime-gender-staged fails"),
    ("Oct 25 owner sheet changed (L06 recorded as PASS)", "docs/hardware/ROUND5_RESULTS_1ca2faec.md",
     "E03, C07, M04, E02, R2-BUG-002,", "L06, E03, C07, M04, E02, R2-BUG-002,",
     "docs/hardware/ROUND5_RESULTS_1ca2faec.md: 32 Oct 25 PASS items"),
    ("Oct 26 C16: a draft leaks into the next save (guard literal deleted)", "tests/test_ui_editor_input_host.cpp",
     'check(trainerB->stagedPokemon()->pendingChanges().empty(), "save B: nothing from save A is staged");', "",
     "C16: tests/test_ui_editor_input_host.cpp no longer contains"),
    ("Oct 26 C04: popups close on contact (tap-outside rule weakened)", "tests/test_popup_dismiss.cpp",
     "assert(s.pickerOpen && s.dismissals == 0);      // contact alone never closes it", "assert(true);",
     "C04: tests/test_popup_dismiss.cpp no longer contains"),
    ("Oct 26 B02: Gold / Silver gender comes back", "src/UI/TrainerViewScreenBase.inc",
     "infoRow(\"Trainer ID\"", "infoRow(\"Gender\",\"Male (fixed by game)\");infoRow(\"Trainer ID\"",
     "B02: CG72-gold-silver-trainer-no-gender fails"),
    ("Oct 26 owner sheet changed (B04 recorded as PASS)", "docs/hardware/ROUND6_RESULTS_26186464.md",
     "| **B04** | **E05** | **FAIL** |", "| **B04** | **E05** | **PASS** |",
     "docs/hardware/ROUND6_RESULTS_26186464.md: Oct 26 B / C / D PASS rows"),
]


def main() -> int:
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        base = Path(tmp) / "base"
        base.mkdir()
        shadow_tree(base)
        errs = gate.check(base)
        if errs:
            print(f"FAIL  unmutated tree must pass the gate: {errs[:3]}")
            failures += 1
        else:
            print("PASS  unmutated tree passes the gate")
        for i, (desc, rel, old, new, want) in enumerate(MUTATIONS):
            root = Path(tmp) / f"m{i}"
            root.mkdir()
            shadow_tree(root)
            mutate(root, rel, old, new)
            errs = gate.check(root)
            if any(want in e for e in errs):
                print(f"PASS  {desc}: gate fails ({want})")
            else:
                print(f"FAIL  {desc}: gate did not report {want!r}; got {errs[:3]}")
                failures += 1
        # A C++ guard dropped from host-test: the mapping names a test the build no longer runs.
        saved = gate.GUARDS["N02"]
        gate.GUARDS["N02"] = (saved[0], [("host", "test_does_not_exist_in_host_test",
                                          "tests/test_navigation_repeat.cpp", "static_assert(delay >= 200")])
        errs = gate.check(base, run_python=False)
        gate.GUARDS["N02"] = saved
        if any("is not built/run by `make -f Makefile.host host-test`" in e for e in errs):
            print("PASS  guard not run by host-test: gate fails")
        else:
            print(f"FAIL  guard not run by host-test was not reported; got {errs[:3]}")
            failures += 1
    print(f"{'FAIL' if failures else 'PASS'}: round 1 gate mutation regression ({failures} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
