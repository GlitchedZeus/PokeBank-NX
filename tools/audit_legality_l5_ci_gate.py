#!/usr/bin/env python3
"""L5 independent exact-head CI acceptance gate.

Consumes trusted, freshly exported GitHub Actions metadata in an evidence JSON
file, NOT a workflow dispatch or a GitHub API client. No network, CI or repo
mutation. Any absent, skipped or queued evidence causes a block.

Usage:
    python3 tools/audit_legality_l5_ci_gate.py evidence.json
    python3 tools/audit_legality_l5_ci_gate.py --self-test
Exit 0 only if every required check, SHA and forward-only ancestry gate pass.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from typing import Any

REQUIRED = (
    "Verify exact pull-request application identity",
    "Run host tests",
    "Focused RSE save-open bridge regression",
    "Run ASan and UBSan tests",
)


def gate(x: dict[str, Any]) -> dict[str, Any]:
    reasons: list[str] = []
    head = x.get("expected_pr_head")
    if not isinstance(head, str) or len(head) != 40:
        reasons.append("Missing 40-digit expected exact PR SHA")
    elif not all(c in "0123456789abcdef" for c in head.lower()):
        reasons.append("Malformed expected exact PR SHA")
    if head != x.get("observed_run_head_sha"):
        reasons.append("Workflow run is not tied to the expected PR SHA")
    if x.get("accepted_behind_count") != 0:
        reasons.append("Development head is behind accepted legality branch")
    if x.get("run_status") != "completed" or x.get("run_conclusion") != "success":
        reasons.append("Latest exact-head workflow is not completed successfully")
    jobs = x.get("jobs", [])
    if not isinstance(jobs, list) or not jobs:
        reasons.append("No GitHub Actions job evidence supplied")
        jobs = []
    steps: dict[str, set[str]] = {name: set() for name in REQUIRED}
    for job in jobs:
        if not isinstance(job, dict):
            reasons.append("Malformed job record")
            continue
        if job.get("status") != "completed" or job.get("conclusion") != "success":
            reasons.append("At least one required workflow job not completed successfully")
        for s in job.get("steps", []):
            if isinstance(s, dict) and s.get("name") in steps:
                steps[s["name"]].add(str(s.get("conclusion")))
    for name, observed in steps.items():
        if observed != {"success"}:
            reasons.append(f"Missing, skipped, failed or ambiguous required step {name}: {sorted(observed)}")
    if x.get("independent_l5_native") != "success":
        reasons.append("L5 standalone source-scope native test not passed on exact-head checkout")
    if x.get("independent_l5_sanitized") != "success":
        reasons.append("L5 standalone source-scope ASan/UBSan test not passed on exact-head checkout")
    if x.get("original_source_license_review") != "complete":
        reasons.append("Source/data import license review not signed off")
    # A CI-ready state does not imply PKHeX parity. Unobservable histories
    # remain unresolved regardless of the statuses above.
    return {"status": "READY_FOR_HUMAN_REVIEW" if not reasons else "BLOCKED",
            "expected_pr_head": head, "reasons": reasons,
            "scope": "CI/research acceptance preflight, never merge or game legality certification"}


def self_test() -> None:
    base = {"expected_pr_head": "a"*40,
            "observed_run_head_sha": "a"*40,
            "accepted_behind_count": 0,
            "run_status": "completed", "run_conclusion": "success",
            "independent_l5_native": "success",
            "independent_l5_sanitized": "success",
            "original_source_license_review": "complete",
            "jobs": [{"status": "completed", "conclusion": "success",
                      "steps": [{"name": name, "conclusion": "success"} for name in REQUIRED]}]}
    assert gate(base)["status"] == "READY_FOR_HUMAN_REVIEW"
    for key, replacement in (
        ("observed_run_head_sha", "b"*40),
        ("accepted_behind_count", 2),
        ("run_status", "queued"),
        ("run_conclusion", None),
        ("independent_l5_native", "not_run"),
        ("independent_l5_sanitized", "not_run"),
        ("original_source_license_review", "pending"),
    ):
        case = dict(base, **{key: replacement})
        assert gate(case)["status"] == "BLOCKED", key
    for ix in range(len(REQUIRED)):
        for conclusion in ("skipped", "failure", "cancelled", None):
            jobs = [{"status": "completed", "conclusion": "success",
                     "steps": [{"name": name,
                                "conclusion": conclusion if i == ix else "success"}
                               for i, name in enumerate(REQUIRED)]}]
            case = dict(base, jobs=jobs)
            assert gate(case)["status"] == "BLOCKED", (ix, conclusion)
    assert gate(dict(base, jobs=[]))["status"] == "BLOCKED"
    assert gate(dict(base, jobs=[{"status": "queued", "conclusion": None,
                                 "steps": []}]))["status"] == "BLOCKED"
    print("L5 CI gate: 26 self-test cases PASS (only fully evidenced exact-head candidate ready)")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("evidence", type=Path, nargs="?")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.evidence is None:
        ap.error("Provide evidence JSON or use --self-test")
    data = json.loads(args.evidence.read_text(encoding="utf-8"))
    result = gate(data)
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "READY_FOR_HUMAN_REVIEW" else 1


if __name__ == "__main__":
    raise SystemExit(main())
