#include "Legality/Gen56OriginalEggHistories.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen56OriginalEggHistories;
namespace E=Legality::Gen56PKHeXEggTemplate;
static H::Input make(E::Game game,unsigned seed){
  H::Input in{};in.originalEggId=seed+1;in.sameOriginalUnhatchedEggIndependentlyAttested=true;
  in.claimedEgg={game,25,0,4,4,true,true,true,false};
  in.count=1;in.allOriginalRoutesIndependentlyEnumerated=true;
  auto& r=in.routes[0];r.routeId=seed+20;r.originalEggId=in.originalEggId;
  r.originalEventId=seed+40;r.originalPairId=seed+50;r.sourceGame=game;
  r.originalEggSpecies=25;r.originalEventIndependentlyAttested=true;
  r.originalPhysicalPairIndependentlyAttested=true;
  r.originalPairEligibilityIndependentlyVerified=true;r.eligibleOriginalPair=true;
  r.originalHiddenTransmissionMethodIndependentlyVerified=true;
  r.allLocalMethodsIndependentlyCovered=true;
  r.slots[0]={seed+100,25,H::Sex::Female,true,true,true,229,true};
  r.slots[1]={seed+101,25,H::Sex::Male,true,false,true,236,true};
  return in;
}
int main(){
  std::uint64_t checks=0;
  for(unsigned g=1;g<=8;++g)for(unsigned seed=1;seed<=4096;++seed){
    auto in=make(static_cast<E::Game>(g),seed);
    if(E::isHiddenPossible5(25) && g<=4){assert(H::evaluate(in).verdict==H::Verdict::OneConditionalMatch);++checks;}
    else if(g>4){assert(H::evaluate(in).verdict==H::Verdict::OneConditionalMatch);++checks;}
    else {assert(H::evaluate(in).verdict==H::Verdict::AllOriginalLocalHistoriesContradicted);++checks;}
    auto x=in;x.routes[0].originalEggId++;
    assert(H::evaluate(x).verdict==H::Verdict::UnknownOriginalSource);++checks;
    x=in;x.routes[0].slots[0].snapshotId=x.routes[0].slots[1].snapshotId;
    assert(H::evaluate(x).verdict==H::Verdict::UnknownOriginalSource);++checks;
    x=in;x.routes[0].originalHiddenTransmissionMethodIndependentlyVerified=false;
    assert(H::evaluate(x).verdict==H::Verdict::UnresolvedLocalHistories);++checks;
    x=in;x.routes[0].slots[0].originallyHiddenAbility=false;
    assert(H::evaluate(x).verdict==H::Verdict::AllOriginalLocalHistoriesContradicted);++checks;
    x=in;x.routes[0].slots[1].originalSpecies=132;
    assert(H::evaluate(x).verdict==H::Verdict::OneConditionalMatch);++checks;
    x=in;x.routes[0].slots[1].originalSpecies=132;x.routes[0].slots[0].originalSpecies=132;
    assert(H::evaluate(x).verdict==H::Verdict::AllOriginalLocalHistoriesContradicted);++checks;
    x=in;x.routes[0].originalPairEligibilityIndependentlyVerified=false;
    assert(H::evaluate(x).verdict==H::Verdict::UnknownOriginalSource);++checks;
    x=in;x.routes[0].slots[1].sex=H::Sex::Female;
    assert(H::evaluate(x).verdict==H::Verdict::AllOriginalLocalHistoriesContradicted);++checks;
    x=in;x.allOriginalRoutesIndependentlyEnumerated=false;
    x.routes[0].eligibleOriginalPair=false;
    assert(H::evaluate(x).verdict==H::Verdict::UnknownExhaustiveness);++checks;
    x=in;x.count=2;x.routes[1]=in.routes[0];x.routes[1].routeId++;
    x.routes[0].originalEggId++;
    const auto rescued=H::evaluate(x);
    assert(rescued.verdict==H::Verdict::OneConditionalMatch && rescued.matched==1);
    ++checks;
    x=in;x.count=2;x.routes[1]=in.routes[0];x.routes[0].routeId=x.routes[1].routeId;
    x.routes[0].eligibleOriginalPair=false;
    assert(H::evaluate(x).verdict==H::Verdict::OneConditionalMatch);++checks;
    x=in;x.count=9;
    assert(H::evaluate(x).verdict==H::Verdict::CapacityUnknown);++checks;
    x=in;x.claimedEgg.originalAbilityNumber=255;
    assert(H::evaluate(x).verdict==H::Verdict::UnresolvedLocalHistories);++checks;
    x=in;x.claimedEgg.originalAbilityNumber=1;
    assert(H::evaluate(x).verdict==H::Verdict::OneConditionalMatch);++checks;
    x=in;x.claimedEgg.originalAbilityNumber=2;
    assert(H::evaluate(x).verdict==H::Verdict::OneConditionalMatch);++checks;
  }
  assert(checks==524288);
  std::cout<<"L2 Gen56 original parent histories: "<<checks<<" PASS\n";
}
