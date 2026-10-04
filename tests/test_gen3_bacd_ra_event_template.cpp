#include "Legality/Gen3BacdRaEventTemplate.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality;
    using namespace Legality::Gen3BacdRaEvent;

    static_assert(kEventCount == 114);

    // Restricted anti-shiny 10ANNIV vector reconstructed from the pinned BA-CD
    // algorithm. Origin seed 0x04A4 yields a RandS7 OT-gender result of male.
    const auto rng = Gen3BacdPidIv::analyzeWithTrainer(
        0x3C442418u, {16, 15, 6, 22, 30, 17}, 6227, 0);
    assert(rng.matched());
    assert(rng.variant == Gen3BacdPidIv::Variant::RegularAntiShiny);
    assert(rng.restrictedSeed);
    assert(rng.originSeed == 0x04A4u);
    assert(expectedOtGender(
        static_cast<uint8_t>(OtGenderRule::RandS7), rng.originSeed) == 0);

    Candidate charizard{
        6, 6227, 0,
        2,   // Ruby
        2,   // English
        0,   // male OT from RandS7
        70, 255, 4,
        false, false, false, false,
        u"10ANNIV"
    };
    const auto matched = match(charizard, rng);
    assert(matched.matched);
    assert((matched.initialMoves == std::array<uint16_t, 4>{17, 163, 82, 83}));

    // EncounterGift3.IsMatchExact does not require a non-egg gift's current
    // moves to remain at its distribution-time payload. The source moves above
    // are retained as provenance metadata, not an immutable encounter field.

    auto wrongGender = charizard;
    wrongGender.otGender = 1;
    assert(!match(wrongGender, rng).matched);

    auto wrongLanguage = charizard;
    wrongLanguage.language = 5; // German row uses OT 10JAHRE.
    assert(!match(wrongLanguage, rng).matched);

    auto wrongOt = charizard;
    wrongOt.otName = u"WISHMKR";
    assert(!match(wrongOt, rng).matched);

    auto wrongOrigin = charizard;
    wrongOrigin.originGame = 1;
    assert(!match(wrongOrigin, rng).matched);

    auto wrongShiny = charizard;
    wrongShiny.shiny = true;
    assert(!match(wrongShiny, rng).matched);

    auto unrestricted = rng;
    unrestricted.restrictedSeed = false;
    assert(!match(charizard, unrestricted).matched);

    // PKHeX RibbonVerifierEvent3 requires National Ribbon to exactly match the
    // WC3 template. The pinned BACD_R_A catalog has exactly two such rows:
    // Japanese FESTA Metang and English ROCKS Metang (both TID 02005).
    std::size_t nationalRibbonRows = 0;
    for (const auto& row : kEntries) {
        if (!row.ribbonNational)
            continue;
        ++nationalRibbonRows;
        assert(row.species == 375);
        assert(row.tid == 2005);

        Candidate metang{
            row.species, row.tid, row.sid,
            2, row.language, 0,
            row.level, 255, 4,
            false, row.fateful, true, false,
            row.otName
        };
        assert(persistentFieldsMatch(row, metang, 0));
        metang.ribbonNational = false;
        assert(!persistentFieldsMatch(row, metang, 0));
    }
    assert(nationalRibbonRows == 2);

    // BACD_R_A also permits the unmodified regular BA-CD outcome when the
    // raw PID was already non-shiny; the seed restriction still applies.
    const Gen3BacdPidIv::Result regularRestricted{
        Gen3BacdPidIv::Variant::Regular, rng.originSeed, true
    };
    assert(rngCompatible(regularRestricted));

    std::cout << "Gen III BACD_R_A event-template + initial-move + National Ribbon evidence: PASS\n";
}
