#include "Legality/Gen3SafariOriginEvidence.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

using namespace Legality;

namespace {

std::string_view gameId(Gen3Safari::Game game) {
    switch (game) {
        case Gen3Safari::Game::Ruby: return "ruby_gba";
        case Gen3Safari::Game::Sapphire: return "sapphire_gba";
        case Gen3Safari::Game::Emerald: return "emerald_gba";
        case Gen3Safari::Game::FireRed: return "firered_gba";
        case Gen3Safari::Game::LeafGreen: return "leafgreen_gba";
        case Gen3Safari::Game::Invalid: return {};
    }
    return {};
}

const Gen3Safari::Entry* firstRow(Gen3Safari::Game game, uint16_t species) {
    for (const auto& row : Gen3Safari::kGen3SafariEntries) {
        if (row.game == static_cast<uint8_t>(game) && row.species == species)
            return &row;
    }
    return nullptr;
}

} // namespace

int main() {
    using namespace Gen3SafariOrigin;

    // Every direct pinned Safari row resolves to the required Safari Ball.
    for (const auto& row : Gen3Safari::kGen3SafariEntries) {
        const auto evidence = wildHistoryEvidence(
            gameId(static_cast<Gen3Safari::Game>(row.game)),
            row.species, row.location, row.minLevel, row.form);
        assert(evidence.resolution == Resolution::RequiredBallKnown);
        assert(evidence.requiredBall == Gen3Safari::kSafariBall);
        assert(evidence.candidateRows >= 1);
    }

    // Ruby Safari Pikachu can evolve into Raichu while retaining the original
    // Safari met location/level/ball provenance.
    const auto* pikachu = firstRow(Gen3Safari::Game::Ruby, 25);
    assert(pikachu != nullptr);
    const auto raichu = wildHistoryEvidence(
        "ruby_gba", 26, pikachu->location, pikachu->minLevel, 0);
    assert(raichu.resolution == Resolution::RequiredBallKnown);
    assert(raichu.requiredBall == Gen3Safari::kSafariBall);
    assert(raichu.evolved);
    assert(raichu.sourceSpecies == 25);

    // FireRed Safari Magikarp -> Gyarados.
    const auto* magikarp = firstRow(Gen3Safari::Game::FireRed, 129);
    assert(magikarp != nullptr);
    const auto gyarados = wildHistoryEvidence(
        "firered_gba", 130, magikarp->location, magikarp->minLevel, 0);
    assert(gyarados.resolution == Resolution::RequiredBallKnown);
    assert(gyarados.evolved);
    assert(gyarados.sourceSpecies == 129);

    // FireRed Safari Scyther -> Scizor crosses a generation-II evolution while
    // remaining a valid Generation III evolved history.
    const auto* scyther = firstRow(Gen3Safari::Game::FireRed, 123);
    assert(scyther != nullptr);
    const auto scizor = wildHistoryEvidence(
        "firered_gba", 212, scyther->location, scyther->minLevel, 0);
    assert(scizor.resolution == Resolution::RequiredBallKnown);
    assert(scizor.evolved);
    assert(scizor.sourceSpecies == 123);

    // Count the accepted current species that are not direct Safari species but
    // have a Safari-catchable ancestor somewhere in the pinned five-game data.
    // This locks the current pre-evolution/source-table relationship.
    std::array<bool, 387> direct{};
    for (const auto& row : Gen3Safari::kGen3SafariEntries)
        if (row.species <= 386)
            direct[row.species] = true;

    std::array<bool, 387> descendant{};
    for (uint16_t species = 1; species <= 386; ++species) {
        if (direct[species])
            continue;
        uint16_t ancestor = Gen34EggMove::preEvolution("ruby_gba", species);
        for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
            if (ancestor <= 386 && direct[ancestor]) {
                descendant[species] = true;
                break;
            }
            const uint16_t next =
                Gen34EggMove::preEvolution("ruby_gba", ancestor);
            if (next == ancestor)
                break;
            ancestor = next;
        }
    }
    std::size_t descendantCount = 0;
    for (bool value : descendant)
        if (value)
            ++descendantCount;
    assert(descendantCount == 30);

    // A wild-family match is not automatically a global verdict. The caller
    // must explicitly declare encounter-family coverage complete.
    CompetingHistory incomplete{};
    auto gated = applyCompetingHistory(raichu, incomplete);
    assert(gated.resolution == Resolution::Unresolved);
    assert(gated.requiredBall == 0);

    CompetingHistory complete{};
    complete.supportComplete = true;
    gated = applyCompetingHistory(raichu, complete);
    assert(gated.resolution == Resolution::RequiredBallKnown);
    assert(gated.requiredBall == Gen3Safari::kSafariBall);

    for (int source = 0; source < 6; ++source) {
        CompetingHistory competing{};
        competing.supportComplete = true;
        switch (source) {
            case 0: competing.staticOrGift = true; break;
            case 1: competing.event = true; break;
            case 2: competing.trade = true; break;
            case 3: competing.egg = true; break;
            case 4: competing.gameCube = true; break;
            case 5: competing.other = true; break;
        }
        const auto result = applyCompetingHistory(raichu, competing);
        assert(result.resolution == Resolution::Unresolved);
        assert(result.requiredBall == 0);
    }

    // Unsupported identities and mismatched persistent fields remain unresolved.
    assert(wildHistoryEvidence("unknown", 26, 57, 25, 0).resolution ==
           Resolution::Unresolved);
    assert(wildHistoryEvidence("ruby_gba", 0, 57, 25, 0).resolution ==
           Resolution::Unresolved);
    assert(wildHistoryEvidence("ruby_gba", 26, 136, 25, 0).resolution ==
           Resolution::Unresolved);
    assert(wildHistoryEvidence("firered_gba", 130, 57, 20, 0).resolution ==
           Resolution::Unresolved);
    assert(wildHistoryEvidence("ruby_gba", 387, 57, 25, 0).resolution ==
           Resolution::Unresolved);

    return 0;
}
