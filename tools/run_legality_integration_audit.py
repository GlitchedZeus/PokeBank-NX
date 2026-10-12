#!/usr/bin/env python3
"""Run the independent Gen I-IV legality gates against one exact checkout.

This intentionally does not modify the shared host Makefile or CI workflow.  It
is an audit-side runner for integration heads: every invoked command is the
same production-linked lane gate used by the coordinator, and its JSON output
records the immutable Git identity and every command result.

Examples:
  python3 tools/run_legality_integration_audit.py --suite l3
  python3 tools/run_legality_integration_audit.py --output /tmp/legality.json
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[1]
EMERALD_BLOB = "3e53c1c951876a26cace5dde060cc9ff2c35d0a7"
L5_COMMANDS = [
    ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2",
     "tests/fixtures/legality_l5_gen12_sourcefree_contract.cpp", "-o", "/tmp/l5-pk12-source-contract"],
    ["/tmp/l5-pk12-source-contract"],
    ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1",
     "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
     "tests/fixtures/legality_l5_gen12_sourcefree_contract.cpp", "-o", "/tmp/l5-pk12-source-contract-sanitize"],
    ["/tmp/l5-pk12-source-contract-sanitize"],
    ["python3", "tools/audit_legality_l5_native_ev_vectors.py", "--source-root", "."],
    ["python3", "tools/audit_legality_l5_native_codec.py", "--source-pin"],
    ["python3", "tools/audit_legality_l5_native_codec.py", "--source-pin", "--sanitize"],
    ["make", "-f", "Makefile.host", "-f", "tools/audit_legality_l5_ev_probe.mk", "l5-ev-fixed"],
    ["make", "-f", "Makefile.host", "-f", "tools/audit_legality_l5_ev_probe.mk", "l5-ev-fixed-sanitize"],
    ["make", "-f", "Makefile.host", "-f", "tools/audit_legality_l5_pk12_probe.mk", "l5-pk12-fixed"],
    ["make", "-f", "Makefile.host", "-f", "tools/audit_legality_l5_pk12_probe.mk", "l5-pk12-fixed-sanitize"],
]


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def plan(suite: str, sha: str) -> list[tuple[str, list[str], dict[str, str]]]:
    base = {"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    q07 = "docs/legality/research/l5/Q07_WC4_EVENT_PROVENANCE_JOIN.json"
    gates = {
        "l1": [("l1", ["bash", "tests/test_legality_l1_run.sh"], {"L1_EXPECT_SHA": sha})],
        "l2": [("l2", ["bash", "tests/test_legality_l2_run_all.sh"], {})],
        "l3": [("l3", ["bash", "tests/test_legality_l3_run_all.sh"],
                {"L3_EXPECTED_EMERALD_BLOB": EMERALD_BLOB})],
        "l4": [("l4", ["bash", "tests/test_legality_l4_runner.sh"], {"L4_L5_Q07_JSON": q07}),
               ("l4-sanitize", ["bash", "tests/test_legality_l4_runner.sh"],
                {"L4_L5_Q07_JSON": q07, "L4_SANITIZE": "1"})],
        "l5": [(f"l5-{index + 1}", command, {}) for index, command in enumerate(L5_COMMANDS)],
    }
    selected = gates if suite == "all" else {suite: gates[suite]}
    return [(name, command, {**base, **extra})
            for entries in selected.values() for name, command, extra in entries]


def run_one(name: str, command: list[str], added_env: dict[str, str], timeout: int) -> dict:
    environment = os.environ.copy()
    environment.update(added_env)
    started = time.monotonic()
    try:
        completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                                 env=environment, timeout=timeout, check=False)
        code, timed_out = completed.returncode, False
        stdout, stderr = completed.stdout, completed.stderr
    except subprocess.TimeoutExpired as error:
        code, timed_out = None, True
        stdout = error.stdout or ""
        stderr = error.stderr or ""
    seconds = round(time.monotonic() - started, 3)
    if stdout:
        print(stdout, end="" if stdout.endswith("\n") else "\n")
    if stderr:
        print(stderr, end="" if stderr.endswith("\n") else "\n", file=sys.stderr)
    return {"name": name, "command": command, "returncode": code,
            "timed_out": timed_out, "passed": code == 0 and not timed_out,
            "seconds": seconds}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=("all", "l1", "l2", "l3", "l4", "l5"), default="all")
    parser.add_argument("--timeout", type=int, default=900,
                        help="per-command timeout in seconds (default: 900)")
    parser.add_argument("--output", type=Path, help="optional JSON summary path")
    parser.add_argument("--keep-going", action="store_true",
                        help="continue after a failure to collect independent results")
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    sha = git("rev-parse", "HEAD")
    tree = git("rev-parse", "HEAD^{tree}")
    dirty = bool(git("status", "--porcelain", "--untracked-files=no"))
    result = {"schema": "POKEBANK_NX_LEGALITY_INTEGRATION_AUDIT_V1", "head": sha,
              "tree": tree, "tracked_checkout_clean": not dirty, "suite": args.suite,
              "commands": []}
    if dirty:
        result["error"] = "refusing dirty tracked checkout"
        print(json.dumps(result, indent=2, sort_keys=True))
        return 2
    for name, command, env in plan(args.suite, sha):
        print(f"==> {name}: {' '.join(command)}", flush=True)
        row = run_one(name, command, env, args.timeout)
        result["commands"].append(row)
        if not row["passed"] and not args.keep_going:
            break
    result["passed"] = len(result["commands"]) == len(plan(args.suite, sha)) and all(
        row["passed"] for row in result["commands"])
    summary = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(summary)
    if args.output:
        args.output.write_text(summary, encoding="utf-8")
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
