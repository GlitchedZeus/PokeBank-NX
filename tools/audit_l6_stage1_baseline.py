#!/usr/bin/env python3
"""Reproduce and classify the L6 Stage 1 integration baseline.

The lane-owned L2 script is executed exactly as registered. Its historical
duplicate-route assertion is reported as a red baseline, never as a passing
test. A separate audit-owned native contract then verifies the non-authenticating
duplicate-route behavior under GCC/Clang, normal and ASan/UBSan builds.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = "tests/fixtures/legality_l6_stage1_l2_duplicate_route_contract.cpp"
KNOWN_FAILURE = "MissingOrDuplicateCandidateIdentity"


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def run(command: list[str], environment: dict[str, str] | None = None) -> dict[str, object]:
    started = time.monotonic()
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                               env=environment, check=False)
    return {"command": command, "returncode": completed.returncode,
            "seconds": round(time.monotonic() - started, 3),
            "stdout": completed.stdout, "stderr": completed.stderr}


def native(compiler: str, sanitize: bool) -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="pbnx-l6-stage1-") as directory:
        executable = Path(directory) / "duplicate_route"
        command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   "-Iinclude", FIXTURE]
        command += (["-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                    if sanitize else ["-O2"])
        command += ["-o", str(executable)]
        build = run(command)
        if build["returncode"] != 0:
            raise RuntimeError("native contract build failed: " + build["stderr"])
        environment = {"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                       "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
        executed = run([str(executable)], environment)
    expected = "L6 Stage 1 L2 duplicate-route native contracts: 128 PASS"
    if executed["returncode"] != 0 or expected not in executed["stdout"]:
        raise RuntimeError("native contract execution failed: " + executed["stdout"] + executed["stderr"])
    return {"compiler": compiler, "sanitized": sanitize, "status": "PASS",
            "cases": 128, "seconds": round(build["seconds"] + executed["seconds"], 3),
            "stdout": executed["stdout"].strip()}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expect-head", help="refuse a checkout other than this SHA")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    head = git("rev-parse", "HEAD")
    if args.expect_head and args.expect_head != head:
        raise SystemExit("exact HEAD mismatch: expected " + args.expect_head + ", got " + head)
    lane = run(["bash", "tests/test_legality_l2_run_all.sh"])
    reproduced = lane["returncode"] != 0 and KNOWN_FAILURE in (lane["stdout"] + lane["stderr"])
    reports = [native(compiler, sanitize) for compiler in ("g++", "clang++")
               for sanitize in (False, True)]
    report = {
        "schema": "POKEBANK_NX_L6_STAGE1_BASELINE_V1",
        "head": head, "tree": git("rev-parse", "HEAD^{tree}"),
        "lane_l2": {**lane, "classification": "known_stale_duplicate_route_assertion" if reproduced
                    else "unexpected_l2_result", "baseline_reproduced": reproduced},
        "native_duplicate_route_contracts": reports,
        "passed": reproduced and len(reports) == 4,
    }
    output = json.dumps(report, indent=2, sort_keys=True) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")
    if not report["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
