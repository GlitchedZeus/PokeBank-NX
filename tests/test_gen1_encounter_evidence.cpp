#include "Legality/Gen1EncounterEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen1Encounter;

    static_assert(kStaticEntries.size() == 31);
    static_assert(kTradeEntries.size() == 17);

    // Red/Blue starter Bulbasaur is native and can later be an Ivysaur/Venusaur while
    // retaining the original catch-rate byte.
    auto starter = matchStatic("red_gb", 1, 5, 45);
    assert(starter.matched && starter.nativeToContainer && starter.originalSpecies == 1);
    auto evolved = matchStatic("blue_gb", 3, 32, 45);
    assert(evolved.matched && evolved.nativeToContainer && evolved.originalSpecies == 1);

    // Yellow starter Pikachu is Yellow-native but can legally inhabit a Red save after link trade.
    auto yellowPika = matchStatic("red_gb", 25, 5, 163);
    assert(yellowPika.matched && !yellowPika.nativeToContainer);

    // In-game trade OT marker + template evidence.
    auto mime = matchTrade("red_gb", 122, 6, 45, true);
    assert(mime.matched && mime.nativeToContainer && mime.originalSpecies == 122);
    assert(!matchTrade("red_gb", 122, 6, 45, false).matched);

    // Yellow Machoke trade evolves on trade; Machamp can retain Machoke's rate.
    auto machamp = matchTrade("yellow_gb", 68, 16, 90, true);
    assert(machamp.matched && machamp.nativeToContainer && machamp.originalSpecies == 67);

    // R/B Jynx can exist at the lower Gen II-return minimum after Time Capsule history.
    auto jynx = matchTrade("yellow_gb", 124, 10, 0x01, true);
    assert(jynx.matched && !jynx.nativeToContainer);

    assert(!matchStatic("red_gb", 150, 50, 3).matched); // Mewtwo below level 70.
    assert(!matchTrade("unknown", 122, 100, 45, true).matched);

    std::cout << "Gen I released static/trade provenance evidence: PASS\n";
}
