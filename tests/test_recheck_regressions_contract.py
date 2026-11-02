#!/usr/bin/env python3
"""Contract for docs/qa/RECHECK_REGRESSIONS.md (owner: "The 94 earlier accepted items touched by new code
must remain marked for re-verification, with targeted regressions based on the actual code changes.").

- Every Hardware Accepted issue with rechecked_needed in the trace data has exactly one row; none was
  flipped to accepted (the trace rows keep status Hardware Accepted + rechecked_needed, counts.json agrees).
- Every row records the git range from its accepted candidate and at least one changed implementing file.
- Every row that is not superseded names at least one regression test; every named test exists, and
  every C++ test is a registered host target (Makefile.host*).
- The Markdown is the rendering of its JSON data (tools/qa/gen_recheck_regressions.py --check).
"""
from pathlib import Path
import glob
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REC = ROOT / "docs" / "qa" / "reconciliation"
failures = []


def require(ok, what):
    if not ok:
        failures.append(what)


trace = []
for f in sorted(glob.glob(str(REC / "trace_G[1-7]*.json"))):
    trace += json.loads(Path(f).read_text(encoding="utf-8"))["issues"]
recheck = {i["id"]: i for i in trace if i["status"] == "Hardware Accepted" and i.get("rechecked_needed")}
counts = json.loads((REC / "counts.json").read_text(encoding="utf-8"))
rows = json.loads((REC / "recheck_regressions.json").read_text(encoding="utf-8"))
md = (ROOT / "docs" / "qa" / "RECHECK_REGRESSIONS.md").read_text(encoding="utf-8")
makefiles = "".join(Path(p).read_text(encoding="utf-8") for p in glob.glob(str(ROOT / "Makefile.host*")))

require(len(recheck) == counts["hardware_accepted_needing_r12_recheck"] == 94,
        f"94 accepted issues need a recheck (trace {len(recheck)}, counts.json "
        f"{counts['hardware_accepted_needing_r12_recheck']})")
ids = [r["id"] for r in rows]
require(len(ids) == len(set(ids)), "one row per issue")
require(set(ids) == set(recheck), f"rows match the trace: missing {sorted(set(recheck) - set(ids))}, "
                                  f"extra {sorted(set(ids) - set(recheck))}")

allowed = {"recheck", "partial", "superseded", "scope"}
for r in rows:
    iid = r["id"]
    require(r["trace_status"] == "Hardware Accepted" and r["rechecked_needed"], f"{iid}: stays marked for recheck")
    require(r["status"] in allowed, f"{iid}: status {r['status']!r}")
    require("recheck required" in r["status_label"] or "re-verification" in r["status_label"],
            f"{iid}: the status says it must be re-verified ({r['status_label']!r})")
    g = r["git"]
    require(g["range"].startswith(r["accepted_candidate"] + ".."), f"{iid}: range starts at the accepted candidate")
    require(g["commit_count"] >= 1 and g["changed_files"], f"{iid}: implementing files changed since acceptance")
    if r["status"] != "superseded":
        require(r["tests"] or r["new_tests"], f"{iid}: names a regression test")
    for t in r["tests"] + r["new_tests"]:
        p = ROOT / t["path"]
        require(p.is_file(), f"{iid}: {t['path']} exists")
        if t["path"].endswith(".cpp"):
            require(p.stem in makefiles, f"{iid}: {t['path']} is a registered host test")
    require(md.count(f"| `{iid}` |") == 1, f"{iid}: exactly one row in RECHECK_REGRESSIONS.md")
    require(iid in recheck and recheck[iid]["status"] == "Hardware Accepted", f"{iid}: trace status unchanged")

for name in ("test_recheck_met_language", "test_ui_recheck_regressions_host"):
    require(f"$(HOST_BUILD)/{name}" in makefiles or f"{name}" in makefiles, f"{name} registered")
    require(f"{name}_sanitize" in makefiles, f"{name} has a sanitize target")

check = subprocess.run([sys.executable, str(ROOT / "tools" / "qa" / "gen_recheck_regressions.py"), str(ROOT), "--check"],
                       capture_output=True, text=True)
require(check.returncode == 0, "RECHECK_REGRESSIONS.md is up to date: " + (check.stdout + check.stderr).strip())

if failures:
    print("FAIL test_recheck_regressions_contract")
    for f in failures:
        print("  -", f)
    sys.exit(1)
print(f"PASS test_recheck_regressions_contract ({len(rows)} rows, all still marked for re-verification)")
