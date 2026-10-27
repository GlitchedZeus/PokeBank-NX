#ifndef POKEBANK_GEN5_DISPLAY_STATS_H
#define POKEBANK_GEN5_DISPLAY_STATS_H

#include "Integration/Gen5/Gen5Progression.h"
#include "Pokemon/Pokemon5ReadOnly.h"

#include <array>
#include <cstdint>

namespace PokeVault::Integration::Gen5 {

// The ONE battle-stat value the Box summary, View and Edit all show for a PK5.
// Party records carry the game's own stat cache (PKHeX PK5.cs 0x90..0x9A,
// max HP first). Boxed records carry none, so the native Gen III-V formula
// (computeStats5) is applied to their level, nature, IVs and EVs. Order: HP,
// Attack, Defense, Speed, Sp. Atk, Sp. Def. All zero when nothing is known.
[[nodiscard]] inline std::array<uint16_t,6> displayStats5(const Pokemon5ReadOnly& p,
                                                          bool black2White2) {
    if(!p.valid() || p.empty())return {};
    const auto bytes=p.decodedBytes();
    if(p.partyRecord()) {
        std::array<uint16_t,6> stats{};
        for(size_t i=0;i<6;++i)stats[i]=Crypto::read16(bytes,0x90+2*i);
        return stats;
    }
    const auto personal=personal5(p.species(),p.form(),black2White2);
    if(!personal || personal.info->growthRate>5 ||
       p.experience()>experienceForLevel5(personal.info->growthRate,100))return {};
    const uint8_t level=levelForExperience5(personal.info->growthRate,p.experience());
    const auto computed=computeStats5(p.species(),p.form(),level,p.nature(),
                                      Crypto::read32(bytes,0x38),p.evs(),black2White2);
    return computed?*computed:std::array<uint16_t,6>{};
}

} // namespace PokeVault::Integration::Gen5
#endif
