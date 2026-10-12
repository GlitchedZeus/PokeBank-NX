// L5 independent read-only native C++ encounter condition regression.
// No original-save mutation; candidate mismatch != global legality Invalid.
#include "Legality/Gen1EncounterEvidence.h"
#include "Legality/Gen2WildEncounter.h"
#include "Legality/Gen3SafariOriginEvidence.h"
#include "Legality/Gen4WildEncounter.h"
#include <cassert>
#include <iostream>
#include <string_view>

int main() {
    using namespace Legality;
    // Gen I: tested original Red starter template; template mismatch is candidate-local.
    assert(Gen1Encounter::matchStatic("red_gb", 1, 5, 45).matched);
    assert(!Gen1Encounter::matchStatic("red_gb", 150, 50, 3).matched);
    assert(!Gen1Encounter::matchStatic("unsupported_game", 1, 5, 45).matched);

    // Gen II Crystal caught-data preserves time/location/level slot constraints.
    constexpr auto ok = static_cast<uint16_t>((1u << 14) | (20u << 8) | 1u);
    constexpr auto wrong = static_cast<uint16_t>((1u << 14) | (63u << 8) | 1u);
    assert(Gen2Wild::matchesCrystalCaughtData(72, ok));
    assert(!Gen2Wild::matchesCrystalCaughtData(72, wrong));
    assert(!Gen2Wild::hasSpecies("unsupported_game", 72));

    // Gen III: take a pinned generated Safari row, not a fabricated real save.
    const auto& first = Gen3Safari::kGen3SafariEntries[0];
    const auto game = static_cast<Gen3Safari::Game>(first.game);
    const std::string_view gameId =
        game == Gen3Safari::Game::Ruby ? "ruby_gba" :
        game == Gen3Safari::Game::Sapphire ? "sapphire_gba" :
        game == Gen3Safari::Game::Emerald ? "emerald_gba" :
        game == Gen3Safari::Game::FireRed ? "firered_gba" : "leafgreen_gba";
    const auto wild = Gen3Safari::directWildEvidence(
        gameId, first.species, first.location, first.minLevel, first.form);
    assert(wild.matched && wild.candidateRows >= 1);
    assert(!Gen3Safari::directWildEvidence(
        "unsupported_game", first.species, first.location, first.minLevel, first.form).matched);
    const auto historical = Gen3SafariOrigin::wildHistoryEvidence(
        gameId, first.species, first.location, first.minLevel, first.form);
    assert(historical.resolution == Gen3SafariOrigin::Resolution::RequiredBallKnown);
    const auto unresolved = Gen3SafariOrigin::applyCompetingHistory(historical, {});
    assert(unresolved.resolution == Gen3SafariOrigin::Resolution::Unresolved);

    // Gen IV: positive source candidate, candidate-local wrong level, unknown ID.
    assert(Gen4Wild::matches("platinum_nds", 396, 16, 2, 0));
    assert(!Gen4Wild::matches("platinum_nds", 396, 16, 50, 0));
    assert(!Gen4Wild::hasSpecies("unsupported_game", 396));

    // Condition specificity: Munchlax group-C trees depend on externally attested ID.
    assert(Gen4Wild::matches("diamond_nds", 446, 23, 5, 0));
    assert(!Gen4Wild::matchesWithTrainerId("diamond_nds", 446, 23, 5, 0, 0));
    std::cout << "L5 12 source-specific positive/candidate-negative/Unknown cases: PASS\n";
}
