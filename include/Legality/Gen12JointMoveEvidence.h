#pragma once
// Source-free native PK1/PK2 four-slot move-family coverage. Neither format
// stores an immutable original retail game, and Gen II pools include bounded
// Gen I Time Capsule compatibility. Pool matching NEVER proves an actual
// tradeback, joint acquisition timeline, parent, game or transfer event.
#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include <array>
#include <cstdint>

namespace Legality::Gen12JointMoveEvidence {
enum class Bound:uint8_t { Empty, OneFamily, TwoFamilies, Unresolved };
struct Result {
    Bound bound=Bound::Unresolved;
    uint8_t familyMask=0;
    uint8_t minFamilies=0;
    bool anyGameFamilyUnknown=true;
};

inline uint8_t moveMask(uint8_t gen,uint16_t species,uint16_t move) noexcept {
    if(gen==1) {
        using namespace PokeVault::Integration::Gen1;
        return (MoveCompatibility::canLearnMove(SourceGame::Red,species,move)?1u:0u) |
               (MoveCompatibility::canLearnMove(SourceGame::Yellow,species,move)?2u:0u);
    }
    if(gen==2) {
        using namespace PokeVault::Integration::Gen2;
        return (MoveCompatibility::canLearnMove(SourceGame::Gold,species,move)?1u:0u) |
               (MoveCompatibility::canLearnMove(SourceGame::Crystal,species,move)?2u:0u);
    }
    return 0;
}

inline Result classify(uint8_t gen,uint16_t species,
                       const std::array<uint16_t,4>& moves) noexcept {
    if(gen!=1 && gen!=2)return {};
    uint8_t common=3,unionMask=0,used=0;
    for(uint16_t move:moves) {
        if(!move)continue;
        const auto family=moveMask(gen,species,move);
        if(!family)return {}; // Event/unknown transfer history unresolved.
        ++used;
        common&=family;
        unionMask|=family;
    }
    if(!used)return {Bound::Empty,0,0,true};
    if(common)
        return {Bound::OneFamily,common,1,true};
    return {Bound::TwoFamilies,unionMask,2,true};
}
} // namespace Legality::Gen12JointMoveEvidence
