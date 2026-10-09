#!/usr/bin/env python3
"""Apply the central Gen III GameCube legality routing to Legality.cpp.

This migration is intentionally idempotent and exact-marker guarded. It exists so
GitHub's legality maintenance workflow can update the large central analyzer without
reconstructing the entire translation unit through the Contents API. Once applied,
subsequent runs are a no-op.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "src" / "Legality" / "Legality.cpp"

INCLUDE_ANCHOR = '#include "Legality/Gen3CxdPidIvCorrelation.h"\n'
INCLUDES = '''#include "Legality/Gen3CxdPidIvCorrelation.h"\n#include "Legality/Gen3ColoEReaderShadowEvidence.h"\n#include "Legality/Gen3GameCubeFixedEncounterEvidence.h"\n#include "Legality/Gen3GameCubeShadowEvidence.h"\n#include "Legality/Gen3XdPokeSpotEvidence.h"\n'''

START = "            const auto cxd = Gen3CxdPidIv::analyze(pk.pid(), ivs);\n"
END = "            const auto channel = Gen3ChannelPidIv::analyze(\n"

REPLACEMENT = r'''            // GameCube-origin PK3 records use source-family-specific rules. PK3 stores
            // Colosseum and XD with the same origin value (15), so persistent encounter
            // identity must choose the family before selecting RNG semantics.
            if (pk.originGame() == 15 && pk.otGender() != 0) {
                add(r, Severity::Invalid,
                    "Pokemon Colosseum/XD-origin PK3 cannot have a female OT gender",
                    CheckIdentifier::Trainer);
            }

            const bool gen3Shiny = pk.isShiny(pk.id32(), {});

            // Japanese Colosseum e-Reader shadows are a special fixed-zero-IV path
            // and do not use the normal CXD PID/IV correlation.
            const auto eReaderShadow = Gen3ColoEReaderShadowEvidence::analyze({
                species, pk.originGame(), pk.language(), pk.otGender(),
                pk.metLevel(), pk.metLocation(), pk.isEgg(),
                pk.isFatefulEncounter(), pk.pid(), ivs
            });
            if (eReaderShadow.identityMatched) {
                if (eReaderShadow.matched) {
                    add(r, Severity::Info,
                        "PID and fixed-zero-IV history match the Japanese Pokemon Colosseum e-Reader shadow generation path",
                        CheckIdentifier::PidRng);
                    add(r, Severity::Info,
                        "Japanese Pokemon Colosseum e-Reader shadow identity and recursive prior-team history match pinned source data",
                        CheckIdentifier::Encounter);
                } else if (eReaderShadow.searchLimited) {
                    add(r, Severity::Info,
                        "Pinned Japanese Colosseum e-Reader shadow identity matches, but bounded recursive team-history search reached its safety limit; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        "Pinned Japanese Colosseum e-Reader shadow identity matches, but its PID/team-history provenance was not proven; legality remains incomplete",
                        CheckIdentifier::Encounter);
                }
                return;
            }

            const auto gameCubeShadow = Gen3GameCubeShadowEvidence::analyze({
                species, pk.originGame(), pk.metLevel(), pk.metLocation(),
                pk.ball(), pk.isEgg(), pk.isFatefulEncounter(), gen3Shiny,
                pk.tid16(), pk.sid16(), pk.pid(), ivs
            });
            if (gameCubeShadow.identityMatched) {
                using Family = Gen3GameCubeShadowEvidence::Family;
                using History = Gen3GameCubeShadowEvidence::HistoryResult;
                const bool isXd = gameCubeShadow.family == Family::XD;

                if (gameCubeShadow.correlation.matched) {
                    std::string rngDetail = isXd
                        ? "PID/IV spread matches the Pokemon XD "
                        : "PID/IV spread matches the standard Pokemon Colosseum ";
                    if (isXd && gameCubeShadow.correlation.antiShiny())
                        rngDetail += "CXDAnti target-PID reroll class required by the pinned shadow identity";
                    else
                        rngDetail += "XDRNG class required by the pinned shadow identity";
                    add(r, Severity::Info, std::move(rngDetail), CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        isXd
                            ? "Pinned Pokemon XD shadow identity matches, but neither standard CXD nor trainer-aware CXDAnti PID/IV provenance was proven"
                            : "Pinned Pokemon Colosseum shadow identity matches, but the required standard CXD PID/IV provenance was not proven",
                        CheckIdentifier::PidRng);
                    return;
                }

                if (gameCubeShadow.history == History::Matched) {
                    if (isXd) {
                        add(r, Severity::Info,
                            gameCubeShadow.xdRebattleLocation
                                ? "Pokemon XD shadow identity and recursive team/anti-shiny history match pinned source data through a source-supported Miror B. rebattle location"
                                : "Pokemon XD shadow identity and recursive team/anti-shiny history match pinned source data",
                            CheckIdentifier::Encounter);
                    } else {
                        std::string detail =
                            "Pokemon Colosseum shadow identity and recursive prior-team history match pinned source data";
                        if (gameCubeShadow.identityCandidateCount > 1)
                            detail += " through one of " +
                                      std::to_string(gameCubeShadow.identityCandidateCount) +
                                      " source-compatible encounter histories";
                        add(r, Severity::Info, std::move(detail), CheckIdentifier::Encounter);
                    }
                } else if (gameCubeShadow.history == History::SearchLimit) {
                    add(r, Severity::Info,
                        isXd
                            ? "Pinned Pokemon XD shadow identity and PID/IV evidence match, but bounded recursive team/anti-shiny history search reached its safety limit; provenance remains incomplete"
                            : "Pinned Pokemon Colosseum shadow identity and PID/IV evidence match, but bounded recursive prior-team search reached its safety limit; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        isXd
                            ? "Pinned Pokemon XD shadow identity and PID/IV evidence match, but no recursive team/anti-shiny history was proven; provenance remains incomplete"
                            : "Pinned Pokemon Colosseum shadow identity and PID/IV evidence match, but no recursive prior-team history was proven; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                }
                return;
            }

            // Direct GameCube starters, gifts and trades have template-specific
            // trainer/language/fateful rules and, for starters, a different XDRNG
            // call sequence from ordinary CXD encounters.
            const std::u16string gameCubeOtName = pk.otName();
            const std::u16string gameCubeNickname = pk.nickname();
            const auto fixedGameCube = Gen3GameCubeFixedEncounterEvidence::analyze({
                species, pk.originGame(), pk.language(), pk.otGender(), pk.gender(),
                pk.tid16(), pk.sid16(), pk.metLevel(),
                static_cast<uint8_t>(pk.metLocation()), pk.ball(), pk.isEgg(),
                pk.isFatefulEncounter(), gen3Shiny, pk.pid(), ivs,
                gameCubeOtName, gameCubeNickname
            });
            if (fixedGameCube.identityMatched) {
                if (fixedGameCube.rngMatched) {
                    add(r, Severity::Info,
                        "PID/IV and trainer evidence match the source-specific GameCube starter/gift/trade RNG policy",
                        CheckIdentifier::PidRng);
                    add(r, Severity::Info,
                        "Persistent fields match a pinned direct Pokemon Colosseum/XD starter, gift or in-game trade template",
                        CheckIdentifier::Encounter);
                } else if (fixedGameCube.searchLimited) {
                    add(r, Severity::Info,
                        "Pinned direct GameCube starter/gift/trade identity matches, but bounded RNG reconstruction reached its safety limit; provenance remains incomplete",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        "Pinned direct GameCube starter/gift/trade identity matches, but its required RNG correlation was not proven; provenance remains incomplete",
                        CheckIdentifier::PidRng);
                }
                return;
            }

            // XD Poke Spot encounters generate PID activation and IV/level animation
            // histories separately, so they must not be forced through ordinary CXD.
            const auto pokeSpot = Gen3XdPokeSpotEvidence::analyze({
                species, pk.originGame(), pk.language(), pk.otGender(),
                pk.metLevel(), static_cast<uint8_t>(pk.metLocation()),
                pk.isEgg(), pk.isFatefulEncounter(), true, pk.pid(), ivs
            });
            if (pokeSpot.identityMatched) {
                if (pokeSpot.matched()) {
                    add(r, Severity::Info,
                        "PID activation and IV/level animation histories match the Pokemon XD Poke Spot RNG path",
                        CheckIdentifier::PidRng);
                    add(r, Severity::Info,
                        "Species/location/level match a pinned Pokemon XD Poke Spot slot",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        "Pinned Pokemon XD Poke Spot identity matches, but the separate PID activation and IV/level histories were not both proven; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                }
                return;
            }

            // Generic standard CXD correlation remains useful when no exact GameCube
            // encounter family above can be established. CXDAnti is intentionally not
            // attempted generically because that reroll policy is XD-family-specific.
            const auto cxd = Gen3CxdPidIv::analyze(pk.pid(), ivs);
            if (cxd.matched) {
                add(r, Severity::Info,
                    "PID/IV spread matches the standard Pokemon Colosseum/XD XDRNG class",
                    CheckIdentifier::PidRng);
                add(r, Severity::Info,
                    "Standard GameCube RNG evidence is present, but no pinned direct shadow/fixed/Poke Spot identity was proven; exact encounter provenance remains incomplete",
                    CheckIdentifier::Encounter);
                return;
            }

'''


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")

    if '#include "Legality/Gen3GameCubeShadowEvidence.h"' not in text:
        if INCLUDE_ANCHOR not in text:
            raise SystemExit("missing Gen3CxdPidIvCorrelation include anchor")
        text = text.replace(INCLUDE_ANCHOR, INCLUDES, 1)

    if "const auto gameCubeShadow = Gen3GameCubeShadowEvidence::analyze" not in text:
        start = text.find(START)
        end = text.find(END, start + 1)
        if start < 0 or end < 0:
            raise SystemExit("central CXD routing anchors do not match expected source")
        text = text[:start] + REPLACEMENT + text[end:]

    TARGET.write_text(text, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
