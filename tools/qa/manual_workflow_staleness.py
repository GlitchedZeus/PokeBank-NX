#!/usr/bin/env python3
"""Flag manual-only workflows whose source checks no longer match the tree.

A workflow that only runs on `workflow_dispatch` never runs on a pull request, so its embedded source
contract (a `python3 - <<'PY' ... PY` heredoc that asserts needles in source files) can drift until it
silently contradicts an enforced contract: gen1-cleanup3-focused.yml still required "Encounter legality",
which CG03 forbids. This tool runs every such read-only heredoc against the tree and lists the ones that
fail, with the failing assertion.

A step that reads an earlier step's output (not a tree file) is listed as "needs build output".
A heredoc is run only when it is a pure source check: it may read files, but any process, network or
write call (subprocess, os.system, urllib, open(..., 'w'), write_text, ...) makes it "not checked".

Usage: python3 tools/qa/manual_workflow_staleness.py [--root DIR] [--strict]
  default  print the report (GitHub step-summary friendly) and exit 0
  --strict exit 1 when any manual workflow is stale
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

import yaml

UNSAFE = re.compile(r"\bsubprocess\b|\bos\.system\b|\bos\.popen\b|\burllib\b|\brequests\b|\bsocket\b|"
                    r"write_text\s*\(|write_bytes\s*\(|open\([^)]*['\"][wa]b?['\"]|\bshutil\.(?:rm|move|copy)|"
                    r"\bos\.(?:remove|unlink|rename|replace|mkdir|makedirs)\b")
HEREDOC = re.compile(r"python3\s+-\s+<<\s*'?(\w+)'?\s*\n(.*?)\n\s*\1\s*(?:\n|$)", re.S)


def triggers(doc: dict) -> set[str]:
    on = doc.get("on", doc.get(True))  # PyYAML reads a bare `on:` key as True
    if isinstance(on, str):
        return {on}
    if isinstance(on, list):
        return set(on)
    if isinstance(on, dict):
        return set(on)
    return set()


def heredocs(doc: dict):
    for job_name, job in (doc.get("jobs") or {}).items():
        for i, step in enumerate((job or {}).get("steps") or []):
            run = (step or {}).get("run") or ""
            for m in HEREDOC.finditer(run):
                yield job_name, step.get("name") or f"step {i + 1}", textwrap.dedent(m.group(2))


def check(root: Path) -> list[tuple[str, str, str, str]]:
    """Return (workflow, step, status, detail) for every heredoc in a manual-only workflow."""
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        for wf in sorted((root / ".github/workflows").glob("*.yml")):
            doc = yaml.safe_load(wf.read_text(encoding="utf-8")) or {}
            if triggers(doc) != {"workflow_dispatch"}:
                continue
            for job, step, code in heredocs(doc):
                if UNSAFE.search(code):
                    rows.append((wf.name, f"{job} / {step}", "not checked", "heredoc is not a read-only source check"))
                    continue
                script = Path(tmp) / "check.py"
                script.write_text(code, encoding="utf-8")
                try:
                    out = subprocess.run([sys.executable, str(script)], cwd=root, capture_output=True,
                                         text=True, timeout=60)
                except subprocess.TimeoutExpired:
                    rows.append((wf.name, f"{job} / {step}", "not checked", "timed out"))
                    continue
                if out.returncode == 0:
                    rows.append((wf.name, f"{job} / {step}", "current", ""))
                else:
                    tail = (out.stderr.strip().splitlines() or ["exit " + str(out.returncode)])[-1]
                    # A step that reads an earlier step's output (embedded-romfs.json, a snapshot
                    # manifest) cannot run against the bare tree: that is not source drift.
                    missing = re.search(r"FileNotFoundError: .*?'([^']+)'", tail)
                    if missing and not (root / missing.group(1)).exists() and \
                            not re.search(r"\b(?:src|include|tests|tools|romfs)/", missing.group(1)):
                        rows.append((wf.name, f"{job} / {step}", "needs build output", missing.group(1)))
                    else:
                        rows.append((wf.name, f"{job} / {step}", "STALE", tail[:200]))
    return rows


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args(argv[1:])
    rows = check(args.root.resolve())
    stale = [r for r in rows if r[2] == "STALE"]
    print(f"Manual-only workflow source checks: {len(rows)} checked, {len(stale)} stale")
    print("| Workflow | Step | Status | Failing check |")
    print("| --- | --- | --- | --- |")
    for wf, step, status, detail in rows:
        print(f"| {wf} | {step} | {status} | {detail.replace('|', '/')} |")
    return 1 if (args.strict and stale) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
