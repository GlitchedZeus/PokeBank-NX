#!/usr/bin/env python3
"""Check that Gen V-IX source boundaries reach the ACTUAL legality analyzer.

This is a fail-closed source wiring contract, NOT execution of the host
analyzer, original retail witnesses, full PK5/PK6 decryption, PKHeX parity
or an approval to edit source saves. The C++ foundation suite is separate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

FILES = (
    "src/Legality/Legality.cpp",
    "include/Legality/Gen59FormatDomainEvidence.h",
    "include/Legality/Gen56NativeEnvelopeEvidence.h",
    "tests/test_legality_gen59_native_foundation.cpp",
)


def ensure(text: str, label: str, tokens: tuple[str, ...]) -> None:
    for token in tokens:
        if token not in text:
            raise ValueError(f"{label}: mandatory production guard missing: {token}")


def inspect(root: Path) -> dict:
    paths = {name: root / name for name in FILES}
    data = {name: path.read_text(encoding="utf-8")
            for name, path in paths.items()}
    source = data[FILES[0]]
    domain = data[FILES[1]]
    envelope = data[FILES[2]]
    test = data[FILES[3]]
    ensure(source, "production analyzer", (
        '#include "Legality/Gen59FormatDomainEvidence.h"',
        "Gen59FormatDomain::generationFromGroup(pk.getGameGroup())",
        "Gen59FormatDomain::classify(pk.getGameGroup())",
        "Gen59FormatDomain::maxSpecies(pk.getGameGroup())",
        "Gen59FormatDomain::maxMove(pk.getGameGroup())",
        "Gen59FormatDomain::mintedStatNature(pk.getGameGroup())",
        "sourceFormatMismatch",
        "r.coverage.structure=CoverageLevel::Partial",
        "r.coverage.encounter=CoverageLevel::None",
        "r.coverage.eventGift=CoverageLevel::None",
        "r.coverage.eggBreeding=CoverageLevel::None",
        "r.coverage.transfer=CoverageLevel::None",
    ))
    ensure(domain, "native storage domain", (
        "Kind::LetsGo", "Kind::SwordShield",
        "Kind::BrilliantDiamondShiningPearl", "Kind::LegendsArceus",
        "Kind::ScarletViolet", "Kind::LegendsZA",
        "return {Kind::LegendsZA,9,0,0,true,true}",
        "return {}; // Gen V/VI and other PK7 formats are NOT implemented.",
        "constexpr bool impossibleMove",
    ))
    ensure(envelope, "PK5/PK6 transport", (
        "PK5StoredBytes=136u", "PK5PartyBytes=220u",
        "PK6StoredBytes=232u", "PK6PartyBytes=260u",
        "UnrecognizedLength", "FormatMismatch", "HeaderOnly",
        "decryptedFieldsAvailable=false",
        "sourceHistoryAuthenticated=false",
    ))
    ensure(test, "native boundary regression", (
        "Gen V-IX native foundation:",
        "E::inspect(bytes,opposite)",
        "assert(raw==immutable)",
        "D::impossibleSpecies",
        "G::GameVersion::ZA",
    ))
    if "sourceProfile->maxSpecies\n            : nativeFormatGen<=2" in source:
        raise ValueError("stale pre-Gen-V source ceiling bypass remains")
    # Explicit historical assertions cannot be inferred from merely identifying
    # a native modern entity or an unencrypted format envelope.
    modern = source.split("// Gen VII-IX have existing native entity classes", 1)
    if len(modern) != 2:
        raise ValueError("modern INFO+Unknown coverage anchor missing")
    modern = modern[1].split("// F5: Gen II native PK2", 1)
    if len(modern) != 2:
        raise ValueError("modern coverage boundary missing")
    if "Severity::Invalid" in modern[0] or "Severity::Warning" in modern[0]:
        raise ValueError("unverified Gen VII-IX provenance must not emit Invalid/Warning")
    return {
        "schema": "POKEBANK_LEGALITY_GEN59_PRODUCTION_WIRING_V1",
        "status": "STATIC_WIRING_VERIFIED",
        "files": {name: hashlib.sha256(body.encode()).hexdigest()
                  for name, body in data.items()},
        "native_pk5_pk6_decoder_executed": False,
        "whole_analyzer_executed": False,
        "retail_history_authenticated": False,
        "pkhex_parity_certified": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    try:
        report = inspect(args.source_root)
    except (OSError, ValueError) as exc:
        print(json.dumps({"status": "BLOCKED", "error": str(exc)}, indent=2))
        return 1
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
