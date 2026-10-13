// L5 adversarial against exact L2 Diamond/Pearl retail IV-stage helper.
// Inputs are synthetic attested-stage hypotheses, not actual save/PID witnesses.
#include "Legality/Gen4DiamondPearlEggIVStage.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
namespace I=Legality::Gen4DiamondPearlEggIVStage;
static uint64_t checks=0,positive=0,local=0,unknown=0,duplicateStats=0,alternativeMatches=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"L5-L2 failed line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
int main(){
 for(const char* game:{"diamond_nds","pearl_nds"})for(uint32_t seed=0;seed<8192;seed++){
  const std::array<uint8_t,6> before{1,2,3,4,5,6},p0{7,8,9,10,11,12},p1{17,18,19,20,21,22};
  const auto stage=I::replay(game,seed,before,p0,p1);
  CHECK(stage.status==I::Status::Replayed);
  CHECK(I::rewind(stage.finalSeed)==seed);
  CHECK(stage.finalSeed!=seed || seed==0xFFFFFFFFu); // six nonidentity draws
  CHECK(stage.distinctSelected>=1 && stage.distinctSelected<=3);
  if(stage.distinctSelected<3)++duplicateStats;
  for(size_t ix=0;ix<3;ix++){
   CHECK(stage.selected[ix]<6 && stage.donor[ix]<2);
   CHECK(stage.child[stage.selected[ix]]==p0[stage.selected[ix]] ||
         stage.child[stage.selected[ix]]==p1[stage.selected[ix]]);
  }
  I::OriginalStage x{};x.game=game;x.initialChild=before;
  x.physicalSlot0=p0;x.physicalSlot1=p1;x.originalEggIVs=stage.child;
  x.finalSeed=stage.finalSeed;
  x.historicalParentIVsVerified=true;x.physicalDepositOrderVerified=true;
  x.preInheritanceChildIVsVerified=true;x.originalUnhatchedEggIVsVerified=true;
  x.finalSeedIndependentlyVerified=true;
  CHECK(I::verify(x)==I::Check::Matched);++positive;
  // Removing each independent source receipt must be Unknown, not locally impossible.
  for(int mask=0;mask<5;mask++){
   auto missing=x;
   if(mask==0)missing.historicalParentIVsVerified=false;
   if(mask==1)missing.physicalDepositOrderVerified=false;
   if(mask==2)missing.preInheritanceChildIVsVerified=false;
   if(mask==3)missing.originalUnhatchedEggIVsVerified=false;
   if(mask==4)missing.finalSeedIndependentlyVerified=false;
   const auto expected=mask==0?I::Check::UnknownParentIVs:
      mask==1?I::Check::UnknownDepositOrder:
      mask==2?I::Check::UnknownInitialChildIVs:
      mask==3?I::Check::UnknownOriginalChildIVs:I::Check::UnknownFinalSeed;
   CHECK(I::verify(missing)==expected);++unknown;
  }
  auto mismatch=x;mismatch.originalEggIVs[0]^=1;
  CHECK(I::verify(mismatch)==I::Check::Contradicted);++local;
  auto wrongSeed=x;wrongSeed.finalSeed=I::advance(x.finalSeed);
  const auto alternative=I::replay(game,I::rewind(wrongSeed.finalSeed),before,p0,p1);
  const bool sameOutput=alternative.child==stage.child;
  CHECK(I::verify(wrongSeed)==(sameOutput?I::Check::Matched:I::Check::Contradicted));
  if(sameOutput)++alternativeMatches;else ++local;
  auto cross=x;cross.game="platinum_nds";
  CHECK(I::verify(cross)==I::Check::UnsupportedGame);
  auto invalid=x;invalid.initialChild[0]=32;
  CHECK(I::verify(invalid)==I::Check::MalformedIVs);
 }
 // Power-item/Platinum branch must not silently borrow D/P mechanics.
 CHECK(I::replay("heartgold_nds",0,{},{},{}).status==I::Status::UnsupportedGame);
 CHECK(I::replay("platinum_nds",0,{},{},{}).status==I::Status::UnsupportedGame);
 CHECK(alternativeMatches==168);
 std::cout<<"L5 Stage4 actual L2 DP egg-IV helper PASS "<<checks<<" assertions; "<<positive
          <<" attested-stage matches, "<<local<<" local contradictions, "<<unknown
          <<" missing-source Unknown, "<<duplicateStats<<" repeated inherited indices, "<<alternativeMatches<<" alternate-seed output collisions\n";
}
