#!/usr/bin/env python3
"""Reproductions of OPEN tooling / CI-hygiene defects (Thread 4 QA). NOT part of any default gate.

Each check FAILS (non-zero exit) while its defect is open; see docs/qa/FINDINGS.md.

QA-F1  tools/verify_nro_romfs.py required_paths() does not cover every romfs:/ asset the application
       source requests (today: the four Gen V game cards). The packaging gate would pass an NRO that
       lacks them as long as the local romfs/ tree lacks them too.
QA-F2  Some committed .github/workflows files are not valid YAML, so GitHub registers them as
       broken workflows (they show up under their file path instead of their name).
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "qa"))

import nro_audit  # noqa: E402
import verify_nro_romfs  # noqa: E402

open_defects = 0


def report(ok: bool, label: str, detail: str = "") -> None:
    global open_defects
    print(("fixed " if ok else "OPEN  ") + label + (f": {detail}" if detail and not ok else ""))
    if not ok:
        open_defects += 1


referenced = nro_audit.referenced_romfs_paths([ROOT / "src", ROOT / "include"])
uncovered = sorted(referenced - set(verify_nro_romfs.required_paths()))
report(not uncovered, "QA-F1 packaged-RomFS manifest covers every source-referenced asset", ", ".join(uncovered))

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
