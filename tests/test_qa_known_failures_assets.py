#!/usr/bin/env python3
"""Reproductions of OPEN tooling / CI-hygiene defects (Thread 4 QA). NOT part of any default gate.

Each check FAILS (non-zero exit) while its defect is open; see docs/qa/FINDINGS.md.

(QA-F1 was fixed in 3b35b3f and moved to tests/test_qa_regressions_assets.py.)
QA-F2  Some committed .github/workflows files are not valid YAML, so GitHub registers them as
       broken workflows (they show up under their file path instead of their name).
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

open_defects = 0


def report(ok: bool, label: str, detail: str = "") -> None:
    global open_defects
    print(("fixed " if ok else "OPEN  ") + label + (f": {detail}" if detail and not ok else ""))
    if not ok:
        open_defects += 1


try:
    import yaml  # type: ignore
except ImportError:  # pragma: no cover - PyYAML is present on ubuntu runners and in the dev container
    yaml = None
if yaml is None:
    print("skip  QA-F2 (PyYAML unavailable)")
else:
    broken = []
    for path in sorted((ROOT / ".github" / "workflows").glob("*.yml")):
        try:
            yaml.safe_load(path.read_text(encoding="utf-8"))
        except yaml.YAMLError as exc:
            mark = getattr(exc, "problem_mark", None)
            broken.append(f"{path.name}:{mark.line + 1 if mark else '?'}")
    report(not broken, "QA-F2 every committed workflow file is valid YAML", ", ".join(broken))

if open_defects:
    print(f"{open_defects} known defect(s) still reproduce")
    sys.exit(1)
print("No known defect reproduces any more: move these checks into the owning suite.")
