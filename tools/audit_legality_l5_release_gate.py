#!/usr/bin/env python3
"""Independent L5 Q18 machine-checkable release audit (research-only, no writes).

A successful MANIFEST validation is not full-engine release approval.
Run from a checkout:
    python3 tools/audit_legality_l5_release_gate.py --verify-manifests
    python3 tools/audit_legality_l5_release_gate.py  # returns 1 while blocked
"""
from __future__ import annotations
import argparse
from collections import Counter
import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
D = ROOT / "docs/legality/research/l5"

def read(name: str) -> dict:
    return json.loads((D / name).read_text(encoding="utf-8"))

def audit() -> dict:
    q01 = list(csv.DictReader((D / "Q01_FAMILY_GAME_MATRIX.csv").open(encoding="utf-8", newline="")))
    q02 = read("Q02_VERIFICATION_SUMMARY.json")
    q03 = read("Q03_PKHEX_REFERENCE_SUMMARY.json")
    q05 = read("Q05_GBA_386_LEVELUP_SOURCE_INDEX.json")
    q07 = read("Q07_WC4_EVENT_PROVENANCE_JOIN.json")
    q10 = read("Q10_PAIRED_ORIGINAL_SOURCE_REQUESTS.json")
    q11 = read("Q11_ALL_FAMILY_TRIADS_1008.json")
    q12 = read("Q12_EV_HARDCAP_FALSE_INVALID_DIFFERENTIAL.json")
    egg = read("Q12_EGG_NONZERO_EV_MISSING_RULE.json")
    q13 = read("Q13_NATIVE_FIELD_CAPABILITY_96.json")
    q14 = read("Q14_PRODUCTION_WIRING_200.json")
    q17 = read("Q17_GAME_FAMILY_PARITY_336.json")
    q18 = read("Q18_FALSE_INVALID_SITE_RISK_36.json")
    gates = read("Q18_RELEASE_GATES_TRANCHE04.json")
    assert len(q01) == 336
    assert {(r["game_group_id"],r["family_id"]) for r in q01} == {
        (r["group"],str(r["family_id"])) for r in q17["rows"]}
    assert q02["matching_sha256"] == q02["entries"] == 79
    assert q03["lexical_method_declarations"] == 305
    assert len(q05["species"]) == 386
    assert all(len(r["source"]) == 5 for r in q05["species"])
    assert len(q07["rows"]) == 247
    assert q07["metrics"]["original_card_receipts_verified"] == 0
    assert q07["metrics"]["archive_item_sha256_verified"] == 0
    assert len(q10["source_witness_requests"]) == 22
    assert q10["verified_native_original_pairs"] == 0
    assert len(q11["rows"]) == 1008
    assert q11["original_method_native_triads_passed"] == 0
    assert q12["source_semantics"]["nx_per_stat_invalid_if_greater_than"] == 252
    assert q12["source_semantics"]["pkhex_gen3_4_max_ev_per_stat"] == 255
    assert q12["scope_bounded_sweep"]["all_delta_combinations"] == 3054
    assert len(egg["controls"]) == 7
    assert all(egg["source_proof"].values())
    assert len(q13["rows"]) == 96
    assert len(q14["edges"]) == 200
    assert len(q17["rows"]) == 336
    assert q17["scores"]["pkhex_full_parity_cleared_cells"] == 0
    assert len(q18["sites"]) == 36
    assert q18["verified_reference_discrepancies"] == 1
    assert len(gates["gate_rows"]) == 14
    assert len({g["id"] for g in gates["gate_rows"]}) == 14
    assert gates["proven_source_semantic_deltas"] == 2
    assert gates["proven_original_whole_pokemon_bad_verdict_cases"] == 0
    assert not gates["release_ready"]
    assert all(g["state"] != "READY" for g in gates["gate_rows"])
    assert q18["sites"][0]["site_id"].startswith("NXINVALID-")
    confirmed = [x for x in q18["sites"] if
                 x["confidence"] == "CONFIRMED_SOURCE_SEMANTIC_MISMATCH"]
    assert len(confirmed) == 1 and confirmed[0]["analyzer_line"] == 1629
    triads = Counter(r["kind"] for r in q11["rows"])
    assert triads == Counter({"positive":336, "candidate_negative":336,"Unknown":336})
    issues = [
        {"issue_id":"NX-EV-253-255", "kind":"FALSE_INVALID_RULE",
         "source_reference":"Q12_EV_HARDCAP_FALSE_INVALID_DIFFERENTIAL.json",
         "original_native_regression_passed":False,
         "required_action":"correct per-stat cap for native Gen III-IV (255)"},
        {"issue_id":"NX-EGG-EV", "kind":"MISSING_REFERENCE_INVALID_RULE",
         "source_reference":"Q12_EGG_NONZERO_EV_MISSING_RULE.json",
         "original_native_regression_passed":False,
         "required_action":"add explicit unhatched Gen III-IV egg nonzero EV rejection"}
    ]
    return {
        "schema":"L5_Q18_RELEASE_AUDIT_RESULT_V1",
        "research_manifest_validation":"PASS",
        "full_pkhex_parity_release_ready":False,
        "source_semantic_findings":issues,
        "gate_counts":dict(Counter(g["state"] for g in gates["gate_rows"])),
        "source_pins_79":q02["matching_sha256"],
        "accepted_analyzer_lexical_edges":len(q14["edges"]),
        "original_native_pairs_authenticated":q10["verified_native_original_pairs"],
        "event_originals_authenticated":q07["metrics"]["original_card_receipts_verified"],
        "native_per_game_family_parity_verified":q17["scores"]["pkhex_full_parity_cleared_cells"],
        "meaning_of_success":"Source research manifest integrity only; no auto-merge, no full Pokémon legality certification."
    }

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify-manifests",action="store_true",
                        help="Exit 0 if research manifests are internally consistent even though full release is blocked")
    parser.add_argument("--report",type=Path)
    args=parser.parse_args()
    result=audit()
    summary=json.dumps(result,indent=2,sort_keys=True)+"\n"
    print(summary)
    if args.report is not None:
        args.report.write_text(summary,encoding="utf-8")
    if args.verify_manifests:
        return 0
    return 0 if result["full_pkhex_parity_release_ready"] else 1

if __name__=="__main__":
    raise SystemExit(main())
