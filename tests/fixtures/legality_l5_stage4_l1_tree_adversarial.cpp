// L5 tests the real byte-pinned L1 HGSS tree candidate helper, read-only.
#include "Legality/Gen4HgssOriginalTreeStage.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
namespace E=Legality::Gen4HgssOriginalTreeStage;
static uint64_t checks=0,local=0,unknown=0,empty=0,matched=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"L5-L1 failed line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
int main(){
 for(uint32_t tid=0;tid<48;tid++)for(uint32_t count=1;count<=45;count++)for(uint32_t tree=0;tree<count;tree++){
  const auto group=E::groupFor(uint8_t(count),uint8_t(tree),tid);
  CHECK(group==E::Group::A||group==E::Group::B||group==E::Group::Empty);
  E::Candidate c{};c.exactOriginalGame=tid%2?"soulsilver_nds":"heartgold_nds";
  c.originalFullTrainerId=tid;c.originalDesignatedTreeCount=uint8_t(count);
  c.originalTreeId=uint8_t(tree);c.witnessedGroup=group==E::Group::Empty?E::Group::A:group;
  c.originalSaveIndependentlyAttested=true;c.fullTrainerIdIndependentlyAttested=true;
  c.originalTreeListIndependentlyAttested=true;c.physicalTreeIdIndependentlyAttested=true;
  c.originalGroupIndependentlyAttested=true;
  auto observed=E::evaluate(c);
  CHECK(observed.computed==group);
  if(group==E::Group::Empty){CHECK(observed.status==E::Status::CandidateTreeEmpty);++empty;}
  else {
   CHECK(observed.status==E::Status::ConditionalOriginalTreeGroup);++matched;
   auto wrong=c;wrong.witnessedGroup=group==E::Group::A?E::Group::B:E::Group::A;
   CHECK(E::evaluate(wrong).status==E::Status::CandidateTreeGroupContradicted);++local;
  }
  // Individually drop each original provenance prerequisite; no guessed save source permitted.
  for(unsigned bit=0;bit<5;bit++){
   auto missing=c;
   if(bit==0)missing.originalSaveIndependentlyAttested=false;
   if(bit==1)missing.fullTrainerIdIndependentlyAttested=false;
   if(bit==2)missing.originalTreeListIndependentlyAttested=false;
   if(bit==3)missing.physicalTreeIdIndependentlyAttested=false;
   if(bit==4)missing.originalGroupIndependentlyAttested=false;
   const auto status=E::evaluate(missing).status;
   const auto expected=bit<=1?E::Status::UnknownOriginalTrainer:
       bit==2?E::Status::UnknownTreeList:
       bit==3?E::Status::UnknownTreeIdentity:E::Status::UnknownObservedGroup;
   CHECK(status==expected);++unknown;
  }
  auto swapped=c;swapped.exactOriginalGame="platinum_nds";
  CHECK(E::evaluate(swapped).status==E::Status::UnknownOriginalGame);++unknown;
  auto special=c;special.specialTreeC=true;special.originalTreeId=100;
  special.witnessedGroup=E::Group::SpecialC;
  CHECK(E::evaluate(special).status==E::Status::UnknownSpecialTreeSource);++unknown;
  special.specialTreePositionIndependentlyAttested=true;
  CHECK(E::evaluate(special).status==E::Status::ConditionalOriginalTreeGroup);
  special.witnessedGroup=E::Group::A;
  CHECK(E::evaluate(special).status==E::Status::CandidateTreeGroupContradicted);
 }
 E::Candidate invalid{};invalid.exactOriginalGame="heartgold_nds";
 invalid.originalSaveIndependentlyAttested=true;invalid.fullTrainerIdIndependentlyAttested=true;
 invalid.originalTreeListIndependentlyAttested=true;invalid.physicalTreeIdIndependentlyAttested=true;
 invalid.originalGroupIndependentlyAttested=true;invalid.witnessedGroup=E::Group::A;
 invalid.originalDesignatedTreeCount=46;
 CHECK(E::evaluate(invalid).status==E::Status::UnknownTreeList);
 invalid.originalDesignatedTreeCount=2;invalid.originalTreeId=2;
 CHECK(E::evaluate(invalid).status==E::Status::UnknownTreeIdentity);
 CHECK(E::groupFor(0,0,1)==E::Group::Unknown);
 CHECK(E::groupFor(46,0,1)==E::Group::Unknown);
 std::cout<<"L5 Stage4 actual L1 HGSS helper PASS "<<checks<<" assertions; "<<matched
          <<" local matches; "<<local<<" local-only contradictions; "<<unknown
          <<" missing-history Unknown; "<<empty<<" empty-tree candidates\n";
}
