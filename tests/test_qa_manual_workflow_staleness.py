#!/usr/bin/env python3
"""Regression for tools/qa/manual_workflow_staleness.py on a synthetic tree."""
from __future__ import annotations

import importlib.util
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("staleness", ROOT / "tools/qa/manual_workflow_staleness.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


def workflow(on: str, body: str) -> str:
    lines = "\n".join("          " + line for line in body.strip().splitlines())
    return (f"name: synthetic\non:\n  {on}:\njobs:\n  j:\n    runs-on: ubuntu-latest\n    steps:\n"
            f"      - name: check\n        run: |\n          python3 - <<'PY'\n{lines}\n          PY\n")


def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "src").mkdir()
        (root / "src/a.cpp").write_text('const char* k = "Origin & Legality";\n')
        wf = root / ".github/workflows"
        wf.mkdir(parents=True)
        read = "from pathlib import Path\nsrc = Path('src/a.cpp').read_text()\n"
        (wf / "current.yml").write_text(workflow("workflow_dispatch", read + "assert 'Origin & Legality' in src"))
        (wf / "stale.yml").write_text(workflow("workflow_dispatch", read + "assert 'Encounter legality' in src, 'Encounter legality'"))
        (wf / "unsafe.yml").write_text(workflow("workflow_dispatch", "import subprocess\nsubprocess.run(['true'])"))
        (wf / "artifact.yml").write_text(workflow("workflow_dispatch", "import json\njson.load(open('embedded-romfs.json'))"))
        (wf / "on_pr.yml").write_text(workflow("pull_request", read + "assert 'Encounter legality' in src"))
        rows = {r[0]: r for r in tool.check(root)}
        expect = {"current.yml": "current", "stale.yml": "STALE", "unsafe.yml": "not checked",
                  "artifact.yml": "needs build output"}
        failures = 0
        for name, status in expect.items():
            got = rows.get(name, (None, None, "missing", ""))[2]
            ok = got == status
            failures += not ok
            print(f"{'PASS' if ok else 'FAIL'}  {name}: {got} (expected {status})")
        if "on_pr.yml" in rows:
            print("FAIL  a pull_request workflow must not be checked (its CI already runs it)")
            failures += 1
        else:
            print("PASS  on_pr.yml: pull_request workflows are left to their own CI")
        if "Encounter legality" not in rows.get("stale.yml", ("", "", "", ""))[3]:
            print("FAIL  the stale row must name the failing assertion")
            failures += 1
        print(f"{'FAIL' if failures else 'PASS'}: manual workflow staleness regression ({failures} failure(s))")
        return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
