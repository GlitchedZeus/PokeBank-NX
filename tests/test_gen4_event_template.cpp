#include "Legality/Gen4EventTemplate.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4EventTemplate;

    static_assert(kGen4EventTemplateCount == 247);

    // Pinned first Pokémon PCD in wc4.pkl: fixed-PID level-50 Pikachu.
    Candidate pikachu{
        25, 8107, 20846, 0x246e13afu,
        50, 3060, 16, 0, 2, 10, 0, true
    };
    const auto match = matchDirect(pikachu);
    assert(match.matched);
    assert(match.fixedPid);

    auto wrongLevel = pikachu;
    wrongLevel.metLevel = 51;
    assert(!matchDirect(wrongLevel).matched);

    auto wrongLocation = pikachu;
    wrongLocation.metLocation = 3059;
    assert(!matchDirect(wrongLocation).matched);

    auto wrongPid = pikachu;
    wrongPid.pid ^= 1u;
    assert(!matchDirect(wrongPid).matched);

    bool sawRandomAntiShiny = false;
    for (const auto& t : kGen4EventTemplates) {
        if (t.pid != 1)
            continue;
        uint32_t candidatePid = 2;
        while (shinyForTrainer(candidatePid, t.tid, t.sid))
            ++candidatePid;
        Candidate candidate{
            t.species, t.tid, t.sid, candidatePid,
            t.metLevel, t.metLocation, t.ball, t.form, t.language, t.version,
            t.otGender, t.fateful
        };
        assert(matchDirect(candidate).matched);
        sawRandomAntiShiny = true;
        break;
    }
    assert(sawRandomAntiShiny);

    std::cout << "Gen IV WC4 direct event-template evidence: PASS\n";
}
