#include "Legality/Gen3PcnyEventTemplate.h"

#include <cassert>
#include <iostream>
#include <string_view>

namespace {
Legality::Gen3PcnyEvent::Candidate candidateFor(
    const Legality::Gen3PcnyEvent::Entry& row,
    uint16_t species,
    std::u16string_view otName) {
    return {
        species,
        1,     // PCNY trainer IDs are non-zero and below 3000.
        0,
        2,     // Ruby/Sapphire origin accepted by the pinned template.
        2,     // non-Japanese recipient language
        row.level,
        255,
        4,
        false,
        false,
        otName,
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

    // EncounterGift3NY matching is evaluated against an evolution criterion.
    // The three gifts in the distribution literally named "Evolution" therefore
    // retain provenance after their valid Gen III evolutions.
    assert(speciesHistoryMatches(kEntries[0], 25));
    assert(speciesHistoryMatches(kEntries[0], 26));   // Pikachu -> Raichu
    assert(!speciesHistoryMatches(kEntries[0], 172)); // Pichu: reverse history

    assert(speciesHistoryMatches(kEntries[1], 44));
    assert(speciesHistoryMatches(kEntries[1], 45));   // Gloom -> Vileplume
    assert(speciesHistoryMatches(kEntries[1], 182));  // Gloom -> Bellossom
    assert(!speciesHistoryMatches(kEntries[1], 43));  // Oddish: reverse history

    assert(speciesHistoryMatches(kEntries[2], 120));
    assert(speciesHistoryMatches(kEntries[2], 121));  // Staryu -> Starmie

    // EvoCriteria is not limited to the distribution named "Evolution". Lock
    // every other evolvable species present in the pinned 50-row PCNY catalog.
    assert(kEntries[3].species == 117);
    assert(speciesHistoryMatches(kEntries[3], 230));  // Seadra -> Kingdra

    assert(kEntries[11].species == 353);
    assert(speciesHistoryMatches(kEntries[11], 354)); // Shuppet -> Banette

    assert(kEntries[12].species == 355);
    assert(speciesHistoryMatches(kEntries[12], 356)); // Duskull -> Dusclops

    assert(kEntries[28].species == 228);
    assert(speciesHistoryMatches(kEntries[28], 229)); // Houndour -> Houndoom

    assert(kEntries[29].species == 179);
    assert(speciesHistoryMatches(kEntries[29], 180)); // Mareep -> Flaaffy
    assert(speciesHistoryMatches(kEntries[29], 181)); // Mareep -> Ampharos

    assert(kEntries[34].species == 298);
    assert(speciesHistoryMatches(kEntries[34], 183)); // Azurill -> Marill
    assert(speciesHistoryMatches(kEntries[34], 184)); // Azurill -> Azumarill

    assert(kEntries[35].species == 360);
    assert(speciesHistoryMatches(kEntries[35], 202)); // Wynaut -> Wobbuffet

    // Directionality remains strict. A separately distributed final evolution
    // cannot reconstruct back into an earlier stage.
    assert(kEntries[38].species == 230);
    assert(!speciesHistoryMatches(kEntries[38], 117)); // Kingdra -> Seadra is impossible
    assert(!speciesHistoryMatches(kEntries[29], 178)); // unrelated species

    const Result antiShiny{Variant::ForceAntiShiny, 0, false};

    assert(matches(candidateFor(kEntries[0], 26, u"PCNYb"), antiShiny));
    assert(matches(candidateFor(kEntries[1], 45, u"PCNYb"), antiShiny));
    assert(matches(candidateFor(kEntries[1], 182, u"PCNYb"), antiShiny));
    assert(matches(candidateFor(kEntries[2], 121, u"PCNYb"), antiShiny));

    assert(matches(candidateFor(kEntries[3], 230, u"PCNYb"), antiShiny));
    assert(matches(candidateFor(kEntries[11], 354, u"PCNYb"), antiShiny));
    assert(matches(candidateFor(kEntries[12], 356, u"PCNYb"), antiShiny));
    assert(matches(candidateFor(kEntries[28], 229, u"PCNYc"), antiShiny));
    assert(matches(candidateFor(kEntries[29], 180, u"PCNYc"), antiShiny));
    assert(matches(candidateFor(kEntries[29], 181, u"PCNYc"), antiShiny));
    assert(matches(candidateFor(kEntries[34], 183, u"PCNYd"), antiShiny));
    assert(matches(candidateFor(kEntries[34], 184, u"PCNYd"), antiShiny));
    assert(matches(candidateFor(kEntries[35], 202, u"PCNYd"), antiShiny));

    const Result regular{Variant::Regular, 0, false};
    assert(!matches(candidateFor(kEntries[3], 230, u"PCNYb"), regular));

    std::cout << "Gen III PCNY evolved gift provenance: PASS\n";
}
