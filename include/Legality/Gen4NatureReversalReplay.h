#pragma once
// L1 / E15: source-attested, forward Method J/K *no-lead nature/PID stage*.
// It is not a whole-encounter, lead, slot, event or PK4 authenticity proof.
// Unlike the legacy 128-pair reverse matcher, a bounded miss is explicitly
// SearchExhausted, never negative evidence about all historical routes.
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen4NatureReversalEvidence.h"
#include <cstdint>
namespace Legality::Gen4NatureReversalReplay {
enum class Method : uint8_t { Unknown, J, K };
enum class Status : uint8_t {
    UnknownHistoricalStage, UnsupportedMethod, InvalidTargetNature,
    SearchExhausted, AttestedStagePidContradicted, AttestedStageMatched
};
struct Candidate {
    Method method=Method::Unknown;
    bool originalNatureStateIndependentlyAttested=false;
    bool originalPidSnapshotIndependentlyAttested=false;
    uint32_t preNatureState=0;
    uint32_t recordedPid=0;
    uint8_t targetNature=0;
    uint32_t maxPidPairs=128;
};
struct Result {
    Status status=Status::UnknownHistoricalStage;
    uint32_t rejectedPairs=0;
    uint32_t inspectedPairs=0;
    uint32_t acceptedPid=0;
    uint32_t preAcceptedPidState=0;
    constexpr bool matched() const noexcept {
        return status==Status::AttestedStageMatched;
    }
};
// HGSS / CreateMonWithNature: nature = upper16(LCRNG()) % 25;
// DP/Pt Method J: nature = upper16(LCRNG()) / 0xA3E.
// Each rejected PID pair consumes two additional LCRNG calls. Retail has no
// cap of 128 rejected pairs; budget is a local resource limit only.
constexpr Result replay(const Candidate& c) noexcept {
    if(!c.originalNatureStateIndependentlyAttested ||
       !c.originalPidSnapshotIndependentlyAttested) return {};
    if(c.method==Method::Unknown) return {Status::UnsupportedMethod};
    if(c.targetNature>=25) return {Status::InvalidTargetNature};
    uint32_t state=Gen3PidIv::Detail::next(c.preNatureState);
    const uint32_t half=state>>16;
    const uint32_t nature=c.method==Method::K ?
        half%25u : half/0x0A3Eu;
    if(nature!=c.targetNature)
        return {Status::AttestedStagePidContradicted};
    for(uint32_t i=0;i<c.maxPidPairs;++i) {
        const uint32_t before=state;
        state=Gen3PidIv::Detail::next(state);
        const uint32_t lo=state>>16;
        state=Gen3PidIv::Detail::next(state);
        const uint32_t pid=(state&0xFFFF0000u)|lo;
        if(pid%25u != nature) continue;
        return {pid==c.recordedPid ?
                    Status::AttestedStageMatched :
                    Status::AttestedStagePidContradicted,
                i,i+1u,pid,before};
    }
    return {Status::SearchExhausted,c.maxPidPairs,c.maxPidPairs};
}
} // namespace Legality::Gen4NatureReversalReplay
