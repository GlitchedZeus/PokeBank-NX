#!/usr/bin/env python3
"""Make, or check, the owner's retest checklist for the bound candidate.

  python3 tools/qa/make_retest.py            # write docs/hardware/RETEST_<sha8>.md from the template
  python3 tools/qa/make_retest.py --check    # verify that file names exactly the bound candidate

The candidate identity comes only from docs/qa/CANDIDATE_UNDER_AUDIT.env (the same single source QA-F9
uses); nothing is guessed. The template (docs/hardware/RETEST_ROUND6_TEMPLATE.md) keeps its items, steps
and empty result boxes; only its identity table and binding note are filled in. The result always says
NOT DEVICE TESTED: only the owner's own test of this exact NRO records acceptance.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
ENV = "docs/qa/CANDIDATE_UNDER_AUDIT.env"
TEMPLATE = "docs/hardware/RETEST_ROUND6_TEMPLATE.md"
REPO = "https://github.com/GlitchedZeus/PokeBank-NX"

FIELDS = {
    "RUN_ID": r"[0-9]+",
    "ARTIFACT_ID": r"[0-9]+",
    "ARTIFACT_NAME": r"PokeBank-NX-Gen1-5-Candidate-[0-9a-f]{8}",
    "NRO_FILE": r"PokeBank-NX-Gen1-5-Candidate-[0-9a-f]{8}\.nro",
    "CODE_SHA": r"[0-9a-f]{40}",
    "TREE_SHA": r"[0-9a-f]{40}",
    "NRO_SHA256": r"[0-9a-f]{64}",
    "NRO_SIZE": r"[0-9]+",
}


def read_env(path: Path) -> dict[str, str]:
    """KEY=VALUE lines of the binding; every field present exactly once and well formed."""
    values: dict[str, str] = {}
    for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        key, sep, value = line.partition("=")
        if not sep:
            raise SystemExit(f"{path}:{n}: not KEY=VALUE: {line!r}")
        if key in values:
            raise SystemExit(f"{path}:{n}: {key} given twice")
        values[key] = value.strip()
    for key, rx in FIELDS.items():
        if key not in values:
            raise SystemExit(f"{path}: {key} missing")
        if not re.fullmatch(rx, values[key]):
            raise SystemExit(f"{path}: {key}={values[key]!r} is not well formed")
    short = values["CODE_SHA"][:8]
    if values["ARTIFACT_NAME"] != f"PokeBank-NX-Gen1-5-Candidate-{short}" or \
            values["NRO_FILE"] != f"PokeBank-NX-Gen1-5-Candidate-{short}.nro":
        raise SystemExit(f"{path}: ARTIFACT_NAME / NRO_FILE do not name CODE_SHA {short}")
    return values


def identity_table(env: dict[str, str]) -> str:
    run = env["RUN_ID"]
    return "\n".join([
        "| Identity | Value |",
        "| --- | --- |",
        f"| NRO file | `{env['NRO_FILE']}` |",
        f"| Code SHA | `{env['CODE_SHA']}` |",
        f"| Tree SHA | `{env['TREE_SHA']}` |",
        f"| Build run | {REPO}/actions/runs/{run} (run `{run}`) |",
        f"| Artifact | `{env['ARTIFACT_NAME']}` (id `{env['ARTIFACT_ID']}`) |",
        f"| NRO SHA-256 | `{env['NRO_SHA256']}` |",
        f"| NRO size | {env['NRO_SIZE']} bytes |",
        "| QA-F9 audit run / job | (recorded by the coordinator after the audit passes) |",
    ])


def render(template: str, env: dict[str, str]) -> str:
    short = env["CODE_SHA"][:8]
    out, n = re.subn(r"^# Round 6 hardware retest checklist \(template\)$",
                     f"# Round 6 hardware retest checklist: {env['NRO_FILE']}", template, count=1, flags=re.M)
    if n != 1:
        raise SystemExit("template heading not found")
    out, n = re.subn(r"^When the coordinator binds the round 6 candidate.*?(?=\n\n)",
                     f"This checklist is for the bound round 6 candidate `{short}` (docs/qa/CANDIDATE_UNDER_AUDIT.env).\n"
                     "Test only the file whose SHA-256 matches the table. Every other build is not this candidate.",
                     out, count=1, flags=re.M | re.S)
    if n != 1:
        raise SystemExit("template binding note not found")
    out, n = re.subn(r"^\| Identity \| Value \|\n(?:\|.*\|\n?)+", identity_table(env) + "\n", out, count=1, flags=re.M)
    if n != 1:
        raise SystemExit("template identity table not found")
    if "NOT DEVICE TESTED" not in out:
        raise SystemExit("rendered checklist lost its NOT DEVICE TESTED status")
    return out


def check(path: Path, env: dict[str, str], template: str) -> list[str]:
    """Problems with an existing RETEST file for this binding (empty list = good)."""
    if not path.is_file():
        return [f"{path} is missing"]
    text = path.read_text(encoding="utf-8")
    problems = [f"{path}: does not name {key} {env[key]}" for key in
                ("NRO_FILE", "CODE_SHA", "TREE_SHA", "NRO_SHA256", "NRO_SIZE", "RUN_ID", "ARTIFACT_ID")
                if env[key] not in text]
    if "NOT DEVICE TESTED" not in text:
        problems.append(f"{path}: no NOT DEVICE TESTED status")
    want = re.findall(r"^### ([A-Z]\d\d) · ", template, re.M)
    have = re.findall(r"^### ([A-Z]\d\d) · ", text, re.M)
    if have != want:
        missing = sorted(set(want) - set(have))
        problems.append(f"{path}: checklist items differ from the template (missing {missing})")
    other = sorted(p.name for p in path.parent.glob("RETEST_*.md")
                   if p != path and env["CODE_SHA"][:8] in p.name)
    if other:
        problems.append(f"{path.parent}: more than one RETEST file for {env['CODE_SHA'][:8]}: {other}")
    return problems


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", type=Path, default=ROOT)
    ap.add_argument("--env", default=ENV)
    ap.add_argument("--template", default=TEMPLATE)
    ap.add_argument("--out", help="output path (default docs/hardware/RETEST_<sha8>.md)")
    ap.add_argument("--check", action="store_true", help="verify the existing file instead of writing it")
    args = ap.parse_args(argv[1:])
    root = args.root.resolve()
    env = read_env(root / args.env)
    template = (root / args.template).read_text(encoding="utf-8")
    out = root / (args.out or f"docs/hardware/RETEST_{env['CODE_SHA'][:8]}.md")
    if args.check:
        problems = check(out, env, template)
        for p in problems:
            print(f"FAIL  {p}")
        print(f"{'FAIL' if problems else 'PASS'}: retest checklist {out.relative_to(root)} for {env['NRO_FILE']}")
        return 1 if problems else 0
    if out.exists():
        raise SystemExit(f"{out} exists; it may hold owner results. Not overwritten.")
    out.write_text(render(template, env), encoding="utf-8")
    print(f"wrote {out.relative_to(root)} for {env['NRO_FILE']} (NOT DEVICE TESTED)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
