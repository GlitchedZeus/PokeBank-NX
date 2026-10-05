#include "Legality/Gen4WurmpleWildSourceEvidence.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>

namespace {

constexpr std::string_view gameId(Legality::Gen4Wild::Game game) noexcept {
    using Game = Legality::Gen4Wild::Game;
    switch (game) {
        case Game::Diamond:    return "diamond_nds";
        case Game::Pearl:      return "pearl_nds";
        case Game::Platinum:   return "platinum_nds";
        case Game::HeartGold:  return "heartgold_nds";
        case Game::SoulSilver: return "soulsilver_nds";
        case Game::Invalid:    return {};
    }
    return {};
}

bool directMatch(std::string_view game,
                 uint16_t species,
                 uint16_t location,
                 uint8_t level) {
    return Legality::Gen4Wild::matchesWithTrainerId(
        game, species, location, level, 0, 0);
}

} // namespace

int main() {
    using namespace Legality;
    using Status = Gen4WurmpleWildSource::Status;

    // Only evolved Wurmple-family records with a real Gen IV met level apply.
    assert(Gen4WurmpleWildSource::classify(
               "diamond_nds", 265, 1, 5, 0).status == Status::NotApplicable);
    assert(Gen4WurmpleWildSource::classify(
               "diamond_nds", 25, 1, 5, 0).status == Status::NotApplicable);
    assert(Gen4WurmpleWildSource::classify(
               "unknown", 266, 1, 5, 0).status == Status::NotApplicable);
    assert(Gen4WurmpleWildSource::classify(
               "diamond_nds", 266, 1, 0, 0).status == Status::NotApplicable);

    bool sawWurmpleRow = false;
    bool sawDirectCocoonRow = false;
    bool sawDirectFinalRow = false;
    bool sawWurmpleOnlyWildContext = false;

    // Walk every pinned Wurmple-family wild row and every level it permits.
    // The expected result is independently reconstructed from the raw generated
    // table's direct-species matchers. This catches future generator/data drift
    // without turning absence from the wild table into a global legality claim.
    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        const uint16_t rowSpecies = Gen4Wild::species(row);
        if (rowSpecies < 265 || rowSpecies > 269)
            continue;

        const auto game = gameId(Gen4Wild::game(row));
        assert(!game.empty());
        const uint16_t location = Gen4Wild::location(row);

        if (rowSpecies == 265)
            sawWurmpleRow = true;
        if (rowSpecies == 266 || rowSpecies == 268)
            sawDirectCocoonRow = true;
        if (rowSpecies == 267 || rowSpecies == 269)
            sawDirectFinalRow = true;

        for (uint16_t level = Gen4Wild::minLevel(row);
             level <= Gen4Wild::maxLevel(row); ++level) {
            for (uint16_t current = 266; current <= 269; ++current) {
                const bool wurmple = directMatch(
                    game, 265, location, static_cast<uint8_t>(level));
                const bool directCurrent = directMatch(
                    game, current, location, static_cast<uint8_t>(level));
                const uint16_t cocoon =
                    Gen4WurmpleWildSource::cocoonAncestor(current);
                const bool directCocoon = cocoon != 0 && directMatch(
                    game, cocoon, location, static_cast<uint8_t>(level));

                const auto result = Gen4WurmpleWildSource::classify(
                    game, current, location, static_cast<uint8_t>(level), 0);

                if (directCurrent || directCocoon) {
                    assert(result.status == Status::AlternateWildSource);
                    assert(result.wurmpleCandidate == wurmple);
                    const uint16_t expectedAlternate =
                        directCurrent ? current : cocoon;
                    assert(result.alternateSourceSpecies == expectedAlternate);
                    assert(!result.wurmpleOnlyWithinWildEvidence());
                } else if (wurmple) {
                    assert(result.status == Status::WurmpleOnlyWildSource);
                    assert(result.wurmpleCandidate);
                    assert(result.alternateSourceSpecies == 0);
                    assert(result.wurmpleOnlyWithinWildEvidence());
                    sawWurmpleOnlyWildContext = true;
                } else {
                    assert(result.status == Status::NoWildFamilySource);
                    assert(!result.wurmpleCandidate);
                    assert(result.alternateSourceSpecies == 0);
                    assert(!result.wurmpleOnlyWithinWildEvidence());
                }
            }
        }
    }

    // These guard the intended counterexample surface: the pinned wild corpus
    // must retain Wurmple plus naturally direct-caught cocoons. Direct final
    // stages vary by retail encounter data, so they are audited when present
    // rather than required as a corpus invariant.
    assert(sawWurmpleRow);
    assert(sawDirectCocoonRow);

    // Wurmple-only wild contexts are useful positive evidence when present, but
    // they remain bounded to wild slots. A future upstream corpus that removes
    // such a context must not turn this test into a false global-legality rule.
    (void)sawDirectFinalRow;
    (void)sawWurmpleOnlyWildContext;

    // Explicit direct-catch regression: every cocoon row must classify as an
    // alternate non-Wurmple wild source for that surviving cocoon species.
    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        const uint16_t species = Gen4Wild::species(row);
        if (species != 266 && species != 268)
            continue;
        const auto game = gameId(Gen4Wild::game(row));
        const auto result = Gen4WurmpleWildSource::classify(
            game, species, Gen4Wild::location(row), Gen4Wild::minLevel(row), 0);
        assert(result.status == Status::AlternateWildSource);
        assert(result.alternateSourceSpecies == species);
        assert(!result.wurmpleOnlyWithinWildEvidence());
    }

    return 0;
}
