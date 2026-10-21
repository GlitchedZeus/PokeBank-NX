// SPDX-License-Identifier: GPL-3.0-or-later
// L1 Phase A: verbatim constants and arithmetic from source-pinned PKHeX.Core
// Legality/RNG/Algorithms/LCRNG64.cs, MonochromeRNG.cs
// (kwsch/PKHeX @ 66ef5983a5d349efa2dd615f2926a1b1ee97306e,
// Git blobs e7069e285264dc18e3eaf8942803156b8f101547,
// 12ad6a53add1509e8a642b8c501fd42c1390da11).
// GPLv3-or-later notices/provenance retained. This is NOT a complete B/W
// encounter RNG generator: original seed, starting frame, nature/gender,
// hidden ability, encounters/event PID mutations remain unverified.
#pragma once
#include <cstdint>
namespace Legality::Gen5PkhexLcrngStage {
inline constexpr uint64_t kMult=0x5D588B656C078965ULL;
inline constexpr uint64_t kAdd=0x0000000000269EC3ULL;
inline constexpr uint64_t kReverseMult=0xDEDCEDAE9638806DULL;
inline constexpr uint64_t kReverseAdd=0x9B1AE6E9A384E6F9ULL;
constexpr uint64_t next(uint64_t seed) noexcept {
 return seed*kMult+kAdd;
}
constexpr uint64_t prev(uint64_t seed) noexcept {
 return seed*kReverseMult+kReverseAdd;
}
constexpr uint32_t next32(uint64_t& seed) noexcept {
 seed=next(seed);return static_cast<uint32_t>(seed>>32);
}
constexpr uint32_t trainerBitXor(uint16_t tid,uint16_t sid) noexcept {
 return (static_cast<uint32_t>(tid)^sid)&1u;
}
constexpr uint32_t wildXor(uint32_t pid,uint16_t tid,uint16_t sid) noexcept {
 const uint32_t corr=(pid>>31)^(pid&1u)^trainerBitXor(tid,sid);
 return corr&1u ? pid^0x80000000u : pid;
}
enum class Status : uint8_t {
 UnknownOriginalGame, UnknownOriginalCapture, UnknownOriginalSeed,
 UnknownOriginalPID, UnknownTrainer, UnknownMutationSteps,
 UnknownSearchBound, CandidateLocalRawPidMismatch,
 ConditionalRawSeedPidStageOnly
};
struct Evidence {
 uint64_t originalSaveIdentity=0,originalCreatureIdentity=0,originalSeed=0;
 bool independentlyAttestedPhysicalOriginal=false;
 bool independentlyAttestedOriginalGame=false;
 uint8_t originalGameVersion=0; // PKHeX GameVersion: W20 B21 W2=22 B2=23
 bool independentlyAttestedOriginalSeed=false;
 bool independentlyAttestedOriginalPid=false;
 bool independentlyAttestedOriginalTrainer=false;
 bool independentlyAttestedWildXorRule=false;
 bool exactOriginalAdvanceIndependentlyAttested=false;
 uint16_t tid=0,sid=0;
 uint32_t originalPid=0;
 uint16_t advance=0,searchFrames=0;
 bool applyWildXor=false;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint16_t checkedFrames=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
inline Result evaluate(const Evidence& e) noexcept {
 if(!e.independentlyAttestedOriginalGame||e.originalGameVersion<20||
    e.originalGameVersion>23)return {Status::UnknownOriginalGame};
 if(!e.originalSaveIdentity||!e.originalCreatureIdentity||
    !e.independentlyAttestedPhysicalOriginal)
    return {Status::UnknownOriginalCapture};
 if(!e.independentlyAttestedOriginalSeed)return {Status::UnknownOriginalSeed};
 if(!e.independentlyAttestedOriginalPid)return {Status::UnknownOriginalPID};
 if(e.applyWildXor&&(!e.independentlyAttestedOriginalTrainer||
                     !e.independentlyAttestedWildXorRule))
    return {Status::UnknownTrainer};
 if(e.searchFrames==0||e.searchFrames>4096 ||
    (e.exactOriginalAdvanceIndependentlyAttested &&
     e.advance>=e.searchFrames))
    return {Status::UnknownSearchBound};
 uint64_t seed=e.originalSeed;
 for(uint16_t i=0;i<e.searchFrames;++i){
    const uint32_t raw=next32(seed);
    if(e.exactOriginalAdvanceIndependentlyAttested && i!=e.advance)
       continue;
    const uint32_t pid=e.applyWildXor?wildXor(raw,e.tid,e.sid):raw;
    if(pid==e.originalPid)
       return {Status::ConditionalRawSeedPidStageOnly,
               static_cast<uint16_t>(i+1)};
 }
 if(e.exactOriginalAdvanceIndependentlyAttested)
    return {Status::CandidateLocalRawPidMismatch,e.searchFrames};
 return {Status::UnknownSearchBound,e.searchFrames};
}
static_assert(prev(next(0ULL))==0ULL);
static_assert(prev(next(0x123456789ABCDEF0ULL))==0x123456789ABCDEF0ULL);
static_assert(next(0ULL)==kAdd);
} // namespace Legality::Gen5PkhexLcrngStage
