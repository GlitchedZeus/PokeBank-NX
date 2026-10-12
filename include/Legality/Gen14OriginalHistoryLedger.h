#pragma once
// L1 stage 5: production-safe READ-ONLY diagnostic envelope for Stage 1..4.
// A source-only win, even several compatible source/RNG/native stages, does
// NOT prove a complete acquisition and cannot cause global Invalid/Legal.
// Deterministic, allocation-free, bounded: suitable for a future diagnostic
// UI after coordinator review; no runtime registration into Legality.cpp.
#include "Legality/Gen14OriginalEvidenceBridge.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen14OriginalHistoryLedger {
namespace B=Gen14OriginalEvidenceBridge;
inline constexpr std::size_t kMaxEvidence=256;
inline constexpr std::size_t kMaxRoutes=64;
enum class Family : uint8_t {
 Gen1Wild, Gen2Wild, Gen2Fishing, Gen2TreeContest, Gen2Swarm,
 Gen2Roamer, Gen3Wild, Gen3Safari, Gen3Roamer, Gen3GameCube,
 Gen4Sinnoh, Gen4Hgss, EggGiftTradeEventTransfer, OtherUnmodeled, Count
};
enum class Stage : uint8_t { PhysicalSource, OriginalWorld, RngNaturePid,
                             NativeOriginalIdentity, Other, Count };
enum class Verdict : uint8_t {
 UnknownMissingRoot, UnknownMalformedInput, UnknownCapacity,
 UnknownAlternativeHistories, CandidateLocalContradictionsOnly,
 SourceOrStageCompatibilityOnly, ConditionalOriginalCandidateOnly
};
// One separately sourced observation. A route may have multiple stage rows,
// but two rows claiming the same stage, original game, and route are ambiguous.
// The source game and specimen root are opaque identifiers, not current PK3/PK4
// version or a transfer-chain shortcut. No inference is done on 0 defaults.
struct Witness {
 uint64_t originalSpecimenRoot=0,routeId=0,originalSaveId=0;
 uint32_t originalGameId=0;
 Family family=Family::OtherUnmodeled;
 Stage stage=Stage::Other;
 B::Class evidence=B::Class::Unknown;
 bool rootIdentityIndependentlyAttested=false;
 bool originalGameAndSaveIndependentlyAttested=false;
 bool sourceMethodIndependentlyAttested=false;
};
struct RouteSummary {
 uint64_t routeId=0;
 Family family=Family::OtherUnmodeled;
 uint32_t originalGameId=0;
 uint64_t originalSaveId=0;
 uint8_t sourceStages=0,rngStages=0,nativeStages=0,contradictions=0,unknownStages=0;
 bool candidateContradicted=false;
};
struct Summary {
 Verdict verdict=Verdict::UnknownMissingRoot;
 uint16_t stageCount=0,routeCount=0;
 uint16_t sourceOnly=0,rngOnly=0,nativeConditional=0,localContradictions=0,unknown=0;
 uint16_t candidateCompatible=0,candidateContradicted=0,candidateUnresolved=0;
 bool globalInvalidSupported=false;
 bool globallyLegalSupported=false;
 // No caller can make these true: complete source histories and provenance
 // are not provided by this bounded diagnostic input type.
 constexpr bool noGlobalDecision() const noexcept {
   return !globalInvalidSupported && !globallyLegalSupported;
 }
};
constexpr bool validFamily(Family f) noexcept {return f<Family::Count;}
constexpr bool validStage(Stage s) noexcept {return s<Stage::Count;}
constexpr bool validClass(B::Class k) noexcept {return k<=B::Class::ConditionalNative;}
constexpr bool isUnmodeled(Family f) noexcept {
 return f==Family::EggGiftTradeEventTransfer||f==Family::OtherUnmodeled;
}
constexpr bool stageClassCoherent(Family f,Stage s,B::Class k) noexcept {
 // A stage-only match must never be relabeled "native conditional" and a
 // physical encounter row must never be represented as a PID/RNG proof.
 if(k==B::Class::SourceOnly)return s==Stage::PhysicalSource||s==Stage::OriginalWorld;
 if(k==B::Class::RngStageOnly)return s==Stage::RngNaturePid &&
    (f==Family::Gen3Wild || f==Family::Gen3Safari || f==Family::Gen3Roamer ||
     f==Family::Gen3GameCube || f==Family::Gen4Sinnoh || f==Family::Gen4Hgss);
 if(k==B::Class::ConditionalNative)return s==Stage::NativeOriginalIdentity &&
    f==Family::Gen3GameCube;
 return true;
}
// The root ID is independently documented as one original specimen, not
// derived from e.g. matching modern species or PID. Different original
// possible games *are permitted* on separate alternate candidate routes.
inline Summary summarize(uint64_t root,bool independentlyAttestedRoot,
                         std::span<const Witness> stages) noexcept {
 Summary out{};
 if(!root||!independentlyAttestedRoot)return out;
 if(stages.size()>kMaxEvidence){out.verdict=Verdict::UnknownCapacity;return out;}
 if(stages.empty()){out.verdict=Verdict::UnknownAlternativeHistories;return out;}
 std::array<RouteSummary,kMaxRoutes> routes{};
 std::array<uint8_t,kMaxRoutes> stageMask{};
 bool malformed=false,anyUnmodeled=false,anyUnknown=false;
 for(const Witness& w:stages){
   if(w.originalSpecimenRoot!=root || !w.rootIdentityIndependentlyAttested ||
      !w.routeId || !w.originalSaveId || !w.originalGameId ||
      !w.originalGameAndSaveIndependentlyAttested ||
      !w.sourceMethodIndependentlyAttested ||
      !validFamily(w.family) || !validStage(w.stage) || !validClass(w.evidence) ||
      !stageClassCoherent(w.family,w.stage,w.evidence)) {
      malformed=true;break;
   }
   std::size_t r=0;
   while(r<out.routeCount&&routes[r].routeId!=w.routeId)++r;
   if(r==out.routeCount){
     if(r>=kMaxRoutes){out.verdict=Verdict::UnknownCapacity;return out;}
     routes[r].routeId=w.routeId;routes[r].family=w.family;
     routes[r].originalGameId=w.originalGameId;
     routes[r].originalSaveId=w.originalSaveId;
     ++out.routeCount;
   }else if(routes[r].family!=w.family ||
            routes[r].originalGameId!=w.originalGameId ||
            routes[r].originalSaveId!=w.originalSaveId){
      malformed=true;break;
   }
   const uint8_t bit=static_cast<uint8_t>(1u<<static_cast<uint8_t>(w.stage));
   if(stageMask[r]&bit){malformed=true;break;}
   stageMask[r]=static_cast<uint8_t>(stageMask[r]|bit);
   auto& route=routes[r];++out.stageCount;
   if(isUnmodeled(w.family))anyUnmodeled=true;
   switch(w.evidence){
    case B::Class::CandidateLocalContradiction:
      ++route.contradictions;++out.localContradictions;break;
    case B::Class::SourceOnly:
      ++route.sourceStages;++out.sourceOnly;break;
    case B::Class::RngStageOnly:
      ++route.rngStages;++out.rngOnly;break;
    case B::Class::ConditionalNative:
      ++route.nativeStages;++out.nativeConditional;break;
    case B::Class::Unknown:
      ++route.unknownStages;++out.unknown;anyUnknown=true;break;
   }
 }
 if(malformed){out.verdict=Verdict::UnknownMalformedInput;return out;}
 bool sourceOrStage=false,nativeCompatible=false;
 for(std::size_t i=0;i<out.routeCount;++i){
   const auto& route=routes[i];
   if(route.contradictions){++out.candidateContradicted;continue;}
   if(route.unknownStages||isUnmodeled(route.family)){
     ++out.candidateUnresolved;anyUnknown=true;continue;
   }
   if(route.sourceStages||route.rngStages||route.nativeStages){
     ++out.candidateCompatible;
     if(route.nativeStages)nativeCompatible=true;
     else sourceOrStage=true;
   }else{++out.candidateUnresolved;anyUnknown=true;}
 }
 // Never misrepresent the absence of listed routes as an exhaustive search.
 if(anyUnknown||anyUnmodeled||out.candidateUnresolved)
   out.verdict=Verdict::UnknownAlternativeHistories;
 else if(nativeCompatible)
   out.verdict=Verdict::ConditionalOriginalCandidateOnly;
 else if(sourceOrStage)
   out.verdict=Verdict::SourceOrStageCompatibilityOnly;
 else if(out.candidateContradicted)
   out.verdict=Verdict::CandidateLocalContradictionsOnly;
 else out.verdict=Verdict::UnknownAlternativeHistories;
 return out;
}
// Tiny typed adapter: never accept a numerically cast arbitrary status as
// an evidence class; enum mapping is explicit, nonmatching -> Unknown.
template<class SourceStatus>
constexpr Witness withEvidence(Witness w,SourceStatus s) noexcept {
 w.evidence=B::map(s);return w;
}
} // namespace Legality::Gen14OriginalHistoryLedger
