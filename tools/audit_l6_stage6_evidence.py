#!/usr/bin/env python3
"""Compile and execute the audit-owned L6 Stage 6 receipt protocol.

The native fixture uses synthetic receipt metadata only. A pass proves that
stale, partial, dry-run, duplicate, and malformed evidence cannot be promoted
to a release-ready matrix; it does not authenticate an original Pokémon or
authorize production acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = "tests/fixtures/legality_l6_stage6_evidence.cpp"
EXPECTED_CHECKS = 67974
QUALIFICATION = "tests/fixtures/legality_l6_stage6_qualification.cpp"
Q17 = "docs/legality/research/l5/Q17_GAME_FAMILY_PARITY_336.json"
Q07 = "docs/legality/research/l5/Q07_WC4_EVENT_PROVENANCE_JOIN.json"
PROJECTION = "include/Legality/Gen4EventTemplateData.inc"
REFERENCE = "6501f0ab46e8f8ca048539dbaf8cae8cb104e722"


def catalog_input(scoreboard: dict, events: dict) -> str:
    """Only the reviewed unqualified catalog schema may enter this adapter.

    Future claimed original/runtime receipts need a separate verification
    adapter; never silently discard a newly asserted proof or treat it as true.
    """
    def require(ok: bool, message: str) -> None:
        if not ok:
            raise ValueError(message)
    require(scoreboard.get("schema") == "L5_Q17_336_CELL_EVIDENCE_SCOREBOARD_V1", "Q17 schema")
    require(events.get("schema") == "L5_Q07_GEN4_247_EVENT_ARCHIVE_JOIN_V1", "Q07 schema")
    rows = scoreboard["rows"]
    groups = sorted({r["group"] for r in rows})
    require(len(groups) == 16 and len(rows) == 336, "Q17 coverage")
    expected = {(group, family) for group in groups for family in range(1, 22)}
    require({(r["group"], r["family_id"]) for r in rows} == expected, "Q17 cell identities")
    lines = []
    for row in rows:
        require(all(row[field] is False for field in (
            "exact_game_specific_runtime_path_executed", "pkhex_reference_differential_executed",
            "real_original_specimen_with_complete_game_history", "fully_closed_candidate_set")),
            "Q17 evidence promotion requires a reviewed proof adapter")
        require(row["parity_grade"] == "UNVERIFIED_BEHAVIORAL_PARITY" and
                type(row["cell_passing_native_tests"]) is int and row["cell_passing_native_tests"] == 0,
                "Q17 unverified grade changed")
        lines.append(f'{groups.index(row["group"])} {row["family_id"] - 1}')
    require(events["source"]["pkhex_revision"] == REFERENCE, "reference revision drift")
    require(events["copying_binaries_permitted"] is False, "archive redistribution policy changed")
    require(len(events["rows"]) == 247, "event coverage")
    for index, row in enumerate(events["rows"]):
        require(row["template_index"] == index, "event index duplicate or missing")
        require(row["original_archive_sha256"] is None and row["archive_path"] is None and
                row["archive_provenance"] == "unverified" and row["original_release_receipt"] == "unknown",
                "event provenance promotion requires a reviewed proof adapter")
        require(row["license_redistribution_clearance"] == "not established", "event rights changed")
        lines.append(" ".join(str(row[key]) for key in ("template_index", "species", "tid", "sid", "card_id")))
    return "\n".join(lines) + "\n"


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def run(compiler: str, sanitize: bool, expected_head: str | None,
        expected_tree: str | None) -> dict[str, object]:
    started = time.monotonic()
    head, tree = git("rev-parse", "HEAD"), git("rev-parse", "HEAD^{tree}")
    clean = not bool(git("status", "--porcelain", "--untracked-files=normal"))
    if expected_head is not None and expected_head != head:
        raise RuntimeError(f"exact HEAD mismatch: expected {expected_head}, got {head}")
    if expected_tree is not None and expected_tree != tree:
        raise RuntimeError(f"exact tree mismatch: expected {expected_tree}, got {tree}")
    if (expected_head or expected_tree) and not clean:
        raise RuntimeError("pinned execution requires a clean checkout including untracked sources")
    scoreboard = json.loads((ROOT / Q17).read_text(encoding="utf-8"))
    events = json.loads((ROOT / Q07).read_text(encoding="utf-8"))
    catalog = catalog_input(scoreboard, events)
    if git("hash-object", PROJECTION) != events["source"]["blob_sha"]:
        raise RuntimeError("production event projection blob drift")
    inputs = ("tools/audit_l6_stage6_evidence.py", FIXTURE, QUALIFICATION, "tools/legality_l6_stage6_evidence.h",
              "tools/legality_l6_stage6_qualification.h", Q17, Q07, PROJECTION)
    digests = {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in inputs}
    with tempfile.TemporaryDirectory(prefix="pbnx-l6-stage6-") as folder:
        target = str(Path(folder) / "stage6-evidence")
        command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   "-I.", "-Iinclude", "-Wno-unused-parameter", FIXTURE]
        command += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                    if sanitize else ["-O2"])
        command += ["-o", target]
        subprocess.run(command, cwd=ROOT, check=True, timeout=300)
        environment = os.environ.copy()
        environment.update({"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
        executed = subprocess.run([target], cwd=ROOT, text=True, capture_output=True,
                                   check=True, timeout=120, env=environment)
        command[command.index(FIXTURE)] = QUALIFICATION
        subprocess.run(command, cwd=ROOT, check=True, timeout=300)
        qualified = subprocess.run([target], cwd=ROOT, text=True, capture_output=True,
                                  check=True, timeout=120, env=environment)
        integrated = subprocess.run([target, "--catalog"], input=catalog, cwd=ROOT, text=True,
                                   capture_output=True, check=True, timeout=120, env=environment)
    matched = re.search(r"L6 Stage 6 release evidence protocol: (\d+) PASS", executed.stdout)
    if matched is None or int(matched.group(1)) != EXPECTED_CHECKS:
        raise RuntimeError("unexpected Stage 6 output: " + executed.stdout + executed.stderr)
    if "L6 Stage 6 parity qualification: 12525 PASS" not in qualified.stdout:
        raise RuntimeError("unexpected qualification output: " + qualified.stdout)
    if "336 cells Unknown, 247 events Unknown; 495 PASS" not in integrated.stdout:
        raise RuntimeError("unexpected catalog output: " + integrated.stdout)
    if digests != {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in inputs}:
        raise RuntimeError("source inputs changed during execution")
    if head != git("rev-parse", "HEAD") or tree != git("rev-parse", "HEAD^{tree}"):
        raise RuntimeError("Git identity changed during execution")
    return {"compiler": compiler, "sanitized": sanitize, "checks": EXPECTED_CHECKS,
            "compiler_version": subprocess.check_output([compiler, "--version"], text=True).splitlines()[0],
            "head": head, "tree": tree, "status": "PASS",
            "qualification_checks": 12525, "catalog_checks": 495,
            "checkout_clean": clean, "seconds": round(time.monotonic() - started, 3),
            "source_sha256": digests, "reference_revision": REFERENCE,
            "pkhex_runtime_executed": False, "qualified_parity_cells": 0,
            "acceptance_authorized": False, "stdout": executed.stdout.strip(),
            "qualification_stdout": qualified.stdout.strip(), "catalog_stdout": integrated.stdout.strip()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--normal", action="store_true")
    mode.add_argument("--sanitize", action="store_true")
    parser.add_argument("--compiler", choices=("g++", "clang++"), default="g++")
    parser.add_argument("--expect-head")
    parser.add_argument("--expect-tree")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    profiles = [False] if args.normal else [True] if args.sanitize else [False, True]
    runs = []
    for profile in profiles:
        try:
            runs.append(run(args.compiler, profile, args.expect_head, args.expect_tree))
        except (OSError, ValueError, KeyError, RuntimeError, subprocess.SubprocessError) as error:
            runs.append({"compiler": args.compiler, "sanitized": profile, "status": "FAIL",
                         "error": f"{type(error).__name__}: {error}", "acceptance_authorized": False})
    data = {"schema": "POKEBANK_NX_L6_STAGE6_EVIDENCE_V1", "runs": runs,
            "passed": all(row["status"] == "PASS" for row in runs)}
    output = json.dumps(data, indent=2, sort_keys=True) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")
    return 0 if data["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
