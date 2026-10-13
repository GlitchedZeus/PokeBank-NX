#include "Legality/Gen56PKHeXEggTemplate.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P=Legality::Gen56PKHeXEggTemplate;
int main(){
  std::uint64_t checks=0;
  for(unsigned b=0;b<82;++b)for(unsigned n=0;n<8;++n){
    const unsigned s=b*8+n;
    assert(P::isHiddenPossible5(s)==((P::BanHidden5[b]&(1u<<n))==0));++checks;
  }
  for(unsigned gi=1;gi<=8;++gi)for(unsigned sp=1;sp<=721;++sp)for(unsigned f=0;f<=6;++f){
    auto g=static_cast<P::Game>(gi);
    P::Claim c{};c.originGame=g;c.originalEggSpecies=sp;c.form=f;
    c.originalAbilityNumber=4;c.originalBall=4;
    c.originalEggSourceIndependentlyAttested=true;
    c.originalAbilityIndependentlyAttested=true;
    c.originalBallIndependentlyAttested=true;
    const auto r=P::evaluate(c);
    const bool valid=P::validSpecies(g,sp);
    if(!valid)assert(r.status==P::Status::Unsupported);
    else {
       const auto possible=P::generation(g)==5?P::isHiddenPossible5(sp):P::isHiddenPossible6(sp,f);
       assert(r.status==(possible?P::Status::ConditionallyCompatible:P::Status::LocalContradiction));
       ++checks;
       auto unknown=c;unknown.originalEggSourceIndependentlyAttested=false;
       assert(P::evaluate(unknown).status==P::Status::IncompleteEvidence);++checks;
       unknown=c;unknown.originalBall=5;
       const auto nonpoke=P::evaluate(unknown).status;
       if(P::generation(g)==5)assert(nonpoke==P::Status::LocalContradiction || !possible);
       else assert(nonpoke==(possible?P::Status::IncompleteEvidence:P::Status::LocalContradiction));
       ++checks;
       unknown.originalBallInheritanceMethodIndependentlyVerified=true;
       const auto inherited=P::evaluate(unknown).status;
       if(P::generation(g)==6)assert(inherited==(possible?P::Status::ConditionallyCompatible:P::Status::LocalContradiction));
       else assert(inherited==P::Status::LocalContradiction || !possible);
       ++checks;
    }
    ++checks;
  }
  for(unsigned s=1;s<=721;++s){
    const bool split=P::splitBreedNotBaby(s);
    assert(split==(s==183||s==202||s==113||s==122||s==143||s==185||s==226||s==315||s==358));++checks;
    assert(P::speciesDeterminedByGender(s)==(s==29||s==32||s==313||s==314));++checks;
  }
  P::Claim bad{};bad.originGame=P::Game::Black;bad.originalEggSpecies=25;bad.originalAbilityNumber=255;
  assert(P::evaluate(bad).status==P::Status::IncompleteEvidence);++checks;
  std::cout<<"L2 Gen56 PKHeX egg template checks: "<<checks<<" PASS\n";
}
