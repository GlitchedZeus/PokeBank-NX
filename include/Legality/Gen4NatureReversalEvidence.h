#pragma once
// RNG-002: bounded Method J/K nature-rejection reversal with an explicit
// EXHAUSTED state. An RNG-stage match is not a complete wild encounter
// witness. Exhaustion is NOT a proven RNG mismatch or global Invalid.
#include "Legality/Gen3PidIvCorrelation.h"
#include <cstdint>

namespace Legality::Gen4NatureReversal {
enum class Status : uint8_t { Matched, SearchExhausted, InvalidNature };
struct Result {
    Status status=Status::SearchExhausted;
    uint16_t rejectedPidPairs=0;
    uint16_t inspectedPidPairs=0;
    constexpr bool matched() const noexcept { return status==Status::Matched; }
    constexpr bool exhausted() const noexcept {
        return status==Status::SearchExhausted;
    }
};
// The retail nature-rejection loop does not impose a 128-PID bound. The
// budget below limits *our evidence search only*; reaching it leaves the
// historical sequence unresolved rather than proving incompatibility.
constexpr Result search(uint32_t prePidSeed,uint8_t nature,
                        uint16_t budget=128) noexcept {
    if(nature>=25) return {Status::InvalidNature,0,0};
    uint32_t upper=prePidSeed>>16;
    for(uint16_t count=0;count<budget;++count) {
        prePidSeed=Gen3PidIv::Detail::prev(prePidSeed);
        const uint32_t lower=prePidSeed>>16;
        const uint32_t pid=(upper<<16)|lower;
        if(pid%25u==nature)
            return {Status::Matched,count,
                    static_cast<uint16_t>(count+1u)};
        prePidSeed=Gen3PidIv::Detail::prev(prePidSeed);
        upper=prePidSeed>>16;
    }
    return {Status::SearchExhausted,0,budget};
}
// RNG-002: a source-attested pre-PID seed should also reproduce the
// observed retained PID. A reverse nature rejection match by itself is
// not enough to identify an actual caught specimen or original RNG frame.
enum class CandidateStatus : uint8_t {
    UnknownOriginalStage, InvalidNature, IncompatibleRetainedPid,
    SearchExhausted, CandidateConsistent
};
struct Candidate {
    bool independentlyKnownPrePidState=false;
    uint32_t prePidState=0, observedRetainedPid=0;
    uint8_t targetNature=0;
    uint16_t pairBudget=128;
};
struct CandidateResult {
    CandidateStatus status=CandidateStatus::UnknownOriginalStage;
    Result reverse{};
};
constexpr uint32_t retainedPidFromStage(uint32_t state) noexcept {
    state=Gen3PidIv::Detail::next(state);
    const uint32_t low=state>>16;
    state=Gen3PidIv::Detail::next(state);
    return ((state>>16)<<16)|low;
}
constexpr CandidateResult verifyKnownCandidate(const Candidate& in) noexcept {
    if(!in.independentlyKnownPrePidState)return {};
    if(in.targetNature>=25)
        return {CandidateStatus::InvalidNature,{}};
    if(retainedPidFromStage(in.prePidState)!=in.observedRetainedPid ||
       in.observedRetainedPid%25u!=in.targetNature)
        return {CandidateStatus::IncompatibleRetainedPid,{}};
    const auto result=search(in.prePidState,in.targetNature,in.pairBudget);
    return {
        result.status==Status::Matched ?
            CandidateStatus::CandidateConsistent:
            CandidateStatus::SearchExhausted,
        result
    };
}
} // namespace Legality::Gen4NatureReversal
