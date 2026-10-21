#include "Legality/Gen56PkhexMachineSources.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P=Legality::Gen56PkhexMachineSources;
using G=P::Game;using S=P::Status;
int main(){
 std::uint64_t checks=0;
 for(auto g:{G::Black,G::White,G::Black2,G::White2,G::X,G::Y,G::OmegaRuby,G::AlphaSapphire}) {
  const auto t=P::machineTable(g);auto n=P::machineTMs(g),h=P::machineHMs(g);
  assert(t.size()==n+h);++checks;
  for(unsigned i=0;i<t.size();++i) {
   const bool hm=i>=n;const auto number=static_cast<std::uint8_t>(hm?i-n+1:i+1);
   const auto good=P::machine(g,hm,number,t[i]);
   if((g==G::Black||g==G::White)&&!hm&&number==95)assert(good==S::Contradicts);
   else assert(good==S::Matches);
   ++checks;
   assert(P::machine(g,hm,number,0)==S::Unknown);++checks;
   if((g==G::Black||g==G::White)&&!hm&&number==95)continue;
   const auto wrong=static_cast<std::uint16_t>(t[i]+1);
   assert(P::machine(g,hm,number,wrong)==S::Contradicts);++checks;
  }
  assert(P::machine(g,false,n+1,10)==S::Unknown);++checks;
  assert(P::machine(g,true,h+1,10)==S::Unknown);++checks;
 }
 assert(P::machine(G::Black,false,95,555)==S::Contradicts);++checks;
 assert(P::machine(G::White2,false,95,555)==S::Matches);++checks;
 assert(P::machine(G::X,true,6,249)==S::Unknown);++checks;
 assert(P::machine(G::OmegaRuby,true,6,249)==S::Matches);++checks;
 assert(P::machine(G::X,false,94,249)==S::Matches);++checks;
 assert(P::machine(G::OmegaRuby,false,94,290)==S::Matches);++checks;
 assert(P::machine(G::Diamond,false,1,468)==S::Unknown);++checks;
 std::cout<<"L3 Gen V/VI exact PKHeX machine map: "<<checks<<" source-index checks PASS\n";
}
