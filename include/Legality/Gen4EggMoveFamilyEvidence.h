#pragma once
// Four-slot, positive-only Gen IV egg-move GAME FAMILY evidence.
// Family membership cannot prove a compatible pair of parents, event egg,
// move-learning chronology, or that any historical breeding took place.
#include "Legality/Gen34EggMoveEvidence.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4EggMoveFamily {
struct Result {
    uint8_t storedMoves=0;
    uint8_t originFamilyCandidates=0;
    uint8_t otherFamilyOnlyCandidates=0;
    uint8_t unmodeled=0;
    bool hasKnownOrigin=false;
};
inline Result classify(std::string_view originalRetailGame,
                       uint16_t species,
                       const std::array<uint16_t,4>& moves) noexcept {
    using namespace Gen34EggMove;
    const Group original=groupForId(originalRetailGame);
    Result result{};
    if(original!=Group::DiamondPearl &&
       original!=Group::Platinum &&
       original!=Group::HeartGoldSoulSilver)
        return result;
    result.hasKnownOrigin=true;
    for(const uint16_t move:moves) {
        if(move==0) continue;
        ++result.storedMoves;
        if(move>467) { ++result.unmodeled; continue; }
        if(Gen34EggMove::classify(originalRetailGame,species,move).matched()) {
            ++result.originFamilyCandidates;
            continue;
        }
        bool other=false;
        if(original!=Group::DiamondPearl)
            other|=Gen34EggMove::classify("diamond_nds",species,move).matched();
        if(original!=Group::Platinum)
            other|=Gen34EggMove::classify("platinum_nds",species,move).matched();
        if(original!=Group::HeartGoldSoulSilver)
            other|=Gen34EggMove::classify("heartgold_nds",species,move).matched();
        if(other)++result.otherFamilyOnlyCandidates;
        else ++result.unmodeled;
    }
    return result;
}
} // namespace Legality::Gen4EggMoveFamily
