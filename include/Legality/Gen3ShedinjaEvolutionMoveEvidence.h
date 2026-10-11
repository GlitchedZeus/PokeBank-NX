#pragma once
// MOVE-003: conditional Emerald Shedinja sibling-copy evidence.
// Shedinja copies the resulting Ninjask's four-move state at a *single*
// evolution, not Ninjask's lifetime move union. No global Invalid:
// other encounter, tutor, transfer, event or move-relearning routes may exist.
#include "Legality/Gen14CandidateMoveHistory.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3ShedinjaEvolutionMove {
struct Result {
    bool ordinaryAgilityCopyPossible=false;
    bool ordinaryBatonCopyPossible=false;
    bool oneEvolutionCannotIntroduceBoth=false;
};
constexpr bool contains(const std::array<uint16_t,4>& moves,
                        uint16_t target) noexcept {
    for(uint16_t m:moves)if(m==target)return true;
    return false;
}
// Requires independently established Emerald-native PK3 original-game code.
constexpr Result classify(uint16_t species, uint8_t currentLevel,
                          const std::array<uint16_t,4>& moves) noexcept {
    if(species!=292 || currentLevel<1 || currentLevel>100) return {};
    const bool agility=contains(moves,97);
    const bool baton=contains(moves,226);
    Result result{};
    result.ordinaryAgilityCopyPossible=agility && currentLevel>=38;
    result.ordinaryBatonCopyPossible=baton && currentLevel>=45;
    if(result.ordinaryAgilityCopyPossible && result.ordinaryBatonCopyPossible) {
        using namespace Legality::Gen14CandidateMoveHistory;
        Slot first{};first.move=97;first.count=1;first.exhaustive=true;
        first.options[0]={97,1,Method::SiblingEvolution,38,38,1,0,0,true};
        Slot second{};second.move=226;second.count=1;second.exhaustive=true;
        second.options[0]={226,1,Method::SiblingEvolution,45,45,1,0,0,true};
        // This completeness is ONLY for a chosen ordinary one-copy
        // candidate, not all real-world history alternatives.
        const Candidate ordinary{1,currentLevel,1,true,1};
        result.oneEvolutionCannotIntroduceBoth=
            evaluate(ordinary,{first,second,{},{}}).status==
                Status::CandidateContradicted;
    }
    return result;
}
} // namespace Legality::Gen3ShedinjaEvolutionMove
