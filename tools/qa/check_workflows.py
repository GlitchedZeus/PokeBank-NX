#!/usr/bin/env python3
"""CI hygiene (Thread 4 QA, QA-F2): every committed GitHub Actions workflow must parse.

GitHub cannot read the `on:` filters of a workflow file that is not valid YAML, so it records a
failed run for that file on EVERY push to every branch carrying it, which hides real failures.
This check fails when any .github/workflows/*.yml|*.yaml file does not parse, has no `on:` trigger
or no `jobs:` mapping.

Usage: check_workflows.py [repo-root]
"""
from __future__ import annotations

import sys
from pathlib import Path

import yaml  # PyYAML: present on ubuntu runners and in the dev container


def problems(root: Path) -> list[str]:
    out: list[str] = []
    files = sorted((root / ".github" / "workflows").glob("*.y*ml"))
    if not files:
        out.append("no workflow files found")
    for path in files:
        try:
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
        except yaml.YAMLError as exc:
            mark = getattr(exc, "problem_mark", None)
            out.append(f"{path.name}:{mark.line + 1 if mark else '?'}: not valid YAML")
            continue
        if not isinstance(data, dict):
            out.append(f"{path.name}: not a mapping")
            continue
        # PyYAML (YAML 1.1) reads the bare key `on` as boolean True.
        if "on" not in data and True not in data:
            out.append(f"{path.name}: no `on:` trigger")
        if not isinstance(data.get("jobs"), dict) or not data["jobs"]:
            out.append(f"{path.name}: no `jobs:` mapping")
    return out


def main(argv: list[str]) -> int:
    root = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2]
    found = problems(root)
    for line in found:
        print(f"FAIL  {line}")
    count = len(list((root / ".github" / "workflows").glob("*.y*ml")))
    if found:
        print(f"{len(found)} workflow problem(s) in {count} file(s)")
        return 1
    print(f"PASS  all {count} workflow files parse with a trigger and jobs")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
