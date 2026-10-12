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
    // MOVE-002: ONLY an independently verified Gen II -> Gen I ->
    // Gen II Time Capsule visit. The stored PK2 itself does not say this
    // trip occurred. Ordinal stage values distinguish entry and return.
    uint8_t verifiedTimeCapsuleEntryStage=0;
    uint8_t verifiedTimeCapsuleReturnStage=0;
    uint16_t speciesAtTimeCapsule=0; // Independently sourced Gen I species
    bool retailTimeCapsulePolicyVerified=false; // Korean/Stadium variants unknown
    // Destination format generation (1..4), if independently known.
    // 0 means unknown: still reject >Gen IV move ids, never invent a
    // narrower source generation from a source-free Pokemon.
    uint8_t observedGeneration=0;
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
        // The SAME original Pokémon cannot hatch from a second egg
        // or receive a second one-time origin event after migrating.
        // Existing inherited/event moves may be retained from BEFORE
        // the edge, subject to the other transfer restrictions.
        if((w.method==Method::Egg || w.method==Method::Event) &&
           w.stage>=candidate.verifiedPalParkStage)return false;
        if(w.method==Method::TransferRetained &&
           Gen34PalParkHMIds::isGen3HM(w.move))return false;
        if(w.method==Method::TransferRetained &&
           w.stage>candidate.verifiedPalParkStage)return false;
        if(w.method==Method::LaterAcquisition &&
           w.stage<=candidate.verifiedPalParkStage)return false;
    }
    // A Gen II-only move (>165) cannot survive a *verified* visit
    // through the Gen I game. A candidate may reacquire it only after
    // returning to Gen II, with an independent verified method. An
    // inherited egg-only move is never simply re-inherited by the SAME
    // individual after returning. This excludes one asserted route only.
    if(candidate.verifiedTimeCapsuleEntryStage!=0) {
        // An original birth or original event distribution is a one-time
        // acquisition: the same individual cannot receive another egg
        // inheritance or its original gift *after* the verified visit.
        // Later level/tutor/machine teaching is a separate method.
        if((w.method==Method::Egg || w.method==Method::Event) &&
           w.stage>=candidate.verifiedTimeCapsuleEntryStage)return false;
        if(w.move>165) {
            // Neither an original Gen II event nor an alleged
            // retained transfer can carry a >165 move across Gen I.
            // Reacquisition requires a real later method witness.
            if(w.method==Method::Egg || w.method==Method::Event ||
               w.method==Method::TransferRetained ||
               w.stage<=candidate.verifiedTimeCapsuleReturnStage)
                return false;
        }
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
    // A malformed hypothetical current four-slot snapshot (duplicate
    // move IDs, or a nonzero move after an empty slot) cannot certify
    // one retail acquisition history. These malformed *inputs* are
    // Unresolved, NEVER an exhaustive candidate contradiction.
    uint16_t moveCeiling=467;
    switch(candidate.observedGeneration) {
        case 0: case 4: moveCeiling=467;break;
        case 3:moveCeiling=354;break;
        case 2:moveCeiling=251;break;
        case 1:moveCeiling=165;break;
        default:return result;
    }
    bool foundEmpty=false;
    for(uint8_t i=0;i<slots.size();++i) {
        if(!slots[i].move){foundEmpty=true;continue;}
        if(slots[i].move>moveCeiling)return result;
        if(foundEmpty)return result;
        for(uint8_t j=0;j<i;++j)
            if(slots[j].move==slots[i].move)return result;
    }
    if(!candidate.id || !candidate.currentLevel ||
       candidate.minEncounterLevel>candidate.currentLevel) return result;
    if(candidate.verifiedTimeCapsuleEntryStage!=0) {
        // Caller must establish BOTH directions, a legal Gen I species
        // at the boundary, and a verified retail-language/source policy.
        // Missing details are Unresolved, never CandidateContradicted.
        if(!candidate.retailTimeCapsulePolicyVerified ||
           candidate.verifiedTimeCapsuleReturnStage <=
               candidate.verifiedTimeCapsuleEntryStage ||
           candidate.verifiedTimeCapsuleReturnStage>candidate.finalStage ||
           candidate.speciesAtTimeCapsule<1 ||
           candidate.speciesAtTimeCapsule>151)
            return result;
    }
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
// L3: keep new chronology helpers in the existing exact-head host compile
// graph without modifying coordinator-owned Makefiles or central analyzer.
#include "Legality/Gen14CandidateMoveChronology.h"
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include "Legality/Gen14CandidateMoveMachineEvidence.h"
