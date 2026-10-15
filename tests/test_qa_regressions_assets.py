#!/usr/bin/env python3
"""Regression tests for FIXED QA tooling findings (Thread 4). Run by `make qa-tools-test`.

QA-F1 (fixed in 3b35b3f): tools/verify_nro_romfs.py required_paths() must cover every romfs:/ asset
       the application source requests, so the packaged-RomFS gate fails an NRO that lacks one even
       when the local romfs/ tree lacks it too.
QA-F2 (fixed on #307 in 96a44685): every committed .github/workflows file must parse, with an `on:`
       trigger and jobs (tools/qa/check_workflows.py).
QA-F9 (fixed on audit/app-qa-20261014): the candidate NRO audit has one identity source, no defaults,
       GitHub API binding and a staleness guard (tests/test_qa_nro_audit_workflow.py).
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "qa"))

import check_workflows  # noqa: E402
sys.path.insert(0, str(ROOT / "tests"))
import test_qa_nro_audit_workflow  # noqa: E402
import nro_audit  # noqa: E402
import verify_nro_romfs  # noqa: E402

referenced = nro_audit.referenced_romfs_paths([ROOT / "src", ROOT / "include"])
uncovered = sorted(referenced - set(verify_nro_romfs.required_paths()))
if not referenced:
    print("FAIL  no romfs:/ asset references found in the source (scanner broken?)")
    sys.exit(1)
if uncovered:
    print(f"FAIL  QA-F1 regression: {len(uncovered)} source-referenced asset(s) missing from the "
          f"packaged-RomFS manifest: {', '.join(uncovered)}")
    sys.exit(1)
print(f"PASS  QA-F1: all {len(referenced)} source-referenced RomFS assets are in the packaged manifest")

workflow_problems = check_workflows.problems(ROOT)
if workflow_problems:
    print("FAIL  QA-F2 regression: " + "; ".join(workflow_problems))
    sys.exit(1)
print("PASS  QA-F2: every committed workflow file parses with a trigger and jobs")

if test_qa_nro_audit_workflow.main([sys.argv[0]]) != 0:
    sys.exit(1)
