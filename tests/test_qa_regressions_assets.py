#!/usr/bin/env python3
"""Regression tests for FIXED QA tooling findings (Thread 4). Run by `make qa-tools-test`.

QA-F1 (fixed in 3b35b3f): tools/verify_nro_romfs.py required_paths() must cover every romfs:/ asset
       the application source requests, so the packaged-RomFS gate fails an NRO that lacks one even
       when the local romfs/ tree lacks it too.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "qa"))

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
