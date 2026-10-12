#pragma once
// BREED-004: inspect an attested, bounded set of ALTERNATIVE Gen IV
// parent/daycare histories for ONE original unhatched PK4. Contradictions
// never prove globally Invalid: even an explicitly exhaustive daycare
// candidate set does not enumerate non-daycare origins or lost archives.
#include "Legality/Gen4JointEggCandidateEvidence.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4EggCandidateUniverse {
namespace J=Gen4JointEggCandidateEvidence;
namespace N=Gen4NativeBreedingParentEvidence;
enum class Status : uint8_t {
    NoCandidates, CapacityExceededUnknown, UnsupportedGame,
    UnknownOriginalEggSource, UnknownCandidateIdentity,
    OneConditionalHistory, UnknownCandidateProvenance,
    UnknownCandidateEvidence, UnknownCandidateEnumeration,
    AllAttestedDaycareHistoriesContradicted
};
struct Entry {
    // Unique independently identified candidate stage/parent-pair hypothesis
    // within this evidence set, NOT a hash derived from child PID alone.
    uint64_t routeIdentity=0;
    bool hypothesisIndependentlyGrounded=false;
    // Required to count a contradiction; positives need not enumerate every
    // alternate move/IV acquisition history.
    bool fullCandidateMethodCoverageIndependentlyVerified=false;
    J::Input history{};
    J::NativeEggWitness originalEgg{};
};
struct Input {
    std::string_view game{};
    const Pokemon::Pokemon4ReadOnly* originalUnhatchedEgg=nullptr;
    bool originalEggSourceIndependentlyVerified=false;
    bool allDaycareHypothesesIndependentlyEnumerated=false;
    std::array<Entry,8> alternatives{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,consistent=0,contradicted=0,
            unresolved=0,unverified=0;
    std::array<J::Status,8> candidateStatuses{};
};
constexpr bool localContradiction(J::Status s) noexcept {
    switch(s) {
        case J::Status::IncoherentNativeEggRecord:
        case J::Status::CandidatePairIncompatible:
        case J::Status::IncoherentParentIdentity:
        case J::Status::IncoherentMoveSnapshot:
        case J::Status::IncoherentSharedStage:
        case J::Status::MoveCandidateContradicted:
        case J::Status::IVCandidateContradicted:
            return true;
        default:return false;
    }
}
static_assert(localContradiction(J::Status::IVCandidateContradicted));
static_assert(!localContradiction(J::Status::UnknownNativeEggRecord));
static_assert(!localContradiction(J::Status::UnknownFinalRNGState));
static_assert(!localContradiction(J::Status::MalformedIVDomain));
inline bool sameOriginalEncryptedEgg(const Pokemon::Pokemon4ReadOnly& a,
                                     const Pokemon::Pokemon4ReadOnly& b) noexcept {
    const auto first=a.originalEncryptedBytes();
    const auto second=b.originalEncryptedBytes();
    return first.size()==second.size() &&
           std::equal(first.begin(),first.end(),second.begin());
}
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    if(J::IV::family(in.game)==J::IV::Family::Unknown) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.count)return out;
    if(in.count>in.alternatives.size()) {
        out.status=Status::CapacityExceededUnknown;return out;
    }
    // A detached native PK4 proves neither the original egg-stage
    // container nor historical chronology: both remain supplied proof.
    if(!in.originalEggSourceIndependentlyVerified ||
       !in.originalUnhatchedEgg ||
       !in.originalUnhatchedEgg->valid() ||
       in.originalUnhatchedEgg->empty() ||
       !in.originalUnhatchedEgg->isEgg() ||
       in.originalUnhatchedEgg->sourceGroup()!=N::requiredGroup(in.game)) {
        out.status=Status::UnknownOriginalEggSource;return out;
    }
    // Duplicate/zero archive identifiers cannot count as independently
    // enumerated candidate histories; do not double-count contradictions.
    for(uint8_t i=0;i<in.count;++i) {
        if(!in.alternatives[i].routeIdentity) {
            out.status=Status::UnknownCandidateIdentity;return out;
        }
        for(uint8_t j=0;j<i;++j) {
            if(in.alternatives[i].routeIdentity==
               in.alternatives[j].routeIdentity) {
                out.status=Status::UnknownCandidateIdentity;return out;
            }
        }
    }
    for(uint8_t i=0;i<in.count;++i) {
        ++out.considered;
        const auto& a=in.alternatives[i];
        if(!a.hypothesisIndependentlyGrounded ||
           a.history.parents.game!=in.game ||
           !a.originalEgg.record ||
           !a.originalEgg.record->valid() ||
           !sameOriginalEncryptedEgg(*in.originalUnhatchedEgg,
                                     *a.originalEgg.record)) {
            ++out.unverified;continue;
        }
        const auto assessed=J::evaluateWithNativeEgg(a.history,a.originalEgg);
        out.candidateStatuses[i]=assessed.status;
        if(assessed.status==J::Status::KnownCandidateConsistent) {
            ++out.consistent;continue;
        }
        if(localContradiction(assessed.status) &&
           a.fullCandidateMethodCoverageIndependentlyVerified) {
            ++out.contradicted;continue;
        }
        ++out.unresolved;
    }
    // Any one conditional path is useful WITHOUT exhaustively proving
    // every alternative. Neither it nor aggregate contradiction is
    // a whole-Pokémon legality verdict.
    if(out.consistent)out.status=Status::OneConditionalHistory;
    else if(out.unverified)out.status=Status::UnknownCandidateProvenance;
    else if(out.unresolved)out.status=Status::UnknownCandidateEvidence;
    else if(!in.allDaycareHypothesesIndependentlyEnumerated)
        out.status=Status::UnknownCandidateEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllAttestedDaycareHistoriesContradicted;
    else out.status=Status::UnknownCandidateEvidence;
    return out;
}
} // namespace Legality::Gen4EggCandidateUniverse
