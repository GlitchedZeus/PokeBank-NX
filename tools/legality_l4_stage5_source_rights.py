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
# The exact L5 Q08 baseline records 14 researched sources. Dropping one
# row would previously pass while silently omitting its blocked permissions.
# Changes to this frozen registry require a new source-specific audit.
EXPECTED_SOURCES = frozenset({
    "PKHeX", "pokered", "pokecrystal", "pokefirered", "pokeyellow",
    "pokeplatinum", "pokeheartgold", "pokeruby", "pokegold",
    "EventsGallery", "pokeemerald", "PokeBank-NX", "pokediamond",
    "PokeAPI-provenance-existing"})
ROW_FIELDS = frozenset({
    "source", "repository", "revision", "source_license_claim",
    "proposed_usage", "copy_source_code", "import_original_retail_tables",
    "redistribute_event_or_pokemon_payloads", "legal_review_complete",
    "legal_basis_or_grant"})
# The exact L5 Q08 baseline (Git blob 7cd3d0a227dc9b008f7c304581a2d7e8d85f06e9,
# reviewed in docs/legality/l4/STAGE6_SOURCE_RIGHTS_Q07_Q08_HANDOFF_20261012.md)
# records two sources with a narrower, source-specific usage than the generic
# citation-and-digest usage. Each source must match its own frozen value
# exactly; any other value still requires a new source-specific review.
DEFAULT_USAGE = "citation_and_digest_only"
SOURCE_SPECIFIC_USAGE = {
    "pokediamond": "symbol references and SHA256 only",
    "PokeAPI-provenance-existing": "existing only; no new binary/data import",
}


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
    require(len(entries)==len(EXPECTED_SOURCES),
            "Q08 frozen source count changed; re-review required")
    for i, row in enumerate(entries):
        require(isinstance(row, dict) and set(row)==ROW_FIELDS,
                f"Q08 source row {i} schema/payload injection")
        name = row.get("source")
        require(isinstance(name, str) and name.strip() and name not in names,
                f"invalid or duplicate source row {i}")
        names.add(name)
        require(row["proposed_usage"] == SOURCE_SPECIFIC_USAGE.get(name, DEFAULT_USAGE) and
                str(row["legal_basis_or_grant"]).startswith("not documented"),
                f"{name}: legal basis/usage changed; independent review required")
        require(row.get("legal_review_complete") is False,
                f"{name}: legal review changed; a new approval process is needed")
        for permission in PERMISSIONS:
            require(row.get(permission) == BLOCKED,
                    f"{name}: {permission} no longer blocked")
    require(names == EXPECTED_SOURCES,
            "Q08 frozen source identities changed; new rights review required")
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
