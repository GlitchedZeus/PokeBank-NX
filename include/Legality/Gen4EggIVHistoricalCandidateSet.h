#pragma once
// L2 B16 (RNG-003): an independently identified, bounded family of
// SAME-CHILD historical Gen IV IV-stage hypotheses. Only a supplied
// original-stage witness can attest daycare slot order, parent IVs,
// pre-inheritance child IVs, held items, or an LCRNG stage endpoint.
// Candidate-local contradictions must never become global Invalid.
#include "Legality/Gen4EggIVInheritanceStage.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4EggIVHistoricalCandidateSet {
namespace IV=Gen4EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceeded,
    UnknownOriginalParentStage, UnknownOriginalEggStage,
    IncoherentStageIdentity, MissingOrDuplicateRouteIdentity,
    MalformedOriginalIVs, ConditionalHistoryMatched,
    UnresolvedCandidateHistory, UnknownHistoryEnumeration,
    AllEnumeratedStageHypothesesContradicted
};
struct Hypothesis {
    uint64_t routeIdentity=0;
    uint64_t stageIdentity=0;
    // Each history is an independent collection-time claim; neither
    // present parent heldItem nor child heldItem is used as evidence.
    bool independentSourceAttestation=false;
    bool historicalHeldItemsKnown=false;
    uint16_t physicalSlot0Item=0,physicalSlot1Item=0;
    bool finalRNGStateKnown=false;
    uint32_t finalRNGState=0;
};
struct Input {
    std::string_view game{};
    std::array<uint8_t,6> initialChild{},physicalSlot0{},physicalSlot1{},observedChild{};
    uint64_t originalStageIdentity=0;
    bool originalPhysicalParentSnapshotsVerified=false;
    bool originalPhysicalSlotOrderVerified=false;
    bool originalInitialChildIVsVerified=false;
    bool originalUnhatchedChildIVsVerified=false;
    bool originalStageIdentityIndependentlyVerified=false;
    // Attests COMPLETE enumeration only of the particular proposed IV
    // stage hypotheses, not all Pokémon origins, parents, gifts, or trades.
    bool allIVStageHypothesesIndependentlyEnumerated=false;
    std::array<Hypothesis,16> histories{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t examined=0,matched=0,contradicted=0,unknown=0;
    std::array<IV::CandidateCheck,16> candidates{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(IV::family(in.game)==IV::Family::Unknown) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(in.count==0)return out;
    if(in.count>in.histories.size()) {
        out.status=Status::CapacityExceeded;return out;
    }
    if(!in.originalPhysicalParentSnapshotsVerified ||
       !in.originalPhysicalSlotOrderVerified ||
       !in.originalInitialChildIVsVerified) {
        out.status=Status::UnknownOriginalParentStage;return out;
    }
    if(!in.originalUnhatchedChildIVsVerified) {
        out.status=Status::UnknownOriginalEggStage;return out;
    }
    if(!in.originalStageIdentityIndependentlyVerified ||
       !in.originalStageIdentity) {
        out.status=Status::IncoherentStageIdentity;return out;
    }
    if(!IV::validIVs(in.initialChild) || !IV::validIVs(in.physicalSlot0) ||
       !IV::validIVs(in.physicalSlot1) || !IV::validIVs(in.observedChild)) {
        out.status=Status::MalformedOriginalIVs;return out;
    }
    // A duplicate cannot establish exhaustive rejection; an otherwise
    // independently verified matching route still supplies a valid witness.
    bool malformedRouteIdentity=false;
    for(uint8_t i=0;i<in.count;++i) {
        ++out.examined;
        const auto& h=in.histories[i];
        if(!h.routeIdentity) {
            malformedRouteIdentity=true;
            ++out.unknown;
            continue;
        }
        bool repeated=false,priorMatch=false;
        for(uint8_t j=0;j<i;++j)
            if(h.routeIdentity==in.histories[j].routeIdentity) {
                repeated=true;
                if(out.candidates[j]==IV::CandidateCheck::Matched)
                    priorMatch=true;
            }
        if(repeated) {
            malformedRouteIdentity=true;
            ++out.unknown;
            if(priorMatch)continue;
        }
        if(!h.independentSourceAttestation ||
           !h.stageIdentity ||
           h.stageIdentity!=in.originalStageIdentity) {
            ++out.unknown;continue;
        }
        IV::KnownFinalStage known{};
        known.game=in.game;
        known.initialChild=in.initialChild;
        known.physicalSlot0=in.physicalSlot0;
        known.physicalSlot1=in.physicalSlot1;
        known.observedChild=in.observedChild;
        known.heldItem0=h.physicalSlot0Item;
        known.heldItem1=h.physicalSlot1Item;
        known.finalSeed=h.finalRNGState;
        known.parentSnapshotsIndependentlyVerified=true;
        known.depositOrderIndependentlyVerified=true;
        known.initialChildIVsIndependentlyVerified=true;
        known.heldItemHistoryIndependentlyVerified=h.historicalHeldItemsKnown;
        known.finalRNGStateIndependentlyVerified=h.finalRNGStateKnown;
        const auto check=IV::verifyKnownFinalStage(known);
        out.candidates[i]=check;
        if(check==IV::CandidateCheck::Matched)++out.matched;
        else if(repeated)continue; // duplicate mismatch cannot exhaust original routes
        else if(check==IV::CandidateCheck::CandidateMismatch)
            ++out.contradicted;
        else ++out.unknown;
    }
    if(out.matched)out.status=Status::ConditionalHistoryMatched;
    else if(malformedRouteIdentity)
        out.status=Status::MissingOrDuplicateRouteIdentity;
    else if(out.unknown)out.status=Status::UnresolvedCandidateHistory;
    else if(!in.allIVStageHypothesesIndependentlyEnumerated)
        out.status=Status::UnknownHistoryEnumeration;
    else out.status=Status::AllEnumeratedStageHypothesesContradicted;
    return out;
}
static_assert(IV::family("diamond_nds")==IV::Family::Unknown,
              "No borrowing Platinum's IV stage for unsupported DP");
} // namespace Legality::Gen4EggIVHistoricalCandidateSet
