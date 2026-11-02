#!/usr/bin/env python3
"""Reproductions of OPEN tooling / CI-hygiene defects (Thread 4 QA). NOT part of any default gate.

Each check FAILS (non-zero exit) while its defect is open; see docs/qa/FINDINGS.md.

(QA-F1, fixed in 3b35b3f, and QA-F2, fixed on #307 in 96a44685, now live in
tests/test_qa_regressions_assets.py.)
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


# No tooling / CI-hygiene defect is open.

if open_defects:
    print(f"{open_defects} known defect(s) still reproduce")
    sys.exit(1)
print("No known defect reproduces any more: move these checks into the owning suite.")
