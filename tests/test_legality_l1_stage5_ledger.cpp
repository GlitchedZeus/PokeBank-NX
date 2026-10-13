#include "Legality/Gen14OriginalHistoryLedger.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
using namespace Legality::Gen14OriginalHistoryLedger;
using E=Legality::Gen14OriginalEvidenceBridge::Class;
constexpr Witness base(uint64_t route=1,Family f=Family::Gen3Wild,Stage s=Stage::PhysicalSource,
                       E k=E::SourceOnly) {
 Witness w{};w.originalSpecimenRoot=0xAAAABBBB;
 w.routeId=route;w.originalSaveId=10;w.originalGameId=3;
 w.family=f;w.stage=s;w.evidence=k;
 w.rootIdentityIndependentlyAttested=true;
 w.originalGameAndSaveIndependentlyAttested=true;
 w.sourceMethodIndependentlyAttested=true;
 return w;
}
template<std::size_t N> Summary go(const std::array<Witness,N>& a) {
 return summarize(0xAAAABBBB,true,a);
}
int main(){
 constexpr std::array<Witness,1> positive{{base()}};
 auto p=go(positive);
 assert(p.verdict==Verdict::SourceOrStageCompatibilityOnly);
 assert(p.candidateCompatible==1&&p.sourceOnly==1&&p.noGlobalDecision());
 auto local=positive;
 local[0].evidence=E::CandidateLocalContradiction;
 assert(go(local).verdict==Verdict::CandidateLocalContradictionsOnly);
 auto unknown=positive;unknown[0].evidence=E::Unknown;
 assert(go(unknown).verdict==Verdict::UnknownAlternativeHistories);
 assert(summarize(0,false,positive).verdict==Verdict::UnknownMissingRoot);
 assert(summarize(0xAAAABBBB,false,positive).verdict==Verdict::UnknownMissingRoot);
 const std::array<Witness,0> empty{};
 assert(go(empty).verdict==Verdict::UnknownAlternativeHistories);
 // A deliberate competing event/egg/transfer source must stay unknown,
 // even when every *enumerated* wild candidate locally contradicted.
 const std::array<Witness,3> competing{{
  base(1,Family::Gen1Wild,Stage::PhysicalSource,E::CandidateLocalContradiction),
  base(2,Family::Gen3Safari,Stage::RngNaturePid,E::CandidateLocalContradiction),
  base(3,Family::EggGiftTradeEventTransfer,Stage::Other,E::Unknown)
 }};
 auto m=go(competing);
 assert(m.candidateContradicted==2&&m.candidateUnresolved==1);
 assert(m.verdict==Verdict::UnknownAlternativeHistories&&m.noGlobalDecision());
 // Independent stage witnesses from a single attested, same-root candidate:
 // never fuse source and RNG into a global Legal verdict.
 const std::array<Witness,3> bundle{{
   base(5,Family::Gen3Wild,Stage::PhysicalSource,E::SourceOnly),
   base(5,Family::Gen3Wild,Stage::RngNaturePid,E::RngStageOnly),
   base(6,Family::Gen3GameCube,Stage::NativeOriginalIdentity,E::ConditionalNative)
 }};
 auto b=go(bundle);
 assert(b.verdict==Verdict::ConditionalOriginalCandidateOnly);
 assert(b.candidateCompatible==2 && b.noGlobalDecision());
 // New historically grounded negative for one stage cannot exclude another
 // competing physical row, different original game or original source family.
 auto alternative=bundle;
 alternative[1].evidence=E::CandidateLocalContradiction;
 auto partial=go(alternative);
 assert(partial.candidateContradicted==1&&partial.candidateCompatible==1);
 assert(partial.verdict==Verdict::ConditionalOriginalCandidateOnly);
 alternative[2].evidence=E::CandidateLocalContradiction;
 assert(go(alternative).verdict==Verdict::CandidateLocalContradictionsOnly);
 // Identity tampering cannot be interpreted as negative legality evidence.
 auto corrupted=bundle;
 corrupted[0].originalSpecimenRoot=123;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].routeId=0;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].originalSaveId=0;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].originalGameId=0;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].sourceMethodIndependentlyAttested=false;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].rootIdentityIndependentlyAttested=false;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].originalGameAndSaveIndependentlyAttested=false;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].family=static_cast<Family>(255);
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].stage=static_cast<Stage>(255);
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].evidence=static_cast<E>(255);
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[0].evidence=E::RngStageOnly;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[2].family=Family::Gen4Sinnoh;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[1].family=Family::Gen2Fishing;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[1].originalGameId=99;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[1].originalSaveId=99;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[1].family=Family::Gen2Wild;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 corrupted=bundle;corrupted[1].stage=Stage::PhysicalSource;
 assert(go(corrupted).verdict==Verdict::UnknownMalformedInput);
 // Capacity: 256 bounded observations, max 64 independently known routes.
 std::array<Witness,256> full{};
 for(std::size_t i=0;i<full.size();++i){
   full[i]=base(i/4+1,Family::Gen3Wild,static_cast<Stage>(i%4),E::Unknown);
 }
 auto f=go(full);
 assert(f.routeCount==64&&f.stageCount==256&&f.verdict==Verdict::UnknownAlternativeHistories);
 std::array<Witness,257> tooMany{};
 for(auto& v:tooMany)v=base();
 assert(go(tooMany).verdict==Verdict::UnknownCapacity);
 std::array<Witness,65> routes{};
 for(std::size_t i=0;i<routes.size();++i)routes[i]=base(i+1);
 assert(go(routes).verdict==Verdict::UnknownCapacity);
 std::cout<<"L1 S5 bounded 256-witness / 64-route ledger, provenance/adversarial controls: PASS\n";
}
