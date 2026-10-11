#pragma once
// MOVE-001: combine complete individual candidate-specific four-slot
// method histories WITHOUT treating a failed candidate as global Invalid.
// Even AllGroundedCandidatesContradicted is restricted to an externally
// attested source candidate enumeration and full move-method coverage.
// No default source or arbitrary union of unrelated move histories.
#include "Legality/Gen14CandidateMoveHistory.h"
#include <array>
#include <cstdint>
namespace Legality::Gen14CandidateSetCoverage {
namespace H=Gen14CandidateMoveHistory;
enum class Status : uint8_t {
    NoCandidates, CapacityExceededUnknown, OneConditionalRoute,
    UnknownSourceCandidateEnumeration, UnknownCandidateProvenance,
    UnknownAcquisitionMethods, AllGroundedCandidatesContradicted
};
struct Entry {
    bool originalEncounterIndependentlyGrounded=false;
    H::Candidate candidate{};
    std::array<H::Slot,4> moves{};
};
struct Input {
    bool allOriginalEncounterCandidatesEnumerated=false;
    std::array<Entry,8> alternatives{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,compatible=0,contradicted=0,
        unresolved=0,unverified=0;
    uint8_t maxCoveredSlots=0;
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.count)return out;
    if(in.count>in.alternatives.size()) {
        out.status=Status::CapacityExceededUnknown;return out;
    }
    for(uint8_t i=0;i<in.count;++i) {
        const auto& candidate=in.alternatives[i];
        ++out.considered;
        if(!candidate.originalEncounterIndependentlyGrounded ||
           !candidate.candidate.id) {
            ++out.unverified;continue;
        }
        const auto r=H::evaluate(candidate.candidate,candidate.moves);
        if(r.coveredSlots>out.maxCoveredSlots)
            out.maxCoveredSlots=r.coveredSlots;
        switch(r.status) {
            case H::Status::WitnessesCompatible:
                ++out.compatible;break;
            case H::Status::CandidateContradicted:
                ++out.contradicted;break;
            case H::Status::Empty:
            case H::Status::Unresolved:
                ++out.unresolved;break;
        }
    }
    // A positive, independently proposed path does not require
    // proving that all OTHER histories have been enumerated. It is
    // still only one hypothetical route, not authentic provenance.
    if(out.compatible)out.status=Status::OneConditionalRoute;
    else if(out.unverified)out.status=Status::UnknownCandidateProvenance;
    else if(out.unresolved)out.status=Status::UnknownAcquisitionMethods;
    else if(!in.allOriginalEncounterCandidatesEnumerated)
        out.status=Status::UnknownSourceCandidateEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllGroundedCandidatesContradicted;
    else out.status=Status::UnknownAcquisitionMethods;
    return out;
}
} // namespace Legality::Gen14CandidateSetCoverage
