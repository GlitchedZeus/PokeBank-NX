#!/usr/bin/env python3
"""Reproducible Gen I-IV legality integration gate for one exact checkout.

This audit-owned runner never changes shared Makefiles, Actions, sources, or
saves. It validates an explicit test-program manifest before execution, pins
HEAD/tree identity, applies bounded per-command timeouts, and writes a
machine-readable report.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import NamedTuple

ROOT = Path(__file__).resolve().parents[1]
EMERALD_BLOB = "3e53c1c951876a26cace5dde060cc9ff2c35d0a7"
Q07 = "docs/legality/research/l5/Q07_WC4_EVENT_PROVENANCE_JOIN.json"


class Gate(NamedTuple):
    name: str
    command: list[str]
    environment: dict[str, str]
    runner: str | None


# Deliberately duplicated from lane runners: removing a program from an L1-L4
# list becomes an audit failure rather than an unnoticed green outer script.
PROGRAMS: dict[str, tuple[str, ...]] = {
    "l1": ("test_legality_l1_emerald_methodh", "test_legality_l1_gen3_family_router",
           "test_legality_l1_hgss_special", "test_legality_l1_gen4_nature_replay",
           "test_legality_l1_safari_indexed_original", "test_legality_l1_gamecube_indexed_shadow",
           "test_legality_l1_gen1_original_source", "test_legality_l1_gen2_original_source",
           "test_legality_l1_gen4_static_indexed", "test_legality_l1_gen4_sinnoh_special",
           "test_legality_l1_original_candidate_universe", "test_legality_l1_gen3_original_roamers",
           "test_legality_l1_gamecube_original_fixed"),
    "l2": ("test_legality_l2_dp_alternative_egg_histories", "test_legality_l2_dp_egg_iv",
           "test_legality_l2_dp_egg_pid", "test_legality_l2_dp_native_egg_history",
           "test_legality_l2_gen3_egg_pid_creation", "test_legality_l2_gen3_iv_attested_alternatives",
           "test_legality_l2_gen3_joint_egg_pid_iv", "test_legality_l2_gen3_joint_pid_iv_moves",
           "test_legality_l2_gen3_pid_iv_moves_alternatives", "test_legality_l2_gen4_iv_candidate_set",
           "test_legality_l2_gen4_native_iv_history"),
    "l3": ("test_legality_l3_move_chronology", "test_legality_l3_gba_versioned_levelup",
           "test_legality_l3_gba_starter_evolution", "test_legality_l3_shedinja_copy",
           "test_legality_l3_machine_methods", "test_legality_l3_gba_relearner",
           "test_legality_l3_palpark_joint_stage"),
    "l4": ("test_legality_l4_archive_projection", "test_legality_l4_unified_event_source",
           "test_legality_l4_contest_history", "test_legality_l4_event_text_history",
           "test_legality_l4_gen3_raw_text", "test_legality_l4_attestation_matrix",
           "test_legality_l4_gen1_3_event_sources", "test_legality_l4_native_pk4_projection",
           "test_legality_l4_native_attested_event", "test_legality_l4_ranger_gift_history",
           "test_legality_l4_event_date_history", "test_legality_l4_ball_history",
           "test_legality_l4_other_metadata", "test_legality_l4_trainer_identity",
           "test_legality_l4_native_pk3_text"),
    "l5": ("legality_l5_gen12_sourcefree_contract", "legality_l5_native_codec_batch",
           "legality_l5_ev_native_analyzer_probe", "legality_l5_pk12_sourcefree_native_probe",
           "legality_l5_native_ev_vectors", "audit_legality_l5_native_codec",
           "audit_legality_l5_ev_probe", "audit_legality_l5_pk12_probe"),
    "audit": ("legality_stage2_native_format_properties", "legality_stage3_cross_lane_contracts",
              "legality_l6_stage1_l2_duplicate_route_contract"),
}
RUNNERS = {"l1": "tests/test_legality_l1_run.sh", "l2": "tests/test_legality_l2_run_all.sh",
           "l3": "tests/test_legality_l3_run_all.sh", "l4": "tests/test_legality_l4_runner.sh",
           "l5": "tools/run_legality_integration_audit.py",
           "audit": "tools/run_legality_integration_audit.py"}


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def version(command: list[str]) -> str:
    try:
        return subprocess.check_output(command, cwd=ROOT, text=True).splitlines()[0]
    except (OSError, subprocess.CalledProcessError):
        return "unavailable"


def plan(suite: str, sha: str) -> list[Gate]:
    base = {"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    l5 = [
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
    groups = {
        "l1": [Gate("l1", ["bash", RUNNERS["l1"]], {"L1_EXPECT_SHA": sha}, RUNNERS["l1"])],
        "l2": [Gate("l2", ["bash", RUNNERS["l2"]], {}, RUNNERS["l2"])],
        "l3": [Gate("l3", ["bash", RUNNERS["l3"]], {"L3_EXPECTED_EMERALD_BLOB": EMERALD_BLOB}, RUNNERS["l3"])],
        "l4": [Gate("l4", ["bash", RUNNERS["l4"]], {"L4_L5_Q07_JSON": Q07}, RUNNERS["l4"]),
               Gate("l4-sanitize", ["bash", RUNNERS["l4"]], {"L4_L5_Q07_JSON": Q07, "L4_SANITIZE": "1"}, RUNNERS["l4"])],
        "l5": [Gate(f"l5-{n + 1}", command, {}, RUNNERS["l5"]) for n, command in enumerate(l5)],
        "audit": [
            Gate("stage1-l2-baseline", ["python3", "tools/audit_l6_stage1_baseline.py"], {}, RUNNERS["audit"]),
            Gate("stage2-native", ["python3", "tools/audit_legality_stage2_native_formats.py", "--normal"], {}, RUNNERS["audit"]),
            Gate("stage2-native-sanitize", ["python3", "tools/audit_legality_stage2_native_formats.py", "--sanitize"], {}, RUNNERS["audit"]),
            Gate("stage3-cross-lane", ["python3", "tools/audit_legality_stage3_cross_lane.py", "--normal"], {}, RUNNERS["audit"]),
            Gate("stage3-cross-lane-sanitize", ["python3", "tools/audit_legality_stage3_cross_lane.py", "--sanitize"], {}, RUNNERS["audit"]),
        ],
    }
    selected = tuple(groups) if suite == "all" else (suite,)
    return [Gate(gate.name, gate.command, {**base, **gate.environment}, gate.runner)
            for name in selected for gate in groups[name]]


def fixture_path(program: str) -> Path:
    candidates = (ROOT / "tests" / f"{program}.cpp", ROOT / "tests" / "fixtures" / f"{program}.cpp",
                  ROOT / "tests" / "fixtures" / f"{program}.json", ROOT / "tools" / f"{program}.py",
                  ROOT / "tools" / f"{program}.mk")
    return next((path for path in candidates if path.is_file()), candidates[0])


def validate_manifest(suite: str) -> list[str]:
    selected = tuple(PROGRAMS) if suite == "all" else (suite,)
    errors: list[str] = []
    for lane in selected:
        runner = ROOT / RUNNERS[lane]
        if not runner.is_file():
            errors.append(f"missing runner: {RUNNERS[lane]}")
            continue
        text = runner.read_text(encoding="utf-8")
        for program in PROGRAMS[lane]:
            fixture = fixture_path(program)
            if not fixture.is_file():
                errors.append(f"missing required program source: {fixture.relative_to(ROOT)}")
            elif lane in {"l1", "l2", "l3", "l4"} and program not in text:
                errors.append(f"runner omits required program {program}: {RUNNERS[lane]}")
    for path in (ROOT / Q07, ROOT / "include/Legality/Gen3EmeraldLevelUpData.inc"):
        if not path.is_file():
            errors.append(f"missing required source input: {path.relative_to(ROOT)}")
    return errors


def run_one(gate: Gate, timeout: int) -> dict[str, object]:
    environment = os.environ.copy()
    environment.update(gate.environment)
    started = time.monotonic()
    try:
        completed = subprocess.run(gate.command, cwd=ROOT, text=True, capture_output=True,
                                 env=environment, timeout=timeout, check=False)
        code, timed_out, stdout, stderr = completed.returncode, False, completed.stdout, completed.stderr
    except subprocess.TimeoutExpired as error:
        code, timed_out, stdout, stderr = None, True, error.stdout or "", error.stderr or ""
    seconds = round(time.monotonic() - started, 3)
    if stdout:
        print(stdout, end="" if stdout.endswith("\n") else "\n")
    if stderr:
        print(stderr, end="" if stderr.endswith("\n") else "\n", file=sys.stderr)
    return {"name": gate.name, "command": gate.command, "returncode": code, "timed_out": timed_out,
            "passed": code == 0 and not timed_out, "seconds": seconds, "runner": gate.runner}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=("all", "l1", "l2", "l3", "l4", "l5", "audit"), default="all")
    parser.add_argument("--timeout", type=int, default=900, help="per-command timeout in seconds (default: 900)")
    parser.add_argument("--expect-head", help="refuse any HEAD other than this full SHA")
    parser.add_argument("--expect-tree", help="refuse any tree other than this full SHA")
    parser.add_argument("--output", type=Path, help="write the JSON summary to this path")
    parser.add_argument("--keep-going", action="store_true", help="collect independent failures after the first failure")
    parser.add_argument("--dry-run", action="store_true", help="validate identity and manifest without executing a command")
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    sha, tree = git("rev-parse", "HEAD"), git("rev-parse", "HEAD^{tree}")
    dirty = bool(git("status", "--porcelain", "--untracked-files=no"))
    errors = validate_manifest(args.suite)
    selected = PROGRAMS if args.suite == "all" else (args.suite,)
    result: dict[str, object] = {
        "schema": "POKEBANK_NX_LEGALITY_INTEGRATION_AUDIT_V2", "head": sha, "tree": tree,
        "branch": git("symbolic-ref", "--short", "-q", "HEAD") or "DETACHED",
        "tracked_checkout_clean": not dirty, "suite": args.suite,
        "required_programs": {name: list(PROGRAMS[name]) for name in selected},
        "manifest_errors": errors, "compiler": version(["g++", "--version"]),
        "python": version(["python3", "--version"]), "commands": []}
    identity_error = ((args.expect_head is not None and args.expect_head != sha) or
                      (args.expect_tree is not None and args.expect_tree != tree))
    if dirty:
        result["error"] = "refusing dirty tracked checkout"
    elif identity_error:
        result["error"] = "exact identity mismatch"
    elif errors:
        result["error"] = "required suite program or source input omitted"
    elif args.dry_run:
        result["passed"] = True
    else:
        gates = plan(args.suite, sha)
        for gate in gates:
            print(f"==> {gate.name}: {' '.join(gate.command)}", flush=True)
            row = run_one(gate, args.timeout)
            result["commands"].append(row)
            if not row["passed"] and not args.keep_going:
                break
        result["passed"] = len(result["commands"]) == len(gates) and all(row["passed"] for row in result["commands"])
    if "error" in result:
        result["passed"] = False
    summary = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(summary)
    if args.output:
        args.output.write_text(summary, encoding="utf-8")
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
