#include "Legality/Gen2CrystalEvolvedWildEvidence.h"
#include <cassert>

int main() {
    namespace G=Legality::Gen2CrystalEvolvedWild;
    // Real pinned Crystal grass encounter: Pidgey #16, location2,
    // levels2/3, timeMask 0x02 (time selector1: daytime).
    constexpr uint64_t row=0x200820020210ULL;
    static_assert(Legality::Gen2Wild::game(row)==
                  Legality::Gen2Wild::Game::Crystal);
    static_assert(Legality::Gen2Wild::species(row)==16);
    static_assert(Legality::Gen2Wild::location(row)==2);
    static_assert(Legality::Gen2Wild::minLevel(row)==2);
    static_assert(Legality::Gen2Wild::maxLevel(row)==2);
    static_assert(Legality::Gen2Wild::timeMask(row)==2);
    constexpr uint16_t caughtPidgey=(1u<<14)|(2u<<8)|2u;
    assert(Legality::Gen2Wild::matchesCrystalCaughtData(16,caughtPidgey));
    assert(G::analyze(17,18,caughtPidgey,false)==G::Ancestor::Pidgey);
    assert(G::analyze(18,36,caughtPidgey,false)==G::Ancestor::Pidgey);
    assert(G::analyze(18,50,caughtPidgey,false)==G::Ancestor::Pidgey);
    assert(G::analyze(17,17,caughtPidgey,false)==G::Ancestor::None);
    assert(G::analyze(18,35,caughtPidgey,false)==G::Ancestor::None);
    assert(G::analyze(18,36,caughtPidgey,true)==G::Ancestor::None);
    assert(G::analyze(18,36,0,false)==G::Ancestor::None);
    assert(G::analyze(18,36,(3u<<14)|(2u<<8)|2u,false)==G::Ancestor::None);
    assert(G::analyze(18,36,(1u<<14)|(63u<<8)|2u,false)==G::Ancestor::None);
    assert(G::analyze(18,36,(1u<<14)|(2u<<8)|127u,false)==G::Ancestor::None);
    assert(G::analyze(16,36,caughtPidgey,false)==G::Ancestor::None);
    assert(G::analyze(25,36,caughtPidgey,false)==G::Ancestor::None);


    // Exact pinned Crystal encounter_r/s/e-style generation II wild rows:
    // data is from the repo's generated PKHeX Gen2WildEncounterData.inc.
    // Each ancestor is a distinct Crystal wild source. Catch level/time
    // persist through level-up evolution into the current species.
    struct Vector {
        uint64_t row;
        uint16_t ancestor;
        uint16_t evolved;
        uint8_t currentLevel;
        uint8_t expectedMetLevel;
        G::Ancestor expected;
    };
    constexpr Vector vectors[]={
        {0x210c3002040aULL,10,11,7,3,G::Ancestor::Caterpie},
        {0x240c3002040dULL,13,14,7,3,G::Ancestor::Weedle},
        {0x240820020213ULL,19,20,20,2,G::Ancestor::Rattata},
        {0x211860020b15ULL,21,22,20,6,G::Ancestor::Spearow},
        {0x240c30080429ULL,41,42,22,3,G::Ancestor::Zubat},
        {0x2108200202a1ULL,161,162,15,2,G::Ancestor::Sentret},
    };
    for(const auto& vec : vectors){
        assert(Legality::Gen2Wild::game(vec.row)==
               Legality::Gen2Wild::Game::Crystal);
        assert(Legality::Gen2Wild::species(vec.row)==vec.ancestor);
        assert(Legality::Gen2Wild::minLevel(vec.row)==vec.expectedMetLevel);
        assert(Legality::Gen2Wild::maxLevel(vec.row)==vec.expectedMetLevel);
        const uint8_t timeMask=Legality::Gen2Wild::timeMask(vec.row);
        const uint8_t time=timeMask==8 ? 3 : 1;
        assert(Legality::Gen2Wild::timeAllows(timeMask,time));
        const uint16_t caught=static_cast<uint16_t>(
            (static_cast<uint16_t>(time)<<14) |
            (static_cast<uint16_t>(vec.expectedMetLevel)<<8) |
            Legality::Gen2Wild::location(vec.row));
        assert(Legality::Gen2Wild::matchesCrystalCaughtData(
            vec.ancestor,caught));
        assert(G::analyze(vec.evolved,vec.currentLevel,caught,false)==
               vec.expected);
        assert(G::analyze(vec.evolved,vec.currentLevel-1,caught,false)==
               G::Ancestor::None);
        assert(G::analyze(vec.evolved,vec.currentLevel,caught,true)==
               G::Ancestor::None);
        assert(G::analyze(vec.evolved,vec.currentLevel,0,false)==
               G::Ancestor::None);
    }

    // Multi-step evolution requires two *separate* level increases after
    // capture, not merely enough total level for the final species.
    const uint16_t caterpie=(1u<<14)|(3u<<8)|4u;
    const uint16_t weedle=(1u<<14)|(3u<<8)|4u;
    assert(G::analyze(12,10,caterpie,false)==G::Ancestor::Caterpie);
    assert(G::analyze(15,10,weedle,false)==G::Ancestor::Weedle);
    assert(G::analyze(12,9,caterpie,false)==G::Ancestor::None);
    assert(G::analyze(15,9,weedle,false)==G::Ancestor::None);
    // A caught-at-evolution-level base cannot immediately be evolved:
    const uint16_t caughtAtSeven=(1u<<14)|(7u<<8)|8u;
    if(Legality::Gen2Wild::matchesCrystalCaughtData(10,caughtAtSeven)){
        assert(G::analyze(11,7,caughtAtSeven,false)==G::Ancestor::None);
        assert(G::analyze(12,8,caughtAtSeven,false)==G::Ancestor::None);
    }

    // Pinned Crystal surfing Tentacool #72 at location1, levels20-24,
    // any timeMask=0; an evolved Tentacruel from that source can exist.
    constexpr uint64_t tentacool=0x206141000148ULL;
    static_assert(Legality::Gen2Wild::game(tentacool)==
                  Legality::Gen2Wild::Game::Crystal);
    static_assert(Legality::Gen2Wild::species(tentacool)==72);
    static_assert(Legality::Gen2Wild::location(tentacool)==1);
    static_assert(Legality::Gen2Wild::minLevel(tentacool)==20);
    static_assert(Legality::Gen2Wild::maxLevel(tentacool)==24);
    constexpr uint16_t caughtTentacool=(1u<<14)|(20u<<8)|1u;
    assert(G::analyze(73,30,caughtTentacool,false)==G::Ancestor::Tentacool);
    assert(G::analyze(73,29,caughtTentacool,false)==G::Ancestor::None);
    assert(G::analyze(73,30,caughtTentacool,true)==G::Ancestor::None);
    assert(G::analyze(73,30,(1u<<14)|(63u<<8)|1u,false)==G::Ancestor::None);
}
