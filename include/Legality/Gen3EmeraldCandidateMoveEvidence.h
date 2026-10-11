#pragma once
// MOVE-001/MOVE-002: attach source-pinned Emerald current-species LEVEL-UP
// witnesses to one explicitly supplied encounter-candidate hypothesis.
// This is a *conditional* method/level bridge. Original game alone does not
// establish a real encounter candidate or prove all four moves coexisted.
// Neither missing move matches nor candidate-local contradictions are Invalid.
#include "Legality/Gen14CandidateMoveHistory.h"
#include "Legality/Gen3EmeraldLevelUpEvidence.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3EmeraldCandidateMoveEvidence {
using namespace Gen14CandidateMoveHistory;
struct Result {
    uint8_t storedMoves=0;
    uint8_t directLevelUpSlots=0;
    Gen14CandidateMoveHistory::Result joint{};
};
constexpr Result classify(const Candidate& candidate,uint16_t species,
                          const std::array<uint16_t,4>& moves) noexcept {
    Result result{};
    std::array<Slot,4> slots{};
    for(uint8_t i=0;i<moves.size();++i) {
        const uint16_t move=moves[i];
        if(!move)continue;
        ++result.storedMoves;
        slots[i].move=move;
        // The pinned table only covers this species in Emerald. The caller
        // must independently establish original-game provenance and define
        // the encounter and stage bounds for this conditional candidate.
        const uint8_t level=Gen3EmeraldLevelUp::firstLevelBetween(
            species,move,candidate.minEncounterLevel,candidate.currentLevel);
        if(!candidate.id || candidate.finalStage<1 || level==0)continue;
        slots[i].options[0]={move,candidate.id,Method::LevelUp,
                            level,level,1,0,0,true};
        slots[i].count=1;
        ++result.directLevelUpSlots;
        // Never mark a slot exhaustive: ancestors, TMs, tutors, breeding,
        // events, transfers, relearning and actual move retention differ.
    }
    result.joint=evaluate(candidate,slots);
    return result;
}
} // namespace Legality::Gen3EmeraldCandidateMoveEvidence
