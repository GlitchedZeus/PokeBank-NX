#!/usr/bin/env python3
"""Fail-closed L6 Stage 2 native legality matrix for a combined checkout.

The receipt-backed program lists are intentionally larger than the historical
brief where current lane heads have added coverage (31 L1 and 19 L3).  Before
executing anything, this tool proves every declared source is present and named
by its owning runner.  It never calls an incomplete combined checkout green.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
from typing import NamedTuple

ROOT = Path(__file__).resolve().parents[1]
Q07 = "docs/legality/research/l5/Q07_WC4_EVENT_PROVENANCE_JOIN.json"


class Lane(NamedTuple):
    receipt_ref: str
    receipt_sha: str
    runner: str
    programs: tuple[str, ...]


LANES: dict[str, Lane] = {
    "l1": Lane("origin/feature/legality-l1-encounters-rng-20261012",
               "36f1f147dbea4a7607ec7440e83ce608147eaa19", "tests/test_legality_l1_run.sh", (
        "test_legality_l1_emerald_methodh", "test_legality_l1_gen3_family_router",
        "test_legality_l1_hgss_special", "test_legality_l1_gen4_nature_replay",
        "test_legality_l1_safari_indexed_original", "test_legality_l1_gamecube_indexed_shadow",
        "test_legality_l1_gen1_original_source", "test_legality_l1_gen1_wild_fishing",
        "test_legality_l1_gen2_original_source", "test_legality_l1_gen2_fishing",
        "test_legality_l1_gen2_tree_contest", "test_legality_l1_gen2_swarms",
        "test_legality_l1_gen2_roamer_routes", "test_legality_l1_gen4_static_indexed",
        "test_legality_l1_gen4_sinnoh_special", "test_legality_l1_stage4_hgss_tree",
        "test_legality_l1_stage4_honey", "test_legality_l1_stage4_method_jk",
        "test_legality_l1_stage4_native_pk3_pk4", "test_legality_l1_stage5_status_bridge",
        "test_legality_l1_stage5_ledger", "test_legality_l1_stage5_permutations",
        "test_legality_l1_stage5_real_sources", "test_legality_l1_original_candidate_universe",
        "test_legality_l1_gen3_original_roamers", "test_legality_l1_gen3_wild_catalog",
        "test_legality_l1_gen3_wild_rng", "test_legality_l1_gen3_roamer_frame",
        "test_legality_l1_gamecube_original_fixed", "test_legality_l1_stage1_production_crypto",
        "test_legality_l1_stage1_gamecube_enum_domains")),
    "l2": Lane("origin/feature/legality-l2-breeding-20261012",
               "6198ac5dfd867bf756de2458eeee3ae0645fa797", "tests/test_legality_l2_run_all.sh", (
        "test_legality_l2_dp_alternative_egg_histories", "test_legality_l2_dp_move_stage",
        "test_legality_l2_dp_egg_iv", "test_legality_l2_dp_egg_pid",
        "test_legality_l2_dp_native_egg_history", "test_legality_l2_gen3_egg_pid_creation",
        "test_legality_l2_gen3_iv_attested_alternatives", "test_legality_l2_gen3_joint_egg_pid_iv",
        "test_legality_l2_gen3_joint_pid_iv_moves", "test_legality_l2_gen3_pid_iv_moves_alternatives",
        "test_legality_l2_gen3_retail_daycare_pair", "test_legality_l2_gen4_pt_hgss_pid",
        "test_legality_l2_gen4_pt_hgss_joint", "test_legality_l2_gen4_pt_hgss_move_stage",
        "test_legality_l2_gen4_pt_hgss_full_moves", "test_legality_l2_gen4_iv_candidate_set",
        "test_legality_l2_gen4_native_iv_history")),
    "l3": Lane("origin/feature/legality-l3-moves-transfer-20261012",
               "2de53eb330f09c13c112d5270d36110c26cd33ec", "tests/test_legality_l3_run_all.sh", (
        "test_legality_l3_original_source_digest", "test_legality_l3_original_source_fingerprints",
        "test_legality_l3_gen12_joint_chronology", "test_legality_l3_gba_original_full_tables",
        "test_legality_l3_gba_original_machine_tables", "test_legality_l3_gba_full_machine_classification",
        "test_legality_l3_joint_evolution_machine", "test_legality_l3_joint_evolution_retail",
        "test_legality_l3_platinum_native_sources", "test_legality_l3_palpark_native_joint",
        "test_legality_l3_palpark_platinum_source_joint", "test_legality_l3_palpark_native_retail",
        "test_legality_l3_move_chronology", "test_legality_l3_gba_versioned_levelup",
        "test_legality_l3_gba_starter_evolution", "test_legality_l3_shedinja_copy",
        "test_legality_l3_machine_methods", "test_legality_l3_gba_relearner",
        "test_legality_l3_palpark_joint_stage")),
    "l4": Lane("origin/feature/legality-l4-events-meta-20261012",
               "3c8c28d597cfc3bc982782ed9ec8ec708a3284ae", "tests/test_legality_l4_runner.sh", (
        "test_legality_l4_archive_projection", "test_legality_l4_unified_event_source",
        "test_legality_l4_contest_history", "test_legality_l4_event_text_history",
        "test_legality_l4_gen3_raw_text", "test_legality_l4_attestation_matrix",
        "test_legality_l4_gen1_3_event_sources", "test_legality_l4_gen12_gift_history",
        "test_legality_l4_gen3_distribution_matrix", "test_legality_l4_gen4_pcd_pgt_container",
        "test_legality_l4_gen4_mystery_gift_histories", "test_legality_l4_historical_event_path",
        "test_legality_l4_clock_receipt_history", "test_legality_l4_historical_adversarial",
        "test_legality_l4_stage5_coordinator_join", "test_legality_l4_stage5_lane_reconciliation",
        "test_legality_l4_native_pk4_projection", "test_legality_l4_native_attested_event",
        "test_legality_l4_ranger_gift_history", "test_legality_l4_event_date_history",
        "test_legality_l4_ball_history", "test_legality_l4_other_metadata",
        "test_legality_l4_trainer_identity", "test_legality_l4_gen4_mystery_gift_native",
        "test_legality_l4_native_pk3_text", "test_legality_l4_gen3_distribution_native_pk3",
        "test_legality_l4_gen3_palpark_metadata_bridge", "test_legality_l4_stage5_native_pk3_join")),
    "l5": Lane("origin/audit/legality-l5-research-qa-20261012",
               "6f3e12eff17980d6e0b5857f8b470c2ed50a5033", "tools/run_legality_integration_audit.py", (
        "legality_l5_gen12_sourcefree_contract", "legality_l5_native_codec_batch",
        "legality_l5_ev_native_analyzer_probe", "legality_l5_pk12_sourcefree_native_probe",
        "legality_l5_native_ev_vectors", "audit_legality_l5_native_codec",
        "audit_legality_l5_ev_probe", "audit_legality_l5_pk12_probe")),
    "audit": Lane("audit/legality-integration-hardening-20261012", "LOCAL",
                  "tools/run_legality_integration_audit.py", (
        "legality_l6_stage1_l2_duplicate_route_contract", "legality_stage2_native_format_properties",
        "legality_stage3_cross_lane_contracts", "legality_l6_stage2_bounds_contract",
        "legality_l6_stage2_joint_headers", "legality_l6_stage2_pk4_offset_native",
        "legality_l6_stage4_gate_ledger", "legality_l6_stage5_joint_history",
        "legality_l6_stage5_native_analyzer", "legality_l6_stage6_evidence",
        "legality_l6_stage6_qualification")),
}

# L5's synthetic EV verifier intentionally binds its result to the exact
# production codec/wrapper source it exercised.  Surface those receipts during
# matrix preflight too, so an integration branch cannot reach a misleading
# partial green before the L5 command reports source drift.
SOURCE_PINS: dict[str, dict[str, str]] = {
    "l5": {
        "src/Encryption/Encryption3FRLG.cpp": "eec27b73a15d6652371ed2e066690ef0fa9fb7ef",
        "src/Encryption/Encryption4.cpp": "406935761713db95ae738398b65ba0a37d4cf97b",
        "include/Encryption/Encryption.h": "9eab685b76511d5ea08e0fffdff63529a5824fe6",
        "include/Pokemon/Pokemon3FRLG.h": "9f0e1ede52d338e90a969dba8ecd0f081e3a88ea",
        "src/Pokemon/Pokemon4ReadOnly.cpp": "021a4ed89c6ede16c4e3e0045506a03e9e3bbb24",
    },
}


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def candidates(program: str) -> tuple[Path, ...]:
    return (ROOT / "tests" / f"{program}.cpp", ROOT / "tests" / "fixtures" / f"{program}.cpp",
            ROOT / "tests" / "fixtures" / f"{program}.json", ROOT / "tools" / f"{program}.py",
            ROOT / "tools" / f"{program}.mk")


def inventory(name: str, lane: Lane) -> dict[str, object]:
    runner = ROOT / lane.runner
    text = runner.read_text(encoding="utf-8") if runner.is_file() else ""
    missing_sources = [program for program in lane.programs
                       if not any(path.is_file() for path in candidates(program))]
    missing_registration = [program for program in lane.programs if program not in text]
    source_pin_mismatches = []
    for relative, expected in SOURCE_PINS.get(name, {}).items():
        path = ROOT / relative
        actual = git("hash-object", relative) if path.is_file() else None
        if actual != expected:
            source_pin_mismatches.append({"path": relative, "expected": expected, "actual": actual})
    return {"receipt_ref": lane.receipt_ref, "receipt_sha": lane.receipt_sha,
            "runner": lane.runner, "expected_programs": len(lane.programs),
            "present_sources": len(lane.programs) - len(missing_sources),
            "registered_programs": len(lane.programs) - len(missing_registration),
            "missing_sources": missing_sources, "missing_registration": missing_registration,
            "source_pin_mismatches": source_pin_mismatches,
            "ready": runner.is_file() and not missing_sources and not missing_registration and
            not source_pin_mismatches}


def preflight() -> dict[str, object]:
    lanes = {name: inventory(name, lane) for name, lane in LANES.items()}
    return {"schema": "POKEBANK_NX_L6_STAGE2_MATRIX_V1", "head": git("rev-parse", "HEAD"),
            "tree": git("rev-parse", "HEAD^{tree}"), "tracked_checkout_clean":
            not bool(git("status", "--porcelain", "--untracked-files=no")),
            "required_native_programs": sum(len(lane.programs) for lane in LANES.values()),
            "lanes": lanes, "ready": all(row["ready"] for row in lanes.values())}


def run_one(name: str, command: list[str], environment: dict[str, str]) -> dict[str, object]:
    started = time.monotonic()
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                               env=environment, check=False)
    seconds = round(time.monotonic() - started, 3)
    if completed.stdout:
        print(completed.stdout, end="" if completed.stdout.endswith("\n") else "\n")
    if completed.stderr:
        print(completed.stderr, end="" if completed.stderr.endswith("\n") else "\n", file=sys.stderr)
    return {"name": name, "command": command, "returncode": completed.returncode,
            "seconds": seconds, "passed": completed.returncode == 0}


def execution_plan(compiler: str, head: str, tree: str) -> list[tuple[str, list[str], dict[str, str]]]:
    environment = {**os.environ, "CXX": compiler, "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                   "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    exact = ["--expect-head", head, "--expect-tree", tree, "--compiler", compiler]
    return [
        ("l1", ["bash", LANES["l1"].runner], {**environment, "L1_EXPECT_SHA": head}),
        ("l2", ["bash", LANES["l2"].runner], environment),
        ("l3", ["bash", LANES["l3"].runner], {**environment,
         "L3_EXPECTED_EMERALD_BLOB": "3e53c1c951876a26cace5dde060cc9ff2c35d0a7"}),
        ("l4-normal", ["bash", LANES["l4"].runner], {**environment, "L4_L5_Q07_JSON": Q07}),
        ("l4-sanitize", ["bash", LANES["l4"].runner], {**environment,
         "L4_L5_Q07_JSON": Q07, "L4_SANITIZE": "1"}),
        ("l5", ["python3", "tools/run_legality_integration_audit.py", "--suite", "l5", *exact], environment),
        ("audit", ["python3", "tools/run_legality_integration_audit.py", "--suite", "audit", *exact], environment),
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true", help="execute the full matrix only after preflight succeeds")
    parser.add_argument("--compiler", choices=("g++", "clang++"), default="g++")
    parser.add_argument("--expect-head")
    parser.add_argument("--expect-tree")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    report = preflight()
    if args.expect_head and args.expect_head != report["head"] or args.expect_tree and args.expect_tree != report["tree"]:
        report["identity_error"] = True
    if shutil.which(args.compiler) is None:
        report["compiler_error"] = f"compiler unavailable: {args.compiler}"
    report["compiler"] = args.compiler
    report["executed"] = []
    if args.execute and report["ready"] and not report.get("identity_error") and not report.get("compiler_error") and report["tracked_checkout_clean"]:
        for name, command, environment in execution_plan(args.compiler, report["head"], report["tree"]):
            row = run_one(name, command, environment)
            report["executed"].append(row)
            if not row["passed"]:
                break
    elif args.execute:
        report["execution_blocked"] = "matrix preflight, identity, compiler, or checkout cleanliness failed"
    report["passed"] = bool(report["ready"] and not report.get("identity_error") and
                            not report.get("compiler_error") and report["tracked_checkout_clean"] and
                            (not args.execute or len(report["executed"]) == 7 and
                             all(row["passed"] for row in report["executed"])))
    output = json.dumps(report, indent=2, sort_keys=True) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")
    return 0 if report["passed"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
