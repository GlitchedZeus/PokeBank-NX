#!/usr/bin/env python3
"""Independent L5 Q04/Q05/Q07/Q08 source-reference QA (no saves or writes).

Run from repo root:
 python3 tools/audit_legality_l5_major2.py --native --sanitize
The native test is source-contract-only, NOT a reconstructed original game binary
or complete PK1-PK4 legality test. No shared Makefiles/CI are changed.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
F = ROOT / "docs/legality/research/l5"
NATIVE = ROOT / "tests/fixtures/legality_l5_source_gate_triad.cpp"
HEX64 = re.compile(r"^[a-f0-9]{64}$")

def load(name):
    return json.loads((F / name).read_text(encoding="utf-8"))

def q04():
    x = load("Q04_DIAMOND_PEARL_SOURCE_STAGE.json")
    assert x["catalog_id"] == "Q04"
    assert x["source"]["revision"] == "b623abef1b4dfe204ca50aab42447ac00c134290"
    assert HEX64.fullmatch(x["source"]["sha256_utf8"])
    assert HEX64.fullmatch(x["rng_support"]["sha256_utf8"])
    names = {p["symbol"] for p in x["source_symbols"] if p["symbol_found"]}
    assert {"ov05_021EC8A8", "ov05_021EC944", "ov05_021EC99C",
            "ov05_021EC9D4", "ov05_Daycare_CreateEgg"} <= names
    assert x["binary_correspondence"]["diamond"] == "unverified"
    assert x["binary_correspondence"]["pearl"] == "unverified"
    assert x["binary_correspondence"]["arm_assembly_executed"] == "no"
    assert Counter(p["kind"] for p in x["candidate_vectors"]) == {
        "positive": 2, "candidate_negative": 2, "Unknown": 2}
    return {"original_source_symbols": len(names), "stage_cases": 6,
            "source_hash_present": True, "original_ARM_execution": False}

def q05():
    x = load("Q05_GBA_386_LEVELUP_SOURCE_INDEX.json")
    games = ("ruby", "sapphire", "emerald", "firered", "leafgreen")
    assert set(x["game_sources"]) == set(games)
    assert len(x["species"]) == 386
    for i, sp in enumerate(x["species"], 1):
        assert sp["dex"] == i and sp["species_symbol"]
        assert 1 <= sp["internal_species_id"] <= 411
        assert set(sp["source"]) == set(games)
        for game in games:
            y = sp["source"][game]
            assert y["symbol"].endswith("LevelUpLearnset")
            assert y["entry_count"] > 0
            assert y["source_line"] > 0
            assert HEX64.fullmatch(y["ordered_token_sha256"])
    totals = {g: sum(sp["source"][g]["entry_count"] for sp in x["species"])
              for g in games}
    assert totals == {g: x["game_sources"][g]["total_ordered_entries"]
                      for g in games}
    assert totals == {"ruby": 3947, "sapphire": 3947, "emerald": 3947,
                      "firered": 4011, "leafgreen": 4013}
    for sp in x["species"]:
        a, b = sp["source"]["firered"], sp["source"]["leafgreen"]
        if a["ordered_token_sha256"] != b["ordered_token_sha256"]:
            assert sp["species_symbol"] in {"DUGTRIO", "DEOXYS"}
    difference = {sp["species_symbol"] for sp in x["species"]
                  if sp["source"]["firered"]["ordered_token_sha256"] !=
                     sp["source"]["leafgreen"]["ordered_token_sha256"]}
    assert difference == {"DUGTRIO", "DEOXYS"}
    assert {sp["source"]["firered"]["ordered_token_sha256"] for sp in x["species"]}
    assert len(x["version_differences"]) == 60
    return {"source_cells": 386 * 5, "levels_by_game": totals,
            "fire_red_leaf_green_differences": sorted(difference),
            "original_save_acquisition_history_verified": False}

def q07():
    x = load("Q07_WC4_EVENT_PROVENANCE_JOIN.json")
    r = x["rows"]
    assert len(r) == 247
    assert [p["template_index"] for p in r] == list(range(247))
    assert len(set(p["identity_key"] for p in r)) == 247
    assert all(p["archive_provenance"] == "unverified"
               and p["original_archive_sha256"] is None
               and p["archive_path"] is None
               and p["original_release_receipt"] == "unknown"
               and p["license_redistribution_clearance"] == "not established"
               for p in r)
    count = Counter(p["card_id"] for p in r)
    assert len(count) == 70
    assert sum(count[p["card_id"]] > 1 for p in r) == 219
    assert x["metrics"]["templates"] == 247
    return {"structural_templates": 247, "card_ids": 70,
            "card_id_collision_rows": 219, "original_archives_authenticated": 0}

def q08():
    x = load("Q08_REDISTRIBUTION_GATE.json")
    assert x["catalog_id"] == "Q08" and len(x["repo_rows"]) == 14
    assert x["dispositions"]["third_party_code_import"] == "BLOCKED"
    assert all(not p["legal_review_complete"] and
               p["copy_source_code"] == "blocked_pending_review" and
               p["redistribute_event_or_pokemon_payloads"] == "blocked_pending_review"
               for p in x["repo_rows"])
    return {"source_licensing_records": len(x["repo_rows"]),
            "bulk_import_clearances": 0, "shipping_license_review_complete": False}

def native(sanitize=False):
    compiler = shutil.which("g++")
    if not compiler:
        raise RuntimeError("g++ unavailable; native source tests NOT RUN")
    with tempfile.TemporaryDirectory(prefix="legality-l5-major2-") as t:
        executable = Path(t) / "source_contract"
        flags = (["-O1", "-fsanitize=address,undefined",
                  "-fno-omit-frame-pointer"] if sanitize else ["-O2"])
        subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra",
                        "-Wpedantic", "-Werror", *flags,
                        str(NATIVE), "-o", str(executable)], check=True, cwd=ROOT)
        run = subprocess.run([str(executable)], check=True, cwd=ROOT,
                             capture_output=True, text=True)
    return {"status": "PASS", "sanitized": sanitize, "output": run.stdout.strip()}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--native", action="store_true")
    ap.add_argument("--sanitize", action="store_true")
    ap.add_argument("--json-out", type=Path)
    args = ap.parse_args()
    result = {"schema": "L5_MAJOR2_AUDIT_V1", "Q04": q04(), "Q05": q05(),
              "Q07": q07(), "Q08": q08(),
              "semantic_limit": "source-scope and candidate-local; no global Invalid"}
    if args.native:
        result["Q11_native"] = native(False)
    if args.sanitize:
        result["Q11_asan_ubsan"] = native(True)
    report = json.dumps(result, indent=2, sort_keys=True)
    print(report)
    if args.json_out:
        args.json_out.write_text(report + "\n", encoding="utf-8")

if __name__ == "__main__":
    main()
