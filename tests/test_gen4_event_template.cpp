#include "Legality/Gen4EventTemplate.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4EventTemplate;

    static_assert(kGen4EventTemplateCount == 247);

    // Pinned first Pokémon PCD in wc4.pkl: fixed-PID level-50 Pikachu.
    Candidate pikachu{
        25, 8107, 20846, 0x246e13afu,
        50, 3060, 16, 0, 0, 2, 10, 0, true
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

    auto wrongForm = pikachu;
    wrongForm.form = 1;
    assert(!matchDirect(wrongForm).matched);

    auto wrongGender = pikachu;
    wrongGender.gender = 1;
    assert(!matchDirect(wrongGender).matched);

    // Evolution must not erase event provenance. The fixed-PID Pikachu can
    // evolve into Raichu while retaining trainer/PID/met/ball/event fields.
    auto evolvedRaichu = pikachu;
    evolvedRaichu.species = 26;
    const auto evolved = matchEvolutionLine(evolvedRaichu);
    assert(evolved.matched);
    assert(evolved.evolved);
    assert(evolved.sourceSpecies == 25);
    assert(evolved.cardId == match.cardId);
    assert(evolved.fixedPid);

    auto unrelated = pikachu;
    unrelated.species = 27;
    assert(!matchEvolutionLine(unrelated).matched);

    bool sawFormChangeableEvent = false;
    for (const auto& t : kGen4EventTemplates) {
        if (t.species != 492 || t.form != 0)
            continue;
        uint32_t candidatePid = t.pid;
        if (candidatePid == 1) {
            candidatePid = 2;
            while (shinyForTrainer(candidatePid, t.tid, t.sid))
                ++candidatePid;
        }
        Candidate candidate{
            t.species, t.tid, t.sid, candidatePid,
            t.metLevel, t.metLocation, t.ball, 1, t.gender, t.language, t.version,
            t.otGender, t.fateful
        };
        assert(matchDirect(candidate).matched);
        sawFormChangeableEvent = true;
        break;
    }
    assert(sawFormChangeableEvent);

    bool sawRandomAntiShiny = false;
    for (const auto& t : kGen4EventTemplates) {
        if (t.pid != 1)
            continue;
        uint32_t candidatePid = 2;
        while (shinyForTrainer(candidatePid, t.tid, t.sid))
            ++candidatePid;
        Candidate candidate{
            t.species, t.tid, t.sid, candidatePid,
            t.metLevel, t.metLocation, t.ball, t.form,
            static_cast<uint8_t>((t.gender + 1u) % 3u),
            t.language, t.version, t.otGender, t.fateful
        };
        assert(matchDirect(candidate).matched);
        sawRandomAntiShiny = true;
        break;
    }
    assert(sawRandomAntiShiny);

    std::cout << "Gen IV WC4 direct event-template evidence: PASS\n";
}
