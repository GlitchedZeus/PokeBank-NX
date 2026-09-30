#include "Legality/LegalityContext.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::CoverageLevel;
    using Legality::sourceGameProfile;

    const auto* red = sourceGameProfile("red_gb");
    assert(red && red->generation == 1 && red->maxSpecies == 151 && red->maxMove == 165);
    assert(red->encounterCoverage == CoverageLevel::Partial);

    const auto* crystal = sourceGameProfile("crystal_gbc");
    assert(crystal && crystal->generation == 2 && crystal->maxSpecies == 251 &&
           crystal->maxMove == 251);
    assert(crystal->encounterCoverage == CoverageLevel::Partial);

    const auto* emerald = sourceGameProfile("emerald_gba");
    assert(emerald && emerald->generation == 3 && emerald->maxSpecies == 386 &&
           emerald->maxMove == 354);
    assert(emerald->encounterCoverage == CoverageLevel::Partial);

    const auto* platinum = sourceGameProfile("platinum_nds");
    assert(platinum && platinum->generation == 4 && platinum->maxSpecies == 493 &&
           platinum->maxMove == 467);
    assert(platinum->encounterCoverage == CoverageLevel::Partial);

    Legality::CoverageSummary coverage{};
    assert(coverage.eventGift == CoverageLevel::None);

    assert(sourceGameProfile("black_nds") == nullptr);
    assert(Legality::sourceGeneration("heartgold_nds") == 4);
    assert(Legality::sourceGeneration("unknown") == 0);

    std::cout << "Gen I-IV legality source context model: PASS\n";
}
