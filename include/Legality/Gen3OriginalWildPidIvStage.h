#pragma once
// L1 Stage 3: forward-replay ONE independently attested original GBA
// physical capture through species slot, level, nature AND PID/IV stages.
// No reverse-first-solution ambiguity: the original pre-PID seed must be
// independently preserved; current PK3 PID/IV cannot authenticate it.
// Ordinary handheld Method 1/2/3/4 ONLY. GameCube, roamers, events, Unown,
// special Safari Pokeblock and unmodeled lead paths remain separate Unknown.
#include "Legality/Gen3OriginalWildJointHistory.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include <array>
#include <cstdint>
namespace Legality::Gen3OriginalWildPidIvStage {
namespace J=Gen3OriginalWildJointHistory;
namespace P=Gen3PidIv;
enum class Status : uint8_t {
    UnknownOriginalCapture, UnknownIndependentPidIvFrame,
    UnknownDifferentCapturedCreature, UnknownOriginalMethod,
    UnknownMalformedIvs, UnknownOriginalWildStages,
    CandidateOriginalWildStageContradicted, CandidateOriginalPidIvContradicted,
    CandidateOriginalNatureContradicted,
    ConditionalOriginalSourceRngPidIvStagesOnly
};
struct Input {
    J::Candidate sourceAndRng{};
    uint64_t frameCaptureIdentity=0,frameOriginalSaveIdentity=0;
    uint32_t originalPrePidSeed=0,originalPid=0;
    std::array<uint8_t,6> originalIvs{};
    P::Method originalPidIvMethod=P::Method::None;
    bool originalFrameIdentityIndependentlyAttested=false;
    bool originalPidIvSnapshotIndependentlyAttested=false;
    bool originalMethodFamilyIndependentlyAttested=false;
    bool independentOriginalPrePidSeedAttested=false;
    bool sameOriginalCaptureIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalCapture;
    P::Method method=P::Method::None;
    uint32_t frameSeed=0;
    bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool ordinary(P::Method method) noexcept {
    return method==P::Method::Method1 || method==P::Method::Method2 ||
           method==P::Method::Method3 || method==P::Method::Method4;
}
constexpr std::array<uint8_t,6> unpackIvs(uint32_t first,uint32_t second) noexcept {
    return {static_cast<uint8_t>(first&31u),
            static_cast<uint8_t>((first>>5)&31u),
            static_cast<uint8_t>((first>>10)&31u),
            static_cast<uint8_t>(second&31u),
            static_cast<uint8_t>((second>>5)&31u),
            static_cast<uint8_t>((second>>10)&31u)};
}
struct Replay {
    uint32_t pid=0;
    std::array<uint8_t,6> ivs{};
};
constexpr Replay replay(uint32_t seed,P::Method method) noexcept {
    if(!ordinary(method))return {};
    uint32_t state=P::Detail::next(seed);
    const uint32_t pidLo=state>>16;
    if(method==P::Method::Method3)state=P::Detail::next(state);
    state=P::Detail::next(state);
    const uint32_t pid=pidLo|((state>>16)<<16);
    if(method==P::Method::Method2)state=P::Detail::next(state);
    state=P::Detail::next(state);
    const uint32_t first=(state>>16)&0x7fffu;
    if(method==P::Method::Method4)state=P::Detail::next(state);
    state=P::Detail::next(state);
    const uint32_t second=(state>>16)&0x7fffu;
    return {pid,unpackIvs(first,second)};
}
inline Result evaluate(const Input& c) noexcept {
    if(!c.sourceAndRng.physicalCaptureIdentity ||
       !c.sourceAndRng.originalSaveIdentity ||
       !c.originalFrameIdentityIndependentlyAttested)
        return {};
    if(!c.sameOriginalCaptureIndependentlyAttested ||
       c.frameCaptureIdentity!=c.sourceAndRng.physicalCaptureIdentity ||
       c.frameOriginalSaveIdentity!=c.sourceAndRng.originalSaveIdentity)
        return {Status::UnknownDifferentCapturedCreature};
    if(!c.originalPidIvSnapshotIndependentlyAttested ||
       !c.independentOriginalPrePidSeedAttested)
        return {Status::UnknownIndependentPidIvFrame};
    if(!c.originalMethodFamilyIndependentlyAttested || !ordinary(c.originalPidIvMethod))
        return {Status::UnknownOriginalMethod};
    if(!P::validStoredIVs(c.originalIvs))
        return {Status::UnknownMalformedIvs};
    const auto joint=J::evaluate(c.sourceAndRng);
    if(joint.status==J::Status::CandidateOriginalSourceContradicted ||
       joint.status==J::Status::CandidateOriginalRngContradicted)
        return {Status::CandidateOriginalWildStageContradicted};
    if(joint.status!=J::Status::ConditionalSourceAndStageOnly)
        return {Status::UnknownOriginalWildStages};
    const auto generated=replay(c.originalPrePidSeed,c.originalPidIvMethod);
    if(generated.pid!=c.originalPid || generated.ivs!=c.originalIvs)
        return {Status::CandidateOriginalPidIvContradicted};
    // Native Gen III nature comes from PID %25, not from the IV word.
    // A different attested nature seed/lead history cannot be silently
    // spliced into this otherwise matching stored PID/IV capture.
    if(c.originalPid%25u!=joint.nature)
        return {Status::CandidateOriginalNatureContradicted};
    return {Status::ConditionalOriginalSourceRngPidIvStagesOnly,
            c.originalPidIvMethod,c.originalPrePidSeed};
}
} // namespace Legality::Gen3OriginalWildPidIvStage
