#!/usr/bin/env python3
"""Fail-closed, source-only integrity audit for the frozen L4 247 WC4 rows.

Verifies exact structural parity against the generated C++ projection. It does
NOT authenticate or license any retail card, original move/ribbon/text payload,
release window, or event-distribution receipt. Those remain separate L5 proofs.
"""
from __future__ import annotations

import argparse
import collections
import csv
import json
import re
from pathlib import Path

FIELDS = ("species", "tid", "sid", "pid", "met_level", "met_location", "ball",
          "form", "gender", "language", "version", "ot_gender", "fateful", "card_id")
REQUIRED = ("projection_index", *FIELDS, "same_card_id_rows", "archive_grade",
            "initial_moves_origin", "ribbon_origin", "raw_text_origin",
            "release_attested", "record_digest_available")
SOURCE_FIELDS = ("archive_grade", "initial_moves_origin", "ribbon_origin",
                 "raw_text_origin", "release_attested", "record_digest_available")
FROZEN_UNKNOWN = ("UNVERIFIED", "UNAVAILABLE", "UNAVAILABLE", "UNAVAILABLE", "NO", "NO")
PATTERN = re.compile(r"^\s*\{\s*([^{}]+?)\s*\},?\s*(?://.*)?$", re.MULTILINE)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def number(value: str, context: str) -> int:
    value = value.strip().removesuffix("u").removesuffix("U")
    require(bool(re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|[0-9]+)", value)),
            f"{context}: invalid integer {value!r}")
    return int(value, 0) if value.lower().startswith("0x") else int(value)


def scalar(value: str, key: str, context: str) -> int | bool:
    if key == "fateful":
        require(value in ("true", "false"), f"{context}: invalid fateful {value!r}")
        return value == "true"
    return number(value, context)


def projection_rows(source: str) -> list[dict[str, int | bool]]:
    rows = []
    for line in source.splitlines():
        if not line.lstrip().startswith("{"):
            continue
        match = PATTERN.fullmatch(line)
        require(match is not None, f"malformed C++ event projection row: {line[:120]!r}")
        cells = [x.strip() for x in match.group(1).split(",")]
        require(len(cells) == len(FIELDS), f"template #{len(rows)}: expected {len(FIELDS)} fields, got {len(cells)}")
        rows.append({key: scalar(value, key, f"template #{len(rows)} {key}")
                     for key, value in zip(FIELDS, cells)})
    return rows


def audit(csv_text: str, inc_text: str, *, expected_count: int = 247,
          expected_distinct: int | None = 70,
          expected_reused: int | None = 219) -> dict[str, int | bool]:
    rows = list(csv.DictReader(csv_text.splitlines()))
    templates = projection_rows(inc_text)
    require(len(rows) == expected_count, f"audit CSV has {len(rows)} rows; expected {expected_count}")
    require(len(templates) == expected_count,
            f"C++ template contains {len(templates)} rows; expected {expected_count}")
    require(rows and set(rows[0]) == set(REQUIRED), "audit CSV header drifted from frozen schema")
    cards = collections.Counter(int(item["card_id"]) for item in templates)
    require(expected_distinct is None or len(cards) == expected_distinct,
            f"distinct card IDs changed to {len(cards)}")
    reused = sum(cards[int(item["card_id"])] > 1 for item in templates)
    require(expected_reused is None or reused == expected_reused,
            f"rows reusing card IDs changed to {reused}")
    for idx, (row, template) in enumerate(zip(rows, templates)):
        ctx = f"projection_index={idx}"
        require(number(row["projection_index"], ctx) == idx,
                f"{ctx}: duplicate/missing/misordered projection index")
        for key in FIELDS:
            actual = scalar(row[key], key, ctx)
            require(actual == template[key],
                    f"{ctx}: {key} changed from C++ projection ({actual!r} != {template[key]!r})")
        require(number(row["same_card_id_rows"], ctx) == cards[int(row["card_id"])],
                f"{ctx}: incorrect same_card_id_rows, never key archival identity by card ID")
        source_flags = tuple(row[key] for key in SOURCE_FIELDS)
        require(source_flags == FROZEN_UNKNOWN,
                f"{ctx}: original-archive/source fields promoted without reviewed L5 evidence; "
                "do not turn structural projections into authentic event proof")
    return {"projection_rows": len(templates), "distinct_card_ids": len(cards),
            "reused_card_id_rows": reused, "original_archives_authenticated": 0,
            "source_projection_only": True}


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit-csv", type=Path,
                        default=root / "docs/legality/l4/GEN4_WC4_TEMPLATE_AUDIT.csv")
    parser.add_argument("--cpp-projection", type=Path,
                        default=root / "include/Legality/Gen4EventTemplateData.inc")
    args = parser.parse_args()
    result = audit(args.audit_csv.read_text(encoding="utf-8"),
                   args.cpp_projection.read_text(encoding="utf-8"))
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
