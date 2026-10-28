#!/usr/bin/env python3
"""Round 8 automated compatibility matrix guard (owner Oct 28).

Every test named in docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md must stay registered in BOTH
HOST_TESTS and HOST_SANITIZE_TESTS, so advanced compatibility evidence can never silently drop out of
the GCC / ASan+UBSan / Clang gates. The matrix must keep its SYNTHETIC / no-real-save labelling. The
Gen II OT prompt check (a source contract no Makefile runs) is executed here.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOC = ROOT / "docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md"

failures = []
text = DOC.read_text(encoding="utf-8")
block = re.search(r"<!-- round8-matrix-tests:begin -->\n(.*?)<!-- round8-matrix-tests:end -->", text, re.S)
if not block:
    sys.exit("FAIL: matrix test list markers missing")
tests = [line.strip() for line in block.group(1).splitlines() if line.strip()]
if len(tests) < 30 or len(set(tests)) != len(tests):
    failures.append(f"matrix test list too short or has duplicates ({len(tests)})")

for phrase in ("Evidence class for every row: SYNTHETIC", "It does not prove",
               "NOT TESTED / NO FIXTURE, never FAIL", "**None.**"):
    if phrase not in text:
        failures.append(f"matrix lost its labelling: {phrase!r}")

out = subprocess.run(["make", "-pq", "-f", "Makefile.host"], cwd=ROOT, capture_output=True, text=True).stdout
lists = {}
for name in ("HOST_TESTS", "HOST_SANITIZE_TESTS"):
    m = re.search(rf"^{name} :?= (.*)$", out, re.M)
    lists[name] = set(m.group(1).split()) if m else set()
    if not m:
        failures.append(f"{name} not found in make database")
for t in tests:
    if f"build-host/{t}" not in lists["HOST_TESTS"]:
        failures.append(f"{t} is not in HOST_TESTS")
    if f"build-host/{t}_sanitize" not in lists["HOST_SANITIZE_TESTS"]:
        failures.append(f"{t} is not in HOST_SANITIZE_TESTS")

ot = subprocess.run([sys.executable, str(ROOT / "tests/test_gen2_ot_prompt_limit.py")], cwd=ROOT)
if ot.returncode != 0:
    failures.append("tests/test_gen2_ot_prompt_limit.py failed")

for f in failures:
    print(f"FAIL  {f}")
print(f"{'FAIL' if failures else 'PASS'}: Round 8 compatibility matrix guard ({len(tests)} tests, "
      f"{len(failures)} failure(s))")
sys.exit(1 if failures else 0)
