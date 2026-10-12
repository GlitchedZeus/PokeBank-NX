#include "Legality/Gen14OriginalHistoryLedger.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace Legality::Gen14OriginalHistoryLedger;
using E=Legality::Gen14OriginalEvidenceBridge::Class;
constexpr Witness mk(uint64_t id,E evidence){
 Witness w{};w.originalSpecimenRoot=12345;w.routeId=id;
 w.originalSaveId=500;w.originalGameId=2;
 w.family=evidence==E::ConditionalNative ? Family::Gen3GameCube : Family::Gen4Sinnoh;
 w.stage=evidence==E::RngStageOnly?Stage::RngNaturePid:
         evidence==E::ConditionalNative?Stage::NativeOriginalIdentity:Stage::PhysicalSource;
 w.evidence=evidence;
 w.rootIdentityIndependentlyAttested=true;
 w.originalGameAndSaveIndependentlyAttested=true;
 w.sourceMethodIndependentlyAttested=true;
 return w;
}
constexpr std::array<E,5> evidence{{
 E::Unknown,E::CandidateLocalContradiction,E::SourceOnly,
 E::RngStageOnly,E::ConditionalNative
}};
int main(){
 uint64_t verified=0;
 // 5^6=15,625 diverse alternative original sources.
 for(unsigned code=0;code<15625;++code){
   std::array<Witness,6> w{};
   unsigned v=code;bool anyUnknown=false,anySuccess=false,allFail=true;
   for(unsigned j=0;j<w.size();++j){
     const auto e=evidence[v%5];v/=5;
     w[j]=mk(j+1,e);
     anyUnknown|=e==E::Unknown;
     anySuccess|=e==E::SourceOnly||e==E::RngStageOnly||e==E::ConditionalNative;
     allFail&=e==E::CandidateLocalContradiction;
   }
   const auto r=summarize(12345,true,w);
   assert(r.noGlobalDecision()&&r.routeCount==6&&r.stageCount==6);
   assert(r.candidateContradicted+r.candidateCompatible+r.candidateUnresolved==6);
   if(anyUnknown)assert(r.verdict==Verdict::UnknownAlternativeHistories);
   if(allFail)assert(r.verdict==Verdict::CandidateLocalContradictionsOnly);
   if(!anyUnknown&&anySuccess)assert(r.verdict==Verdict::ConditionalOriginalCandidateOnly ||
                                     r.verdict==Verdict::SourceOrStageCompatibilityOnly);
   ++verified;
 }
 // A confirmed source stage followed by unknown RNG is still unresolved.
 std::array<Witness,2> same{{mk(1,E::SourceOnly),mk(1,E::Unknown)}};
 same[1].stage=Stage::RngNaturePid;
 const auto s=summarize(12345,true,same);
 assert(s.verdict==Verdict::UnknownAlternativeHistories);
 // The same route cannot silently re-use physical-stage evidence from two
 // contradictory witnesses (or two distinct game identities).
 same[1].stage=Stage::PhysicalSource;
 assert(summarize(12345,true,same).verdict==Verdict::UnknownMalformedInput);
 same[1].stage=Stage::NativeOriginalIdentity;
 same[1].originalSaveId=1;
 assert(summarize(12345,true,same).verdict==Verdict::UnknownMalformedInput);
 std::cout<<"L1 S5 15625 route/evidence alternative permutations and duplicate-stage controls: PASS "<<verified<<"\n";
}
