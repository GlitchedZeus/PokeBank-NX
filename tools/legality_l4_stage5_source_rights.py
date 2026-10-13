#!/usr/bin/env python3
"""L4 Stage 5 coordinator gate for the independently fetched L5 Q08 research catalog.

This is a fail-closed REDISTRIBUTION audit, not legal advice. No third-party
retail payload is loaded, imported, included, or redistributed by this tool.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


BLOCKED = "blocked_pending_review"
PERMISSIONS = ("copy_source_code", "import_original_retail_tables",
               "redistribute_event_or_pokemon_payloads")
DISPOSITIONS = ("third_party_code_import", "bulk_retail_table_import",
                "distribution_ROM_or_event_payload")


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def verify_rights(text: str) -> dict:
    source = json.loads(text)
    require(source.get("schema") == "L5_Q08_REDISTRIBUTION_GATE_V1"
            and source.get("catalog_id") == "Q08", "wrong or missing Q08 schema")
    require(source.get("not_legal_opinion") is True,
            "research summary must not purport to be legal advice")
    require(source.get("project_license_observed") == "AGPL-3.0" and
            source.get("pkhex_reference_license_observed") == "GPL-3.0",
            "underlying license context changed: review required")
    entries = source.get("repo_rows")
    require(isinstance(entries, list) and len(entries) >= 1,
            "no independently recorded source permissions")
    names: set[str] = set()
    for i, row in enumerate(entries):
        name = row.get("source")
        require(isinstance(name, str) and name.strip() and name not in names,
                f"invalid or duplicate source row {i}")
        names.add(name)
        require(row.get("legal_review_complete") is False,
                f"{name}: legal review changed; a new approval process is needed")
        for permission in PERMISSIONS:
            require(row.get(permission) == BLOCKED,
                    f"{name}: {permission} no longer blocked")
    dispositions = source.get("dispositions")
    require(isinstance(dispositions, dict), "missing Q08 dispositions")
    for permission in DISPOSITIONS:
        require(dispositions.get(permission) == "BLOCKED",
                f"{permission}: must remain blocked until specific review")
    require(dispositions.get("third_party_research_reference") ==
            "allowed_as_unredistributed_reference", "research-only boundary changed")
    require(dispositions.get("native_original_save_from_user") ==
            "consent_and_data_protection_required", "user-source boundary changed")
    return {"research_rows": len(entries), "unlicensed_imports_permitted": 0,
            "retail_event_payloads_permitted": 0,
            "original_archive_authenticity_proved": False,
            "research_reference_only": True}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--l5-q08", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(verify_rights(args.l5_q08.read_text(encoding="utf-8")),
                     sort_keys=True))


if __name__ == "__main__":
    main()
