#!/usr/bin/env python3
"""L5 Stage 1: independent 336-cell provenance and evidence-boundary gate.

Reads *actual* Q01/Q17 archival manifests; does not infer game/method parity
from synthetic native tests, a source snippet or a historical green CI run.

  python3 tools/audit_legality_l5_stage1_parity_gate.py --verify-manifests
  python3 tools/audit_legality_l5_stage1_parity_gate.py --self-test
  python3 tools/audit_legality_l5_stage1_parity_gate.py \
      --current-integration-head <40-digit-current-PR298-head>

Without --verify-manifests, certification is fail-closed: manifest correctness
alone is not a release certificate. No network, original save or branch writes.
"""
from __future__ import annotations

import argparse
from collections import Counter
import copy
import csv
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "docs/legality/research/l5"
SHA = re.compile(r"^[0-9a-f]{40}$")
FILES = ("Q01_FAMILY_GAME_MATRIX.csv",
         "Q17_GAME_FAMILY_PARITY_336.json",
         "Q17_INTEGRATION_NATIVE_EVIDENCE_336.json")
GROUPS = {"RB": 1, "Y": 1, "GS": 2, "C": 2,
          "R": 3, "S": 3, "E": 3, "FR": 3, "LG": 3,
          "COLO": 3, "XD": 3,
          "D": 4, "P": 4, "Pt": 4, "HG": 4, "SS": 4}
FAMILIES = set(range(1, 22))
EXPECTED = {(g, f) for g in GROUPS for f in FAMILIES}


def load(root: Path = DATA) -> tuple[list[dict], dict, dict]:
    with (root / FILES[0]).open(encoding="utf-8", newline="") as stream:
        matrix = list(csv.DictReader(stream))
    with (root / FILES[1]).open(encoding="utf-8") as stream:
        earlier = json.load(stream)
    with (root / FILES[2]).open(encoding="utf-8") as stream:
        integrated = json.load(stream)
    return matrix, earlier, integrated


def validate(matrix: list[dict], older: dict, newer: dict,
             current_integration_head: str | None = None) -> dict:
    failures: list[str] = []
    blockers: list[str] = []

    def require(condition: bool, code: str) -> None:
        if not condition:
            failures.append(code)

    require(len(matrix) == 336, "MATRIX_SIZE")
    require(isinstance(older.get("rows"), list), "MISSING_BASE_ROWS")
    require(isinstance(newer.get("rows"), list), "MISSING_INTEGRATED_ROWS")
    a = older.get("rows", [])
    b = newer.get("rows", [])
    require(len(a) == 336, "BASE_336")
    require(len(b) == 336, "INTEGRATED_336")
    require(older.get("schema") == "L5_Q17_336_CELL_EVIDENCE_SCOREBOARD_V1",
            "BASE_SCHEMA")
    require(newer.get("schema") == "L5_Q17_INTEGRATION_INDEPENDENT_336_V1",
            "INTEGRATED_SCHEMA")

    def index(rows: list[dict], group_name: str, family_name: str,
              prefix: str) -> dict[tuple[str, int], dict]:
        result = {}
        for row in rows:
            try:
                key = (row[group_name], int(row[family_name]))
                if key in result:
                    failures.append(prefix + "_DUPLICATE:" + str(key))
                result[key] = row
            except (KeyError, TypeError, ValueError):
                failures.append(prefix + "_INVALID_KEY")
        require(set(result) == EXPECTED, prefix + "_MISSING_OR_EXTRA_KEYS")
        return result

    im = index(matrix, "game_group_id", "family_id", "MATRIX")
    ia = index(a, "group", "family_id", "BASE")
    ib = index(b, "game_group", "family_id", "INTEGRATED")
    accepted = older.get("accepted_analyzer")
    anchored = newer.get("integration_head")
    require(isinstance(accepted, str) and bool(SHA.fullmatch(accepted)),
            "INVALID_ACCEPTED_SHA")
    require(isinstance(anchored, str) and bool(SHA.fullmatch(anchored)),
            "INVALID_INTEGRATION_SHA")
    require(newer.get("total_game_family_cells") == 336, "TOTAL_CELL_COUNT")
    require(newer.get("l5_original_scoreboard_blob")
            and bool(SHA.fullmatch(newer["l5_original_scoreboard_blob"])),
            "MISSING_SCOREBOARD_SOURCE_BLOB")

    not_native = 0
    certified = 0
    authenticated = 0
    bad_cells = 0
    for key in sorted(EXPECTED):
        m, x, y = im.get(key), ia.get(key), ib.get(key)
        if not (m and x and y):
            continue
        g, f = key
        for origin, row in (("MATRIX", m), ("BASE", x),
                            ("INTEGRATED", y)):
            try:
                require(int(row["generation"]) == GROUPS[g],
                        origin + "_GEN:" + str(key))
            except (KeyError, ValueError, TypeError):
                failures.append(origin + "_GEN_MISSING:" + str(key))
        require(m.get("family_name") == x.get("family_name")
                == y.get("family_name"), "FAMILY_DRIFT:" + str(key))
        require(m.get("local_applicability") == x.get("local_applicability"),
                "APPLICABILITY_DRIFT:" + str(key))
        require(x.get("accepted_analyzer_current_sha") == accepted,
                "ACCEPTED_SHA_DRIFT:" + str(key))
        native = x.get("local_applicability") == "potentially_applicable"
        require(x.get("local_applicability")
                in ("potentially_applicable", "not_native_in_this_format"),
                "INVALID_APPLICABILITY:" + str(key))
        if not native:
            not_native += 1
            require(x.get("reference_cell") == "not_applicable_native_fields",
                    "NOT_NATIVE_REFERENCE:" + str(key))
            require(x.get("cell_passing_native_tests") == 0,
                    "NOT_NATIVE_TEST_CREDIT:" + str(key))
        require(isinstance(x.get("cell_passing_native_tests"), int)
                and x["cell_passing_native_tests"] >= 0,
                "INVALID_TEST_COUNT:" + str(key))
        if x.get("pkhex_reference_differential_executed"):
            require(native, "REFERENCE_ON_UNSTORED_FIELD:" + str(key))
        x_proven = x.get("parity_grade") == "VERIFIED_FULL_PKHEX_PARITY"
        y_proven = y.get("pkhex_exact_game_full_parity_proven") is True
        if x_proven or y_proven:
            certified += 1
            # Necessary, never sufficient: genuine paired history and
            # executable reference + format-specific full analyzer paths.
            required_x = ("exact_game_specific_runtime_path_executed",
                          "pkhex_reference_differential_executed",
                          "real_original_specimen_with_complete_game_history",
                          "fully_closed_candidate_set")
            require(native and all(x.get(k) is True for k in required_x)
                    and x.get("cell_passing_native_tests", 0) > 0,
                    "UNSUPPORTED_BASE_CERTIFICATE:" + str(key))
            require(y_proven and x_proven
                    and y.get("original_retail_history_proven") is True
                    and y.get("game_source_original_method_oracle") is not None,
                    "UNSUPPORTED_INTEGRATION_CERTIFICATE:" + str(key))
        if y.get("original_retail_history_proven") is True:
            authenticated += 1
            require(x.get("real_original_specimen_with_complete_game_history")
                    is True, "UNPAIRED_ORIGINAL_CLAIM:" + str(key))
        if y.get("source_free_false_invalid_in_production_source"):
            bad_cells += 1
            require(y.get("source_contract_reproduced") is True
                    and y.get("coverage_grade") == "CONFIRMED_SEMANTIC_DEFECT",
                    "UNSUPPORTED_DEFECT_ANNOTATION:" + str(key))
        if y.get("actual_native_pk1_pk2_analyzer_case_executed"):
            require(y.get("coverage_grade") != "RESEARCH_ONLY_OR_UNTESTED",
                    "UNMAPPED_NATIVE_CASE:" + str(key))
        if "PK3" in str(y.get("synthetic_test_scope", "")):
            require(not y_proven and y.get("game_source_original_method_oracle")
                    is None, "SYNTHETIC_TO_PARITY_PROMOTION:" + str(key))
    scores = older.get("scores", {})
    for k, value in (("cells", 336), ("groups", 16), ("families", 21),
                     ("not_native", not_native),
                     ("potentially_applicable", 336 - not_native),
                     ("pkhex_full_parity_cleared_cells", certified),
                     ("authenticated_retail_history_cells", authenticated)):
        require(scores.get(k) == value, "SCORE_DRIFT:" + k)
    require(newer.get("exact_game_retail_native_parity_complete") == certified,
            "INTEGRATION_SCORE_DRIFT")
    require(newer.get("original_paired_save_histories_verified") == 0,
            "UNVERIFIED_PAIR_CREDIT")
    require(newer.get("original_attested_event_archive_items") == 0,
            "UNVERIFIED_ARCHIVE_CREDIT")
    require(authenticated == 0, "UNSUPPORTED_ORIGINAL_HISTORY_PROMOTION")
    require(certified == 0, "UNSUPPORTED_FULL_PARITY_PROMOTION")
    require(any(row.get("source_free_false_invalid_in_production_source")
                for row in b), "MISSING_RECORDED_SOURCEFREE_DEFECTS")
    runs = newer.get("integration_runs", [])
    require(bool(runs) and isinstance(runs, list), "MISSING_RUN_EVIDENCE")
    for run in runs if isinstance(runs, list) else []:
        require(run.get("sha") == anchored
                and run.get("status") == "completed"
                and run.get("conclusion") == "success",
                "ARCHIVED_RUN_IDENTITY_OR_STATUS")
        require(run.get("real_original_retail_specimens") is False,
                "SYNTHETIC_RUN_AS_ORIGINAL")
    if current_integration_head is None:
        blockers.append("NO_LIVE_INTEGRATION_SHA_SUPPLIED")
    elif not SHA.fullmatch(current_integration_head):
        failures.append("MALFORMED_CURRENT_INTEGRATION_SHA")
    elif current_integration_head != anchored:
        blockers.append("STALE_INTEGRATION_EVIDENCE")
    # A stored historical green Actions run never verifies the present L5
    # head, present L1-L4 heads, complete original receipts, or PKHeX parity.
    blockers.extend(("NO_AUTHENTICATED_ORIGINAL_METHOD_PAIRS",
                     "NO_EXECUTABLE_EXACT_GAME_PKHEX_FULL_PARITY"))
    return {
        "schema": "L5_STAGE1_336_CELL_FAIL_CLOSED_V1",
        "manifest_integrity": "PASS" if not failures else "FAIL",
        "release_state": "BLOCKED",
        "historical_integration_head": anchored,
        "current_integration_head": current_integration_head,
        "counts": {"matrix_cells": len(matrix), "base_cells": len(a),
                   "integrated_cells": len(b), "not_native": not_native,
                   "potentially_applicable": 336 - not_native,
                   "full_parity_certified": certified,
                   "authenticated_histories": authenticated,
                   "sourcefree_defect_cells": bad_cells},
        "failures": sorted(set(failures)),
        "blockers": sorted(set(blockers)),
        "scope": "Historical source/fixture evidence consistency only; not runtime PKHeX parity or release approval",
    }


def self_test(matrix: list[dict], a: dict, b: dict) -> None:
    baseline = validate(matrix, a, b, b["integration_head"])
    assert baseline["manifest_integrity"] == "PASS", baseline["failures"]
    assert baseline["release_state"] == "BLOCKED"
    assert baseline["counts"]["full_parity_certified"] == 0
    cases = [
        ("duplicate matrix", lambda m, x, y: m.append(copy.deepcopy(m[0]))),
        ("duplicate baseline", lambda m, x, y: x["rows"].append(copy.deepcopy(x["rows"][0]))),
        ("remove integration", lambda m, x, y: y["rows"].pop()),
        ("family label drift", lambda m, x, y: y["rows"][0].update(family_name="FAKE")),
        ("generation drift", lambda m, x, y: x["rows"][0].update(generation=4)),
        ("source SHA drift", lambda m, x, y: x["rows"][0].update(accepted_analyzer_current_sha="0"*40)),
        ("applicability drift", lambda m, x, y: x["rows"][0].update(local_applicability="not_native_in_this_format")),
        ("fabricated parity", lambda m, x, y: y["rows"][0].update(pkhex_exact_game_full_parity_proven=True)),
        ("fabricated original", lambda m, x, y: y["rows"][0].update(original_retail_history_proven=True)),
        ("inflated scoreboard", lambda m, x, y: x["scores"].update(pkhex_full_parity_cleared_cells=1)),
        ("inflated integration", lambda m, x, y: y.update(exact_game_retail_native_parity_complete=1)),
        ("fake archive", lambda m, x, y: y.update(original_attested_event_archive_items=1)),
        ("fake pair", lambda m, x, y: y.update(original_paired_save_histories_verified=1)),
        ("synthetic original", lambda m, x, y: y["integration_runs"][0].update(real_original_retail_specimens=True)),
        ("run SHA spoof", lambda m, x, y: y["integration_runs"][0].update(sha="0"*40)),
        ("incorrect test credit", lambda m, x, y: next(r for r in x["rows"] if r["local_applicability"] == "not_native_in_this_format").update(cell_passing_native_tests=1)),
        ("lost defect trace", lambda m, x, y: next(r for r in y["rows"] if r["source_free_false_invalid_in_production_source"]).update(source_contract_reproduced=False)),
    ]
    for title, mutation in cases:
        m, x, y = copy.deepcopy((matrix, a, b))
        mutation(m, x, y)
        result = validate(m, x, y, b["integration_head"])
        assert result["manifest_integrity"] == "FAIL", (title, result)
    stale = validate(matrix, a, b, "f"*40)
    assert "STALE_INTEGRATION_EVIDENCE" in stale["blockers"]
    missing = validate(matrix, a, b)
    assert "NO_LIVE_INTEGRATION_SHA_SUPPLIED" in missing["blockers"]
    malformed = validate(matrix, a, b, "not-a-sha")
    assert malformed["manifest_integrity"] == "FAIL"
    print("L5 Stage1 parity gate: baseline + 17 adversarial mutations + 3 live-head controls PASS")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--verify-manifests", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--current-integration-head", type=str)
    ap.add_argument("--report", type=Path)
    args = ap.parse_args()
    matrix, old, new = load()
    if args.self_test:
        self_test(matrix, old, new)
        return 0
    result = validate(matrix, old, new, args.current_integration_head)
    encoded = json.dumps(result, sort_keys=True, indent=2) + "\n"
    print(encoded, end="")
    if args.report:
        args.report.write_text(encoded, encoding="utf-8")
    if result["manifest_integrity"] != "PASS":
        return 2
    return 0 if args.verify_manifests else 1


if __name__ == "__main__":
    raise SystemExit(main())
