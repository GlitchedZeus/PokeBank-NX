#!/usr/bin/env python3
"""Independently reconcile L4 WC4 structural audit with L5's Q07 source join.

Source artifact is fetched separately from the advancing L5 draft PR.
This verifies frozen indexed projections, NOT original event authenticity.
"""
from __future__ import annotations

import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def verify(l4_csv: str, l5_json: str, cpp_projection: bytes,
           expected_count: int = 247, expected_distinct: int = 70,
           expected_reused: int = 219) -> dict:
    l4 = list(csv.DictReader(l4_csv.splitlines()))
    l5 = json.loads(l5_json)
    rows = l5["rows"]
    require(len(l4) == len(rows) == expected_count,
            "L4/L5 projection coverage differs")
    rev = l5["source"]["pkhex_revision"]
    require(rev == "6501f0ab46e8f8ca048539dbaf8cae8cb104e722",
            "PKHeX resource revision changed without new source audit")
    require(l5["source"]["projection_records"] == expected_count,
            "L5 declared projection count differs")
    require(l5["source"]["blob_sha"] == git_blob_sha(cpp_projection),
            "L5 source C++ template Git blob differs from checked source")
    counts = collections.Counter(int(x["card_id"]) for x in l4)
    reused = sum(counts[int(x["card_id"])] > 1 for x in l4)
    duplicates = sum(x > 1 for x in counts.values())
    require(len(counts) == expected_distinct and reused == expected_reused,
            "L4 frozen count/collision topology changed")
    m = l5["metrics"]
    require((m["templates"], m["unique_card_ids"], m["repeated_card_id_templates"],
             m["duplicate_cards"]) ==
            (expected_count, expected_distinct, expected_reused, duplicates),
            "L5 index/collision metrics disagree with L4")
    require(m["original_card_receipts_verified"] == 0 and
            m["archive_item_sha256_verified"] == 0,
            "independently authenticated originals require a new reviewed audit policy")
    keys = set()
    for i, (a, b) in enumerate(zip(l4, rows)):
        where = f"projection #{i}"
        require(int(a["projection_index"]) == i and b["template_index"] == i,
                f"{where}: changed indexed identity")
        for source, dest in (("species", "species"), ("tid", "tid"),
                             ("sid", "sid"), ("card_id", "card_id"),
                             ("version", "native_game_code"),
                             ("language", "native_language_code")):
            require(int(a[source]) == b[dest], f"{where}: {source}/{dest} differs")
        identity = ":".join((f"wc4_{rev[:8]}", str(i), a["species"], a["tid"],
                             a["sid"], a["card_id"]))
        require(b["identity_key"] == identity and identity not in keys,
                f"{where}: composite index/species/TID/SID/card ID not unique or changed")
        keys.add(identity)
        require(b["archive_path"] is None and b["original_archive_sha256"] is None
                and b["archive_provenance"] == "unverified"
                and b["original_release_receipt"] == "unknown"
                and b["license_redistribution_clearance"] == "not established",
                f"{where}: L5 reported an original receipt/license without joint review")
        for field in ("original_payload_moves", "original_payload_ribbons",
                      "original_raw_trainer_bytes", "distribution_language_window"):
            require(b[field] == "unknown", f"{where}: unaudited original {field}")
        require(a["archive_grade"] == "UNVERIFIED" and
                a["record_digest_available"] == "NO" and
                a["release_attested"] == "NO",
                f"{where}: L4 asserted unaudited event authentication")
    return {"rows": len(rows), "matching_composite_keys": len(keys),
            "reused_card_id_rows": reused, "authenticated_original_cards": 0,
            "cross_lane_projection_agreement": True,
            "original_event_provenance_still_unknown": True}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[1]
    parser.add_argument("--l4-audit", type=Path,
                        default=root / "docs/legality/l4/GEN4_WC4_TEMPLATE_AUDIT.csv")
    parser.add_argument("--l5-q07", type=Path, required=True,
                        help="separately retrieved L5 Q07_WC4_EVENT_PROVENANCE_JOIN.json")
    parser.add_argument("--cpp-projection", type=Path,
                        default=root / "include/Legality/Gen4EventTemplateData.inc")
    args = parser.parse_args()
    print(json.dumps(verify(args.l4_audit.read_text(encoding="utf-8"),
                            args.l5_q07.read_text(encoding="utf-8"),
                            args.cpp_projection.read_bytes()), sort_keys=True))


if __name__ == "__main__":
    main()
