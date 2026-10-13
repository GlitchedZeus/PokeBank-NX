#include "Legality/Gen3OriginalWildRngWitness.h"
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <iostream>
int main(){
 namespace H=Legality::Gen3OriginalWildRngWitness; namespace W=Legality::Gen3OriginalWildSourceStage;
 using S=H::Status;
 static_assert(H::next(0)==24691u);
 static_assert(H::slotFromRoll(W::Method::Grass,W::Rod::Unknown,0)==0u);
 static_assert(H::slotFromRoll(W::Method::Grass,W::Rod::Unknown,99)==11u);
 static_assert(H::slotFromRoll(W::Method::Surf,W::Rod::Unknown,90)==2u);
 static_assert(H::slotFromRoll(W::Method::Fishing,W::Rod::Old,99)==1u);
 static_assert(H::slotFromRoll(W::Method::Fishing,W::Rod::Good,99)==4u);
 static_assert(H::slotFromRoll(W::Method::Fishing,W::Rod::Super,99)==9u);
 std::size_t accepted=0,negative=0,unknown=0;
 for(std::size_t i=0;i<W::kGroups.size();++i){
  const auto& g=W::kGroups[i];
  for(uint32_t x=0;x<128;++x){
   H::Stage st{}; st.exactOriginalGame=g.game;st.exactGroupIndex=i;
   st.rod=g.method==W::Method::Fishing?W::Rod::Super:W::Rod::Unknown;
   st.originalLead=H::Lead::NoLead;
   st.preSlotSeed=x*76543u+1234u;st.preNatureSeed=x*9239u+5u;
   st.originalGameAndGroupAttested=st.originalSlotSeedAttested=true;
   st.originalNatureSeedAttested=st.originalSlotAndLevelAttested=true;
   st.originalLeadAttested=st.originalNatureAttested=true;
   st.originalSafariContextKnown=st.originalSafariPokeblockPathRuledOut=true;
   const uint16_t a=static_cast<uint16_t>(((uint64_t(st.preSlotSeed)*1103515245u+24691u)&0xffffffffu)>>16);
   // Independently computed reference arithmetic; no call to the matcher
   // when constructing either original-seed stage fixture.
   uint8_t slot=255;
   if(g.method==W::Method::Grass){
    constexpr unsigned cum[]{20,40,50,60,70,80,85,90,94,98,99,100};
    for(unsigned j=0;j<12;++j)if((a%100)<cum[j]){slot=j;break;}
   }else if(g.method==W::Method::Surf){
    constexpr unsigned cum[]{60,90,95,99,100};
    for(unsigned j=0;j<5;++j)if((a%100)<cum[j]){slot=j;break;}
   }else if(g.method==W::Method::Fishing){
    constexpr unsigned cum[]{40,80,95,99,100};
    for(unsigned j=0;j<5;++j)if((a%100)<cum[j]){slot=static_cast<uint8_t>(j+5);break;}
   }
   assert(slot<W::slotCount(g));const auto row=W::decode(g,slot);
   const auto b=static_cast<uint32_t>((uint64_t(st.preSlotSeed)*1103515245u+24691u)&0xffffffffu);
   const auto c=static_cast<uint32_t>((uint64_t(b)*1103515245u+24691u)&0xffffffffu);
   st.attestedSlot=slot;st.attestedLevel=static_cast<uint8_t>(row.minLevel+((c>>16)%(row.maxLevel-row.minLevel+1)));
   const auto d=static_cast<uint32_t>((uint64_t(st.preNatureSeed)*1103515245u+24691u)&0xffffffffu);
   st.attestedNature=static_cast<uint8_t>((d>>16)%25u);
   assert(H::evaluate(st).status==S::StageOnlyCompatibleHistoryUnverified);++accepted;
   auto bad=st;bad.attestedSlot=static_cast<uint8_t>((slot+1)%W::slotCount(g));
   assert(H::evaluate(bad).status==S::CandidateSlotContradicted);++negative;
   bad=st;bad.attestedLevel=static_cast<uint8_t>(st.attestedLevel==100?99:st.attestedLevel+1);
   assert(H::evaluate(bad).status==S::CandidateLevelContradicted);++negative;
   bad=st;bad.attestedNature=static_cast<uint8_t>((st.attestedNature+1)%25);
   assert(H::evaluate(bad).status==S::CandidateNatureContradicted);++negative;
   bad=st;bad.originalSlotSeedAttested=false;
   assert(H::evaluate(bad).status==S::UnknownOriginalStage);++unknown;
   bad=st;bad.originalLeadAttested=false;
   assert(H::evaluate(bad).status==S::UnknownOriginalLead);++unknown;
   bad=st;bad.originalSafariContextKnown=false;
   assert(H::evaluate(bad).status==S::UnknownOriginalStage);++unknown;
   bad=st;bad.originalInSafari=true;bad.originalSafariPokeblockPathRuledOut=false;
   assert(H::evaluate(bad).status==S::UnknownSafariPokeblock);++unknown;
   bad=st;bad.originalLead=H::Lead::Pressure;
   if(g.game!=W::Game::Emerald){assert(H::evaluate(bad).status==S::UnknownOrUnsupportedLeadPath);++unknown;}
  }
 }
 // Emerald successful Synchronize, failed Synchronize, and Pressure/Hustle/Vital Spirit.
 for(auto lead : {H::Lead::Synchronize,H::Lead::Pressure,H::Lead::Hustle,H::Lead::VitalSpirit}){
  H::Stage s{};s.exactOriginalGame=W::Game::Emerald;s.exactGroupIndex=18; // Route119 Grass
  s.originalLead=lead;s.preSlotSeed=151u;s.preNatureSeed=331u;
  s.originalGameAndGroupAttested=s.originalSlotSeedAttested=true;
  s.originalNatureSeedAttested=s.originalSlotAndLevelAttested=true;
  s.originalLeadAttested=s.originalNatureAttested=true;
  s.originalSafariContextKnown=s.originalSafariPokeblockPathRuledOut=true;
  s.originalSyncNatureAttested=true;s.originalSyncNature=12;
  const auto& g=W::kGroups[s.exactGroupIndex];
  const uint32_t r1=H::next(s.preSlotSeed);
  s.attestedSlot=H::slotFromRoll(g.method,s.rod,H::sample(r1));
  const auto row=W::decode(g,s.attestedSlot);
  uint32_t state=H::next(r1);
  uint8_t roll=static_cast<uint8_t>(H::sample(state)%(row.maxLevel-row.minLevel+1));
  if(lead!=H::Lead::Synchronize){state=H::next(state);if(H::sample(state)%2==0)roll=row.maxLevel-row.minLevel;else if(roll>0)--roll;}
  s.attestedLevel=static_cast<uint8_t>(row.minLevel+roll);
  auto ns=H::next(s.preNatureSeed);
  if(lead==H::Lead::Synchronize&&H::sample(ns)%2==0)s.attestedNature=12;
  else{if(lead==H::Lead::Synchronize)ns=H::next(ns);s.attestedNature=static_cast<uint8_t>(H::sample(ns)%25u);}
  assert(H::evaluate(s).status==S::StageOnlyCompatibleHistoryUnverified);++accepted;
  auto alt=s;alt.originalSafariContextKnown=false;
  assert(H::evaluate(alt).status==S::UnknownOriginalStage);++unknown;
 }
 // Exercise BOTH Emerald Synchronize success and failure predicates. The
 // nature-stage seed is independently witnessed and is not assumed adjacent
 // to the slot/level RNG stages (there may be intervening retail calls).
 unsigned syncSuccess=0,syncFailure=0;
 for(uint32_t seed=0;seed<1024;++seed){
  H::Stage st{};st.exactOriginalGame=W::Game::Emerald;
  st.exactGroupIndex=15; // Route110 grass source
  st.originalLead=H::Lead::Synchronize;
  st.preSlotSeed=seed*12367u+19u;st.preNatureSeed=seed*9743u+31u;
  st.originalSyncNature=static_cast<uint8_t>((seed*7)%25);
  st.originalGameAndGroupAttested=st.originalSlotSeedAttested=true;
  st.originalNatureSeedAttested=st.originalSlotAndLevelAttested=true;
  st.originalLeadAttested=st.originalNatureAttested=true;
  st.originalSyncNatureAttested=true;
  st.originalSafariContextKnown=st.originalSafariPokeblockPathRuledOut=true;
  const auto& g=W::kGroups[st.exactGroupIndex];
  const auto sl=H::next(st.preSlotSeed);
  st.attestedSlot=H::slotFromRoll(g.method,st.rod,H::sample(sl));
  const auto row=W::decode(g,st.attestedSlot);
  const auto lv=H::next(sl);
  st.attestedLevel=static_cast<uint8_t>(row.minLevel+(H::sample(lv)%(row.maxLevel-row.minLevel+1)));
  uint32_t nat=H::next(st.preNatureSeed);
  const bool succeeded=(H::sample(nat)%2==0);
  if(succeeded){st.attestedNature=st.originalSyncNature;++syncSuccess;}
  else{nat=H::next(nat);st.attestedNature=static_cast<uint8_t>(H::sample(nat)%25);++syncFailure;}
  assert(H::evaluate(st).status==S::StageOnlyCompatibleHistoryUnverified);++accepted;
  auto bad=st;bad.attestedNature=static_cast<uint8_t>((st.attestedNature+1)%25);
  assert(H::evaluate(bad).status==S::CandidateNatureContradicted);++negative;
  bad=st;bad.originalSyncNatureAttested=false;
  assert(H::evaluate(bad).status==S::UnknownOriginalLead);++unknown;
 }
 assert(syncSuccess>0&&syncFailure>0);
 assert(accepted==33*128+4+1024);
 std::cout<<"L1 Gen3 source-attested original slot/level/nature RNG "<<accepted
          <<" positives, "<<negative<<" local negatives, "<<unknown<<" unknowns: PASS\n";
}
