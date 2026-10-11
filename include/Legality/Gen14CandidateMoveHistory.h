#pragma once
// MOVE-001: bounded encounter-candidate-scoped move witness evaluation.
// A compatibility table is not a verified acquisition method, and a compatible
// witness set is not proof that a full four-slot history actually occurred.
// Never convert CandidateContradicted to global Invalid unless *all* genuine
// encounter candidates and all acquisition paths were independently exhausted.
#include "Legality/Gen34PalParkHMIds.h"
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMoveHistory {

enum class Method : uint8_t {
    BroadPool, LevelUp, Egg, Machine, Tutor, Event,
    SiblingEvolution, TransferRetained, LaterAcquisition
};
enum class Status : uint8_t {
    Empty, WitnessesCompatible, Unresolved, CandidateContradicted
};
struct Witness {
    uint16_t move=0;
    uint32_t candidateId=0;     // One original encounter / gift candidate.
    Method method=Method::BroadPool;
    uint8_t minLevel=1;
    uint8_t maxLevel=100;
    uint8_t stage=0;            // Ordered generation/evolution/transfer stage.
    uint16_t eventId=0;        // A nonzero exclusive distribution identity.
    uint16_t parentId=0;       // Nonzero shared breeding-parent identity.
    bool methodVerified=false; // Broad family membership is NOT a method.
};
struct Slot {
    uint16_t move=0;
    std::array<Witness,8> options{};
    uint8_t count=0;
    bool exhaustive=false;
};
struct Candidate {
    uint32_t id=0;
    uint8_t currentLevel=1;
    uint8_t minEncounterLevel=1;
    bool allRelevantRoutesEnumerated=false;
    uint8_t finalStage=5;       // No acquisition beyond observed stage.
    // If nonzero, CALLER has independently verified Gen III -> IV Pal
    // Park provenance and assigned this ordered boundary stage. The
    // engine must not infer Pal Park merely from an origin label.
    uint8_t verifiedPalParkStage=0;
};
struct Result {
    Status status=Status::Unresolved;
    uint8_t storedSlots=0;
    uint8_t coveredSlots=0;
    bool hasUnverifiedMethod=false;
};

// Independent methods can coexist. Two moves gained solely by the same
// one-time sibling evolution must agree on its evolution level; event/known
// parent witnesses must refer to the *same* event/parent, not a union of
// incompatible distributions. Unknown event/parent IDs do not certify a
// joint route, but never disprove one.
struct Selection {
    uint16_t eventId=0;
    uint16_t parentId=0;
    uint8_t siblingEvolutionLevel=0;
    bool unknownExclusive=false;
    bool broadMethod=false;
};
constexpr bool extend(Selection& choice, const Witness& w) noexcept {
    if (w.method==Method::Event) {
        if(w.eventId==0) choice.unknownExclusive=true;
        else if(choice.eventId!=0 && choice.eventId!=w.eventId) return false;
        else choice.eventId=w.eventId;
    }
    if(w.method==Method::Egg) {
        if(w.parentId==0) choice.unknownExclusive=true;
        else if(choice.parentId!=0 && choice.parentId!=w.parentId) return false;
        else choice.parentId=w.parentId;
    }
    if(w.method==Method::SiblingEvolution) {
        // Single evolution transition: level is one exact witness, not a
        // pair of unrelated opportunities separated by seven levels.
        if(w.minLevel!=w.maxLevel) choice.unknownExclusive=true;
        else if(choice.siblingEvolutionLevel &&
                choice.siblingEvolutionLevel!=w.minLevel) return false;
        else choice.siblingEvolutionLevel=w.minLevel;
    }
    if(!w.methodVerified || w.method==Method::BroadPool)
        choice.broadMethod=true;
    return true;
}
constexpr bool valid(const Candidate& candidate, const Witness& w) noexcept {
    // A candidate with verified Pal Park provenance cannot treat an old
    // Generation III HM as retained across migration. HM on the native PK4
    // instead needs an independently verified later-acquisition witness.
    // This rejects only this explicitly supplied candidate path, never a
    // Pokémon or all alternative events, trades and histories.
    if(candidate.verifiedPalParkStage!=0) {
        if(candidate.verifiedPalParkStage>candidate.finalStage)return false;
        if(w.method==Method::TransferRetained &&
           Gen34PalParkHMIds::isGen3HM(w.move))return false;
        if(w.method==Method::TransferRetained &&
           w.stage>candidate.verifiedPalParkStage)return false;
        if(w.method==Method::LaterAcquisition &&
           w.stage<=candidate.verifiedPalParkStage)return false;
    }
    return candidate.id!=0 && w.candidateId==candidate.id &&
        w.move!=0 && w.minLevel<=w.maxLevel &&
        w.minLevel<=candidate.currentLevel && w.stage<=candidate.finalStage &&
        ((w.method==Method::Egg || w.method==Method::Event ||
          w.method==Method::TransferRetained) ||
         w.maxLevel>=candidate.minEncounterLevel);
}
struct Search {
    bool complete=false;
    bool incomplete=false;
    uint8_t bestCoverage=0;
};
// At most eight witnesses per each of four observed moves: <=4096 states;
// no heap, recursion to fixed depth four, no global game move enumeration.
constexpr void search(const Candidate& candidate,
                      const std::array<Slot,4>& slots,
                      const std::array<uint8_t,4>& positions,
                      uint8_t used, uint8_t depth,
                      Selection choice, Search& state) noexcept {
    if(depth==used) {
        if(!choice.unknownExclusive && !choice.broadMethod) state.complete=true;
        else state.incomplete=true;
        return;
    }
    const Slot& slot=slots[positions[depth]];
    for(uint8_t i=0;i<slot.count && i<slot.options.size();++i) {
        const Witness& w=slot.options[i];
        if(w.move!=slot.move || !valid(candidate,w)) continue;
        Selection next=choice;
        if(!extend(next,w)) continue;
        if(depth+1>state.bestCoverage) state.bestCoverage=depth+1;
        search(candidate,slots,positions,used,depth+1,next,state);
        if(state.complete) return;
    }
}
constexpr Result evaluate(const Candidate& candidate,
                          const std::array<Slot,4>& slots) noexcept {
    Result result{};
    std::array<uint8_t,4> pos{};
    bool completeTables=candidate.allRelevantRoutesEnumerated;
    for(uint8_t i=0;i<slots.size();++i) {
        const auto& slot=slots[i];
        if(slot.move==0)continue;
        pos[result.storedSlots++]=i;
        if(!slot.exhaustive || slot.count>slot.options.size())
            completeTables=false;
    }
    if(!result.storedSlots) {result.status=Status::Empty;return result;}
    if(!candidate.id || !candidate.currentLevel ||
       candidate.minEncounterLevel>candidate.currentLevel) return result;
    Search findings{};
    search(candidate,slots,pos,result.storedSlots,0,{},findings);
    result.coveredSlots=findings.bestCoverage;
    result.hasUnverifiedMethod=findings.incomplete;
    if(findings.complete) result.status=Status::WitnessesCompatible;
    else if(findings.incomplete || !completeTables)
        result.status=Status::Unresolved;
    else result.status=Status::CandidateContradicted;
    return result;
}
} // namespace Legality::Gen14CandidateMoveHistory
