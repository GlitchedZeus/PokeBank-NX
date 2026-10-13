#!/usr/bin/env python3
"""Fail-closed coordinator contract: legality research must reach PRODUCTION.

A source header's existence or standalone green lane tests do not establish
that Legality::analyze executes the correct source-evidence rule. This checks
the exact production integration connections and their native-analyzer CI
probes on the current checkout. It DOES NOT substitute for a compiler, real
retail original specimens or a PKHeX whole-Pokemon parity certificate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

PRODUCTION = "src/Legality/Legality.cpp"
WORKFLOW = ".github/workflows/host-tests.yml"
PK2_PROBE = "tests/fixtures/legality_l5_pk2_egg_stat_exp_analyzer_probe.cpp"
PK4_PROBE = "tests/fixtures/legality_l5_platinum_native_analyzer_probe.cpp"

# Source-identifying comments delimit reviewed code paths. Avoid checking
# mere includes: each bridge must execute in the actual analyzer.
F5_START = "// F5: Gen II native PK2 stores five 16-bit stat-experience fields,"
F5_END = "// MOVE-003: a Shedinja is copied from the freshly evolved"
GBA_START = "// L3 source-exact Ruby/Sapphire and FireRed/LeafGreen original"
GBA_END = "// Source-pinned Gen III level-up evidence can also describe a"
PARK_START = "// Source-pinned Gen III level-up evidence can also describe a"
PARK_END = "// L3 exact ORIGINAL Platinum direct-method candidates:"
PT_START = "// L3 exact ORIGINAL Platinum direct-method candidates:"
PT_END = "// META-001/EVENT-001: raw native PK4 text is an independent"


def segment(text: str, begin: str, end: str) -> str:
    if text.count(begin) != 1 or text.count(end) != 1:
        raise ValueError("missing or duplicated production anchor: " + begin)
    left = text.index(begin)
    right = text.index(end)
    if left >= right:
        raise ValueError("wrong production evidence order: " + begin)
    return text[left:right]


def require_all(label: str, text: str, needles: tuple[str, ...]) -> None:
    for needle in needles:
        if needle not in text:
            raise ValueError(f"{label}: mandatory execution guard missing: {needle}")


def audit(root: Path) -> dict:
    paths = (PRODUCTION, WORKFLOW, PK2_PROBE, PK4_PROBE)
    contents = {name: (root / name).read_text(encoding="utf-8")
                for name in paths}
    production = contents[PRODUCTION]
    workflow = contents[WORKFLOW]
    egg = contents[PK2_PROBE]
    native = contents[PK4_PROBE]

    # The native read-only type is established through an explicit virtual
    # pointer capability, never a broad saved-game enum or C-style cast.
    for forbidden in (
        "static_cast<const Pokemon::Pokemon1ReadOnly&>(pk)",
        "static_cast<const Pokemon::Pokemon2ReadOnly&>(pk)",
        "reinterpret_cast<const Pokemon::Pokemon1ReadOnly&>(pk)",
        "reinterpret_cast<const Pokemon::Pokemon2ReadOnly&>(pk)",
    ):
        if forbidden in production:
            raise ValueError("unsafe typed adapter downcast reintroduced: " + forbidden)
    require_all("typed native adapters", production, (
        "pk.nativePk1ReadOnly() != nullptr",
        "pk.nativePk2ReadOnly() != nullptr",
        "*pk.nativePk1ReadOnly()",
        "*pk.nativePk2ReadOnly()",
    ))

    f5 = segment(production, F5_START, F5_END)
    require_all("PK2 egg production", f5, (
        "nativeFormatGeneration == 2", "pk.isEgg()",
        "pk.gen2NativeStatExperience(nativeStatExperience)",
        "std::array<uint16_t, 5> nativeStatExperience",
        "if (nonzero)", "Severity::Invalid", "CheckIdentifier::Egg"))
    if "static_cast<const Pokemon::Pokemon2ReadOnly&>" in f5:
        raise ValueError("unsafe Gen II polymorphic downcast reintroduced")
    if f5.count("Severity::Invalid") != 1:
        raise ValueError("F5 native structural Invalid site count drift")

    checks = (
        ("PK3 original GBA", GBA_START, GBA_END,
         ("Gen34FormatDomain::generationFromGroup",
          "Gen4Origin::exactGen3GameId(pk.originGame())",
          "Gen3GbaVersionedLevelUpTimeline::offerCount",
          "Gen3GbaVersionedLevelUpTimeline::offerAt",
          "Game::Ruby", "Game::Sapphire", "Game::FireRed", "Game::LeafGreen",
          "Severity::Info", "CheckIdentifier::Moves")),
        ("PK4 Pal Park", PARK_START, PARK_END,
         ("Gen4Origin::Kind::Gen3Handheld",
          "Gen34PalParkMoves::verifiedTransferFields",
          "Gen4Origin::exactGen3GameId(pk.originGame())",
          "Gen3GbaVersionedLevelUpTimeline::offerAt",
          "move>354", "Severity::Info", "CheckIdentifier::Moves")),
        ("PK4 Platinum", PT_START, PT_END,
         ("Gen4Origin::exactRetailGameId(pk.originGame())",
          '"platinum_nds"', "Gen4PlatinumNativeMethodData::row",
          "Gen4PlatinumNativeMethodData::packedOffer",
          "Gen4PlatinumNativeMethodData::tutorCompatible",
          "Gen4PlatinumPinnedMachines::compatible",
          "Severity::Info", "CheckIdentifier::Moves")),
    )
    audited = []
    for label, start, end, required in checks:
        block = segment(production, start, end)
        require_all(label, block, required)
        for forbidden in ("Severity::Invalid", "Severity::Warning",
                          "setMove(", "setOriginGame(", "refreshChecksum("):
            if forbidden in block:
                raise ValueError(label + ": candidate-only bridge may not "
                                 "mutate or upgrade verdict: " + forbidden)
        audited.append(label)

    require_all("native PK2 analyzer cases", egg,
                ("Legality::analyze(", "1000 && eggNonzero == 400",
                 "pk.statExpHP()", "pk.statExpSpecial()",
                 "pk.strictRecord().rawBody == originalBody"))
    require_all("encrypted PK4 analyzer cases", native,
                ("Encryption::encryptArray4", "Legality::analyze(",
                 "ciphertext==original",
                 "Original Platinum native level-up method:",
                 "Original Platinum native tutor method:",
                 "Original Platinum native TM/HM method:",
                 "Pal Park original ruby_gba level-up source opportunity:",
                 "Pal Park original firered_gba level-up source opportunity:",
                 "PK4 19-context INFO gate PASS"))

    # These must be actually run on the PR application head; a path in the
    # workflow without a both-profile target does not close validation.
    require_all("exact integration CI", workflow, (
        "github.event.pull_request.head.sha",
        "l5-pk2-egg-fixed\n", "l5-pk2-egg-fixed-sanitize",
        "l5-platinum-native\n", "l5-platinum-native-sanitize",
        "l5-pk12-fixed-sanitize",
        "--expected-invalid 38", "stage4_crosslane.py --l1-root ."))
    return {
        "schema": "POKEBANK_LEGALITY_PRODUCTION_CONNECTION_CONTRACT_V1",
        "contracts": ["native Gen II egg F5"] + audited,
        "source_sha256": {
            name: hashlib.sha256(body.encode("utf-8")).hexdigest()
            for name, body in contents.items()},
        "test_status": "STATIC_WIRING_VERIFIED_ONLY",
        "native_binary_executed": False,
        "pkhex_whole_pokemon_parity_certified": False,
    }


def self_test() -> None:
    assert segment("a--start--b--end--c", "--start--", "--end--") == "--start--b"
    for invalid in ("--start--b", "--end--b", "--start--x--start--y--end--"):
        try:
            segment(invalid, "--start--", "--end--")
        except ValueError:
            pass
        else:
            raise AssertionError("missing/duplicate anchors must fail")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    try:
        result = audit(args.source_root)
    except (OSError, ValueError) as exc:
        print(json.dumps({"status": "BLOCKED", "error": str(exc)}, indent=2))
        return 1
    result["status"] = "PASS_STATIC_CONTRACT"
    print(json.dumps(result, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
