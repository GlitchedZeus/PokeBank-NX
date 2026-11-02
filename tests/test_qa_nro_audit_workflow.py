#!/usr/bin/env python3
"""QA-F9 regression: the candidate-NRO audit workflow can never audit a stale or defaulted artifact.

Run 38732921392 ("Exact-head audit of the published candidate NRO" on #307 a0633b6d) was green while
auditing the OLD fa9cfcea NRO: the workflow fell back to hard-coded identity defaults on every
pull_request run. This check pins the fix in .github/workflows/app-qa-nro-audit.yml:

  * no 40-hex / 64-hex identity literals and no `${{ a || b }}` defaults (outside `concurrency`);
  * the pull_request trigger has no paths filter (the audit runs on every head);
  * workflow_dispatch requires every identity input;
  * the identity file docs/qa/CANDIDATE_UNDER_AUDIT.env exists with exactly the expected keys and
    valid formats;
  * the workflow binds the run (head_sha, conclusion, workflow path) and artifact id to the GitHub
    API, guards staleness, checks the tree SHA, and cross-checks BUILD_IDENTITY.txt / SHA256SUMS.txt.

Usage: test_qa_nro_audit_workflow.py [workflow.yml [candidate.env]]   (exit 1 on any violation)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

import yaml  # PyYAML: present on ubuntu runners and in the dev container

ROOT = Path(__file__).resolve().parent.parent
KEYS = {
    "RUN_ID": r"[0-9]+",
    "ARTIFACT_ID": r"[0-9]+",
    "ARTIFACT_NAME": r"[A-Za-z0-9._-]+",
    "NRO_FILE": r"[A-Za-z0-9._-]+\.nro",
    "CODE_SHA": r"[0-9a-f]{40}",
    "TREE_SHA": r"[0-9a-f]{40}",
    "NRO_SHA256": r"[0-9a-f]{64}",
    "NRO_SIZE": r"[0-9]+",
}
DISPATCH_INPUTS = {"run_id", "artifact_id", "artifact_name", "nro_file", "code_sha", "tree_sha",
                   "nro_sha256", "nro_size"}
REQUIRED_TEXT = {
    "run binding (GitHub API run record)": "actions/runs/$RUN_ID",
    "run head_sha == CODE_SHA": '"$head_sha" = "$CODE_SHA"',
    "run conclusion == success": '"$conclusion" = "success"',
    "run workflow path": ".github/workflows/gen15-integrated-candidate.yml",
    "artifact id binding": "$ARTIFACT_ID",
    "artifact not expired": "expired",
    "staleness: ancestor": "merge-base --is-ancestor",
    "staleness: non-QA diff": "git diff --name-only \"$CODE_SHA\" HEAD",
    "tree SHA check": "^{tree}",
    "BUILD_IDENTITY cross-check": "BUILD_IDENTITY.txt",
    "BUILD_IDENTITY APPLICATION_SHA": "expect APPLICATION_SHA",
    "BUILD_IDENTITY DEVICE_ACCEPTED=false": "expect DEVICE_ACCEPTED false",
    "SHA256SUMS cross-check": "SHA256SUMS.txt",
    "identity file source": "docs/qa/CANDIDATE_UNDER_AUDIT.env",
}


def problems(workflow: Path, env_file: Path) -> list[str]:
    out: list[str] = []
    text = workflow.read_text(encoding="utf-8")
    data = yaml.safe_load(text)
    for m in re.finditer(r"(?<![0-9a-f])([0-9a-f]{64}|[0-9a-f]{40})(?![0-9a-f])", text):
        out.append(f"hard-coded identity literal {m.group(1)[:12]}... in the workflow")
    for line in text.splitlines():
        if line.lstrip().startswith("group:"):
            continue  # concurrency group naming, not an identity
        for expr in re.findall(r"\$\{\{(.*?)\}\}", line):
            if "||" in expr:
                out.append(f"`||` default in expression: {line.strip()}")
    on = data.get("on", data.get(True, {})) or {}
    pr = on.get("pull_request")
    if "pull_request" not in on:
        out.append("no pull_request trigger")
    elif isinstance(pr, dict) and any(k in pr for k in ("paths", "paths-ignore")):
        out.append("pull_request trigger has a paths filter (the audit could be skipped into a stale green)")
    inputs = ((on.get("workflow_dispatch") or {}).get("inputs") or {})
    missing = DISPATCH_INPUTS - set(inputs)
    if missing:
        out.append(f"workflow_dispatch lacks inputs: {sorted(missing)}")
    for name in DISPATCH_INPUTS & set(inputs):
        if not (inputs[name] or {}).get("required"):
            out.append(f"workflow_dispatch input {name} is not required")
    if isinstance(data.get("env"), dict) and any(k in data["env"] for k in KEYS):
        out.append("top-level env still defines candidate identity keys")
    for label, needle in REQUIRED_TEXT.items():
        if needle not in text:
            out.append(f"missing {label} ({needle})")

    if not env_file.is_file():
        out.append(f"{env_file} missing")
        return out
    seen: dict[str, str] = {}
    for raw in env_file.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        key, sep, value = line.partition("=")
        if not sep or key not in KEYS:
            out.append(f"{env_file.name}: unexpected line {line!r}")
            continue
        if key in seen:
            out.append(f"{env_file.name}: duplicate {key}")
        seen[key] = value
        if not re.fullmatch(KEYS[key], value):
            out.append(f"{env_file.name}: {key}={value!r} is not {KEYS[key]}")
    for key in KEYS.keys() - seen.keys():
        out.append(f"{env_file.name}: missing {key}")
    if seen.get("CODE_SHA") and seen.get("ARTIFACT_NAME") and seen.get("NRO_FILE"):
        short = seen["CODE_SHA"][:8]
        if not seen["ARTIFACT_NAME"].endswith(short) or seen["NRO_FILE"] != seen["ARTIFACT_NAME"] + ".nro":
            out.append(f"{env_file.name}: ARTIFACT_NAME/NRO_FILE do not name CODE_SHA {short}")
    return out


def main(argv: list[str]) -> int:
    workflow = Path(argv[1]) if len(argv) > 1 else ROOT / ".github/workflows/app-qa-nro-audit.yml"
    env_file = Path(argv[2]) if len(argv) > 2 else ROOT / "docs/qa/CANDIDATE_UNDER_AUDIT.env"
    found = problems(workflow, env_file)
    for line in found:
        print(f"FAIL  QA-F9 {line}")
    if found:
        return 1
    print("PASS  QA-F9: candidate NRO audit has one identity source, no defaults, API binding and a staleness guard")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
