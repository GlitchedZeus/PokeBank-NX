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
    ("round 2 record changed (S03 no longer PASS)", "docs/hardware/ROUND2_RESULTS_e10f307f.md",
     "| S03 | Gen V Create, Clone, Move and Release work (left-panel clipping, parity) | PASS |",
     "| S03 | Gen V Create, Clone, Move and Release work (left-panel clipping, parity) | FAIL |",
     "docs/hardware/ROUND2_RESULTS_e10f307f.md: PASS"),
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
