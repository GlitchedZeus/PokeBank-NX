#!/usr/bin/env python3
"""Independent L5 tranche-3 QA, no production writes or shared workflow changes.

Example from an accepted legality checkout after forward-only integration:
    python3 tools/audit_legality_l5_major3.py --native --sanitize \
        --source-analyzer src/Legality/Legality.cpp

This validates source-research manifests and a synthetic candidate-contract
fuzzer; it does NOT establish original retail save provenance or PKHeX parity.
"""
from __future__ import annotations
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "docs/legality/research/l5"
TEST = ROOT / "tests/fixtures/legality_l5_witness_bound_fuzz.cpp"
REF_SHA = "d29eada5c087e201e00c749703f1a2de3ddfadeb"


def load(filename: str) -> dict:
    return json.loads((DATA / filename).read_text(encoding="utf-8"))


def q10() -> dict:
    x = load("Q10_PAIRED_ORIGINAL_SOURCE_REQUESTS.json")
    rows = x["source_witness_requests"]
    assert x["catalog"] == "Q10" and len(rows) == x["scenarios"] == 22
    assert len({r["request_id"] for r in rows}) == 22
    assert x["verified_native_original_pairs"] == 0
    assert all(r["original_paired_fixture_digest"] is None
               and r["original_data_consent"] is None
               and not r["independently_approved_source"]
               and r["source_state"] == "missing_original_history"
               and r["expected_verdict_without_external_stage"] == "Incomplete/Unknown"
               for r in rows)
    return {"original_pair_requests": len(rows), "original_pairs_verified": 0}


def q11() -> dict:
    x = load("Q11_ALL_FAMILY_TRIADS_1008.json")
    rows = x["rows"]
    assert x["catalog"] == "Q11" and len(rows) == x["triad_scenarios"] == 1008
    assert len({r["case_id"] for r in rows}) == 1008
    cases = {}
    for r in rows:
        key = (r["game_group"], r["family_id"])
        cases.setdefault(key, set()).add(r["kind"])
        assert not r["independent_verified_method_oracle"]
        assert not r["latest_head_native_test_passed"]
        assert r["fixture_file"] is None and r["original_source_save_pair"] is None
    assert len(cases) == 336
    assert all(v == {"positive", "candidate_negative", "Unknown"}
               for v in cases.values())
    assert sum(r["fixture_grade"] == "not_native_in_format" for r in rows) == 78
    return {"game_family_cells": 336, "three_way_case_rows": 1008,
            "native_not_applicable_case_rows": 78,
            "authenticated_original_method_triads_passed": 0}


def q13() -> dict:
    x = load("Q13_NATIVE_FIELD_CAPABILITY_96.json")
    rows = x["rows"]
    assert x["catalog_id"] == "Q13" and x["source_revision"] == REF_SHA
    assert len(rows) == x["cell_count"] == 96
    fields = {r["field"] for r in rows}
    assert len(fields) == x["field_families"] == 24
    assert {r["generation"] for r in rows} == {"I", "II", "III", "IV"}
    assert len({(r["generation"], r["field"]) for r in rows}) == 96
    assert all(r["source_revision"] == REF_SHA and r["source_line"] > 0
               and r["source_path"].startswith("include/")
               and not r["asserted_original_capture_history"]
               for r in rows)
    assert all({r["generation"] for r in rows if r["field"] == k}
               == {"I", "II", "III", "IV"} for k in fields)
    for g in ("I", "II"):
        for f in ("pid", "modern_ivs", "stored_checksum"):
            r = next(r for r in rows if r["generation"] == g and r["field"] == f)
            assert r["capability"] == "not_in_native_format"
    assert Counter(r["capability"] for r in rows) == x["states"]
    return {"capability_cells": len(rows), "distinct_field_families": len(fields),
            "historical_stage_certifications": 0}


def q14(analyzer: Path | None) -> dict:
    x = load("Q14_PRODUCTION_WIRING_200.json")
    edges = x["edges"]
    assert x["catalog"] == "Q14" and x["analyzer_sha"] == REF_SHA
    assert len(edges) == x["summary"]["lexical_call_occurrences"] == 200
    assert len({(r["namespace"], r["method"]) for r in edges}) == 116
    assert len({r["namespace"] for r in edges}) == 73
    assert x["summary"]["verified_runtime_per_game_calls"] == 0
    assert x["summary"]["verified_pkhex_behavioral_parity_calls"] == 0
    assert all(r["runtime_reachability"] == "unverified"
               and r["pkhex_parity"] == "unverified" for r in edges)
    result = {"production_lexical_calls": len(edges),
              "unique_verifier_methods": 116, "behavioral_parity_verified": 0}
    if analyzer is not None:
        lines = analyzer.read_text(encoding="utf-8").splitlines()
        observed = [(i + 1, m.group(1), m.group(2))
                    for i, line in enumerate(lines)
                    for m in re.finditer(
                        r"\b(Gen(?:12|34|[1-4])[\w]*)::([A-Za-z_]\w*)\s*\(",
                        line)]
        expected = [(r["analyzer_line"], r["namespace"], r["method"])
                    for r in edges]
        if observed != expected:
            raise ValueError("Accepted analyzer call edges drifted: re-audit required")
        result["accepted_analyzer_exact_lexical_edge_match"] = True
    else:
        result["accepted_analyzer_exact_lexical_edge_match"] = "NOT_RUN"
    return result


def q17() -> dict:
    x = load("Q17_GAME_FAMILY_PARITY_336.json")
    rows = x["rows"]
    assert len(rows) == x["scores"]["cells"] == 336
    assert x["scores"]["groups"] == 16 and x["scores"]["families"] == 21
    assert len({(r["group"], r["family_id"]) for r in rows}) == 336
    assert x["scores"]["not_native"] == 26
    assert x["scores"]["potentially_applicable"] == 310
    assert x["scores"]["pkhex_full_parity_cleared_cells"] == 0
    assert all(r["parity_grade"] == "UNVERIFIED_BEHAVIORAL_PARITY"
               and not r["pkhex_reference_differential_executed"]
               and not r["exact_game_specific_runtime_path_executed"]
               for r in rows)
    return {"matrix_cells": 336, "potentially_applicable": 310,
            "not_native": 26, "parity_percentage": "NOT_ESTABLISHED"}


def compile_fuzzer(sanitize: bool) -> dict:
    compiler = shutil.which("g++")
    if not compiler:
        raise RuntimeError("g++ unavailable, native tests NOT RUN")
    flags = (["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
             if sanitize else ["-O2"])
    with tempfile.TemporaryDirectory(prefix="l5-tranche3-") as out:
        exe = Path(out) / "fuzz"
        subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra",
                        "-Wpedantic", "-Werror", *flags, str(TEST),
                        "-o", str(exe)], cwd=ROOT, check=True)
        p = subprocess.run([str(exe)], cwd=ROOT, check=True,
                           text=True, capture_output=True)
    assert "65536 deterministic candidate trials PASS" in p.stdout
    return {"execution": "PASS", "sanitized": sanitize,
            "scope": "synthetic candidate contract only",
            "trials": 65536, "output": p.stdout.strip()}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--source-analyzer", type=Path)
    parser.add_argument("--report", type=Path)
    a = parser.parse_args()
    report = {"schema": "L5_MAJOR3_LOCAL_QA_V1", "Q10": q10(), "Q11": q11(),
              "Q13": q13(), "Q14": q14(a.source_analyzer), "Q17": q17(),
              "semantic_limit": "No global Invalid from a missing original candidate or synthetic stage"}
    if a.native:
        report["Q16_native"] = compile_fuzzer(False)
    if a.sanitize:
        report["Q16_sanitized"] = compile_fuzzer(True)
    text = json.dumps(report, indent=2, sort_keys=True) + "\n"
    print(text)
    if a.report is not None:
        a.report.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
