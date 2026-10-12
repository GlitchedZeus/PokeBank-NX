#!/usr/bin/env python3
"""L5 read-only source/catalog verifier; no save writes or global legality verdicts.

Run from repo root: python3 tools/audit_legality_l5_inventory.py --native
Optional: --source-cache DIR to SHA-256-check exactly pinned source files.
Cache convention: DIR/<owner>/<repo>/<revision>/<repository-path>.
The tool does not download or redistribute retail source or event binaries.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "docs/legality/research/l5"
REGISTRY = BASE / "Q02_FROZEN_SOURCE_REGISTRY.json"
MATRIX = BASE / "Q01_FAMILY_GAME_MATRIX.csv"
EDGES = BASE / "Q03_PRODUCTION_CALL_EDGES.csv"
ENCOUNTERS = BASE / "Q06_ENCOUNTER_CATEGORY_INVENTORY.csv"
FIXTURES = BASE / "Q09_FIXTURE_PROVENANCE.csv"
NATIVE = ROOT / "tests/fixtures/legality_l5_native_encounters.cpp"
PIN = "fc0225e6351af62365f29200300323e38f52c8fe"
BASE_SHA = "0dccf2e10289164ad0d065985dcb29f14fd2fb35"

def rows(path):
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))

def check_registry(cache):
    data = json.loads(REGISTRY.read_text(encoding="utf-8"))
    ids = [r["id"] for r in data]
    assert len(ids) == len(set(ids)) == 79, "registry ID duplication or truncation"
    results = Counter()
    blocked = []
    for record in data:
        assert re.fullmatch(r"[a-f0-9]{40}", record["revision"]), record["id"]
        assert re.fullmatch(r"[a-f0-9]{64}", record["sha256"]), record["id"]
        assert record["source_path"] and record["repository"] and record["evidence_grade"] in ("A", "B")
        if not record["symbol"]:
            blocked.append({"id": record["id"], "issue": "no pinned symbol"})
        if cache is None:
            results["unverified_hash_no_source_cache"] += 1
            continue
        parts = record["repository"].split("/")
        assert len(parts) == 2 and all(parts)
        candidate = cache.joinpath(*parts, record["revision"], record["source_path"])
        if not candidate.is_file():
            results["unverified_missing_local_source"] += 1
            blocked.append({"id": record["id"], "issue": "missing source", "path": str(candidate)})
            continue
        body = candidate.read_bytes()
        actual = hashlib.sha256(body).hexdigest()
        if actual != record["sha256"]:
            results["hash_mismatch"] += 1
            blocked.append({"id": record["id"], "issue": "digest differs",
                            "expected": record["sha256"], "actual": actual})
        elif record["symbol"] and record["symbol"].encode() not in body:
            results["symbol_not_found"] += 1
            blocked.append({"id": record["id"], "issue": "pinned symbol missing"})
        else:
            results["pinned_file_verified"] += 1
    return {"count": len(data), "grades": dict(Counter(r["evidence_grade"] for r in data)),
            "source_status": dict(results), "blockers": blocked}

def check_remote_batches():
    """Verify downloaded GitHub exact-pin SHA reports, not retail legitimacy."""
    registry = json.loads(REGISTRY.read_text(encoding="utf-8"))
    expected = {record["id"]: record for record in registry}
    seen = {}
    for i, count in enumerate((14, 14, 14, 14, 14, 9), 1):
        path = BASE / f"Q02_REMOTE_HASH_BATCH_{i:02d}.json"
        batch = json.loads(path.read_text(encoding="utf-8"))
        assert len(batch["rows"]) == count, path
        for r in batch["rows"]:
            assert r["id"] in expected and r["id"] not in seen
            ref = expected[r["id"]]
            assert r["sha256_match"] is True, r["id"]
            assert r["observed_sha256"] == ref["sha256"], r["id"]
            if ref["symbol"]:
                assert r["symbol_match"] is True, r["id"]
            else:
                assert r["symbol_match"] is None, r["id"]
            seen[r["id"]] = r
    assert len(seen) == len(expected) == 79
    return {"source_hashes_exact_pin_verified": 79,
            "pinned_symbols_verified": sum(bool(x["symbol"]) for x in registry),
            "symbol_not_specified": [x["id"] for x in registry if not x["symbol"]],
            "license_reuse_approved": False, "original_stage_certified": False}

def check_pkhex_method_index():
    ids = set()
    count = 0
    for i, length in enumerate((11, 11, 9), 1):
        part = json.loads((BASE / f"Q03_PKHEX_PINNED_METHODS_{i:02d}.json").read_text(encoding="utf-8"))
        assert len(part["rows"]) == length
        for r in part["rows"]:
            assert r["id"] not in ids and not r["error"]
            ids.add(r["id"])
            assert r["method_count"] == len(r["method_declarations"])
            count += r["method_count"]
    assert len(ids) == 31 and count == 305
    return {"pinned_source_files_inspected": len(ids), "lexical_method_declarations": count,
            "full_pkhex_call_graph": False, "all_game_conditions_covered": False}

def check_matrix():
    data = rows(MATRIX)
    groups = sorted(set(r["game_group_id"] for r in data))
    families = sorted(set(int(r["family_id"]) for r in data))
    keys = [(r["game_group_id"], r["family_id"]) for r in data]
    assert len(groups) == 16 and families == list(range(1, 22))
    assert len(data) == len(set(keys)) == 336, "missing/duplicate verifier-game cell"
    assert all(r["cell_evidence_state"] != "implemented_full_parity" for r in data)
    return {"source_groups": groups, "family_count": len(families),
            "cells": len(data), "states": dict(Counter(r["cell_evidence_state"] for r in data))}

def check_edges(analyzer):
    data = rows(EDGES)
    assert data and len(set((r["line"], r["callee_namespace"], r["callee_method"]) for r in data)) == len(data)
    if analyzer:
        source = analyzer.read_text(encoding="utf-8").splitlines()
        drift = []
        for row in data:
            n = int(row["line"])
            if n > len(source) or row["callee_namespace"] + "::" + row["callee_method"] not in source[n-1]:
                drift.append(row)
        assert not drift, f"analyzer exact-head drift in {len(drift)} edges; audit cannot be reused blindly"
    return {"lexically_observed_calls": len(data), "not_automatically_production_reachable": len(data),
            "original_sha": BASE_SHA, "analyzer_checked": bool(analyzer)}

def multiset_diff(expected, actual):
    # Complete tuple, not a set: identical persisted identities may encode distinct source histories.
    def key(row):
        return tuple((name, str(row[name])) for name in sorted(row) if name != "comment")
    left = Counter(map(key, expected))
    right = Counter(map(key, actual))
    return {"missing_original_rows": sum((left-right).values()),
            "extra_generated_rows": sum((right-left).values()),
            "equal": left == right}

def check_encounters(source_rows, generated_rows):
    data = rows(ENCOUNTERS)
    keys = [(r["game"], r["method_id"]) for r in data]
    assert len(data) == len(set(keys)) == 46
    total = sum(int(r["generated_rows_at_research_pin"]) for r in data)
    assert total == 20147
    counts = dict(Counter(r["game"] for r in data))
    result = {"generated_categories": len(data), "generated_rows": total,
              "game_categories": counts, "original_source_row_reconciliation": "blocked"}
    if source_rows and generated_rows:
        source = rows(source_rows)
        emitted = rows(generated_rows)
        assert list(source[0]) == list(emitted[0]), "source/generated schemas differ"
        result["multiset"] = multiset_diff(source, emitted)
        result["original_source_row_reconciliation"] = "compared; inspect differences"
    return result

def check_fixtures():
    data = rows(FIXTURES)
    assert len(data) == 12 and len(set(r["fixture_id"] for r in data)) == 12
    assert Counter(r["kind"] for r in data) == {
        "positive": 4, "candidate_negative": 4, "Unknown": 4}
    assert all(r["missing_original_stage"] and "retail" not in r["provenance_grade"].lower()
               for r in data)
    return {"synthetic_native_conditions": len(data),
            "kinds": dict(Counter(r["kind"] for r in data)), "authenticated_retail_specimens": 0}

def native_test(sanitize=False):
    compiler = shutil.which("g++")
    if not compiler:
        return {"status": "NOT_RUN", "reason": "g++ missing"}
    with tempfile.TemporaryDirectory(prefix="l5-native-") as temp:
        executable = Path(temp) / "l5_encounters"
        flags = (["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                 if sanitize else ["-O2"])
        subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", *flags, "-I" + str(ROOT / "include"),
                        str(NATIVE), "-o", str(executable)], cwd=ROOT, check=True)
        env = os.environ.copy()
        if sanitize:
            env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
        result = subprocess.run([str(executable)], cwd=ROOT, env=env, capture_output=True,
                                text=True, check=True)
    return {"status": "PASS", "stdout": result.stdout.strip()}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source-cache", type=Path)
    ap.add_argument("--analyzer", type=Path, help="accepted exact-head Legality.cpp; drift guard")
    ap.add_argument("--source-rows", type=Path, help="external pinned original full encounter rows CSV")
    ap.add_argument("--generated-rows", type=Path, help="generated complete-identity encounter rows CSV")
    ap.add_argument("--native", action="store_true", help="compile and execute independent new C++ fixture")
    ap.add_argument("--native-sanitize", action="store_true", help="run L5 native fixture with ASan/UBSan (no shared CI changes)")
    ap.add_argument("--json-out", type=Path, help="optional audit output, never modifies source saves")
    args = ap.parse_args()
    if bool(args.source_rows) != bool(args.generated_rows):
        ap.error("original and generated rows must be supplied together")
    result = {"schema": "l5_audit_v1", "base_sha": BASE_SHA, "research_pin": PIN,
              "Q01": check_matrix(), "Q02": check_registry(args.source_cache),
              "Q02_live": check_remote_batches(), "Q03": check_edges(args.analyzer),
              "Q03_reference": check_pkhex_method_index(),
              "Q06": check_encounters(args.source_rows, args.generated_rows),
              "Q09": check_fixtures()}
    if args.native:
        result["native"] = native_test()
    if args.native_sanitize:
        result["native_asan_ubsan"] = native_test(sanitize=True)
    message = json.dumps(result, indent=2, sort_keys=True)
    print(message)
    if args.json_out:
        args.json_out.write_text(message + "\n", encoding="utf-8")
    if result["Q02"]["source_status"].get("hash_mismatch", 0):
        raise SystemExit("Source hash mismatch: human review required (no global Invalid)")
    if result["Q02"]["source_status"].get("symbol_not_found", 0):
        raise SystemExit("Source symbol mismatch: human review required")

if __name__ == "__main__":
    main()
