// L5 attacks real L3 source-backed GBA evolution table boundaries.
// Source offers alone are NOT four-move coexistence or original individual evidence.
#include "Legality/Gen14CandidateMoveGbaStarterTables.h"
#include "Legality/Gen14CandidateMoveGbaNinjaTables.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
namespace S=Legality::Gen14CandidateMoveGbaStarterTables;
namespace N=Legality::Gen14CandidateMoveGbaNinjaTables;
static uint64_t checks=0,compatible=0,versionLocal=0,unknown=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"L5-L3 failed line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
enum class State { CandidateLocalMatched, CandidateLocalContradicted, Unknown };
template<class O,size_t Extent> State sameOffer(std::span<const O,Extent> offers,uint8_t level,uint16_t move,bool exactGameVerified,bool sourceRowComplete) {
 if(!exactGameVerified || !sourceRowComplete)return State::Unknown;
 bool levelPresent=false;
 for(const auto& o:offers)if(o.level==level){levelPresent=true;if(o.move==move)return State::CandidateLocalMatched;}
 return levelPresent?State::CandidateLocalContradicted:State::Unknown;
}
template<class A> void exercise(const A& a){
 for(unsigned level=1;level<=100;level++)for(unsigned move=1;move<=354;move++){
  const auto yes=sameOffer(std::span(a),uint8_t(level),uint16_t(move),true,true);
  const auto noGame=sameOffer(std::span(a),uint8_t(level),uint16_t(move),false,true);
  const auto noCoverage=sameOffer(std::span(a),uint8_t(level),uint16_t(move),true,false);
  CHECK(noGame==State::Unknown && noCoverage==State::Unknown);
  bool hadLevel=false,matched=false;
  for(const auto& o:a)if(o.level==level){hadLevel=true;matched|=o.move==move;}
  const auto expected=matched?State::CandidateLocalMatched:
                       hadLevel?State::CandidateLocalContradicted:State::Unknown;
  CHECK(yes==expected);
  if(yes==State::CandidateLocalMatched)++compatible;
  else if(yes==State::Unknown)++unknown;
 }
 // Keep ordered same-level actions: three Ninjask offers at Lv20 are not one action.
 for(size_t i=1;i<a.size();i++)CHECK(a[i].level>=a[i-1].level);
}
int main(){
 exercise(S::kIvysaur);exercise(S::kVenusaur);exercise(S::kCharmeleonRse);
 exercise(S::kCharmeleonFrlg);exercise(S::kCharizardRse);exercise(S::kCharizardFrlg);
 exercise(S::kSquirtle);exercise(S::kWartortle);exercise(S::kBlastoise);
 exercise(N::kNincada);exercise(N::kNinjask);exercise(N::kShedinja);
 for(unsigned level=1;level<=100;level++)for(unsigned move=1;move<=354;move++){
  const auto ruby=sameOffer(std::span(S::kCharmeleonRse),uint8_t(level),uint16_t(move),true,true);
  const auto frlg=sameOffer(std::span(S::kCharmeleonFrlg),uint8_t(level),uint16_t(move),true,true);
  const bool changed=(level==13 && (move==108||move==232)) || (level==20 && (move==99||move==108));
  if(changed){CHECK(ruby!=frlg);++versionLocal;}
  else CHECK(ruby==frlg);
 }
 // Source rows distinguish 3 Ninjask lv20 moves and Shedinja-only independent timeline.
 CHECK(sameOffer(std::span(N::kNinjask),20,104,true,true)==State::CandidateLocalMatched);
 CHECK(sameOffer(std::span(N::kNinjask),20,210,true,true)==State::CandidateLocalMatched);
 CHECK(sameOffer(std::span(N::kNinjask),20,103,true,true)==State::CandidateLocalMatched);
 CHECK(sameOffer(std::span(N::kShedinja),20,104,true,true)==State::Unknown);
 CHECK(sameOffer(std::span(N::kShedinja),38,247,true,true)==State::CandidateLocalMatched);
 CHECK(sameOffer(std::span(N::kNincada),38,232,true,true)==State::CandidateLocalMatched);
 CHECK(sameOffer(std::span(S::kCharizardFrlg),1,257,true,true)==State::CandidateLocalMatched);
 CHECK(sameOffer(std::span(S::kCharizardRse),1,257,true,true)==State::CandidateLocalContradicted);
 std::cout<<"L5 Stage4 actual L3 original move-source tables PASS "<<checks
          <<" assertions; "<<compatible<<" exact source row matches, "<<versionLocal
          <<" candidate-only version differences, "<<unknown<<" absent-level Unknown\n";
}
