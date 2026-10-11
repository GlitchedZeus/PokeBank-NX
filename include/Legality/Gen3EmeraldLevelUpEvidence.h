#pragma once
// Positive-only Generation III Emerald LEVEL-UP METHOD evidence.
// Pinned pret/pokeemerald 5eff78649e7170a877b961ef0b3da13b81a16038
// through tools/gen_legality_gen3_emerald_levelup.py.
// National Dex 1..386, NOT Emerald's internal species numbering.
// A match says only that THIS SPECIES has a listed level-up opportunity by
// its current level. It does not establish actual acquisition or rule out
// pre-evolution, tutor, TM/HM, egg, event and trading histories.
#include <array>
#include <cstdint>

namespace Legality::Gen3EmeraldLevelUp {
#include "Legality/Gen3EmeraldLevelUpData.inc"

// 0 means "no positively matched direct Emerald level-up method by level";
// never interpret it as an impossible Pokémon or move.
constexpr uint8_t firstLevelAtOrBefore(
    uint16_t nationalSpecies,uint16_t moveId,uint8_t currentLevel) noexcept {
    if(nationalSpecies==0 || nationalSpecies>386 ||
       moveId==0 || moveId>354 || currentLevel==0)
        return 0;
    uint8_t earliest=0;
    for(uint16_t i=kEmeraldLevelOffsets[nationalSpecies];
        i<kEmeraldLevelOffsets[nationalSpecies+1];++i) {
        const uint16_t entry=kEmeraldPackedLevelMoves[i];
        const uint16_t move=entry&511u;
        const uint8_t level=static_cast<uint8_t>(entry>>9);
        if(move==moveId && level<=currentLevel &&
           (earliest==0 || level<earliest))
            earliest=level;
    }
    return earliest;
}

} // namespace Legality::Gen3EmeraldLevelUp
