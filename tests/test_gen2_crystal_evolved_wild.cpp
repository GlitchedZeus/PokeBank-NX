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
