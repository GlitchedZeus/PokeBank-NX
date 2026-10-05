#include "Legality/Gen3PcnyEventTemplate.h"

#include <cassert>
#include <iostream>

namespace {
Legality::Gen3PcnyEvent::Candidate evolutionGift(uint16_t species) {
    return {
        species,
        1,     // PCNY trainer IDs are non-zero and below 3000.
        0,
        2,     // Ruby/Sapphire origin accepted by the pinned template.
        2,     // non-Japanese recipient language
        50,
        255,
        4,
        false,
        false,
        u"PCNYb",
    };
}
}

int main() {
    using namespace Legality::Gen3PcnyEvent;
    using Legality::Gen3BacdPidIv::Result;
    using Legality::Gen3BacdPidIv::Variant;

    static_assert(kEventCount == 50);
    static_assert(kEntries[0].species == 25);
    static_assert(kEntries[1].species == 44);
    static_assert(kEntries[2].species == 120);
    static_assert(kEntries[0].distribution == static_cast<uint8_t>(Distribution::Evolution));
    static_assert(kEntries[1].distribution == static_cast<uint8_t>(Distribution::Evolution));
    static_assert(kEntries[2].distribution == static_cast<uint8_t>(Distribution::Evolution));

    // The pinned PCNY "Evolution" distribution contains Pikachu, Gloom and
    // Staryu. EncounterGift3NY matching is evaluated against an evolution
    // criterion, so their legitimate Gen III descendants retain provenance.
    assert(speciesHistoryMatches(kEntries[0], 25));
    assert(speciesHistoryMatches(kEntries[0], 26));   // Raichu
    assert(!speciesHistoryMatches(kEntries[0], 172)); // Pichu: reverse history

    assert(speciesHistoryMatches(kEntries[1], 44));
    assert(speciesHistoryMatches(kEntries[1], 45));   // Vileplume
    assert(speciesHistoryMatches(kEntries[1], 182));  // Bellossom
    assert(!speciesHistoryMatches(kEntries[1], 43));  // Oddish: reverse history

    assert(speciesHistoryMatches(kEntries[2], 120));
    assert(speciesHistoryMatches(kEntries[2], 121));  // Starmie

    const Result antiShiny{Variant::ForceAntiShiny, 0, false};
    assert(matches(evolutionGift(25), antiShiny));
    assert(matches(evolutionGift(26), antiShiny));
    assert(matches(evolutionGift(44), antiShiny));
    assert(matches(evolutionGift(45), antiShiny));
    assert(matches(evolutionGift(182), antiShiny));
    assert(matches(evolutionGift(120), antiShiny));
    assert(matches(evolutionGift(121), antiShiny));

    assert(!matches(evolutionGift(172), antiShiny));
    assert(!matches(evolutionGift(43), antiShiny));

    const Result regular{Variant::Regular, 0, false};
    assert(!matches(evolutionGift(26), regular));

    std::cout << "Gen III PCNY Evolution gift descendant provenance: PASS\n";
}
