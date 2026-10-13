#include "Legality/Gen1RbyOriginalJointHistory.h"
#include "Legality/Gen1OriginalMachineTables.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace M=Legality::Gen1OriginalMachineTables;
namespace H=Legality::Gen1RbyOriginalJointHistory;
static_assert(M::kMoves.size()==56&&M::kRB.size()==152&&M::kYellow.size()==152);
static_assert(H::sourceMachine(H::Game::Red,25,H::Method::TM,24)==85);
static_assert(H::sourceMachine(H::Game::Yellow,25,H::Method::HM,5)==148);
static_assert(H::sourceMachine(H::Game::Red,132,H::Method::TM,10)==0);
static_assert(M::moveFor(2,151,true,5)==148);
int main(){
 std::uint64_t rbCount=0,yCount=0,cases=0;
 unsigned changed=0;
 for(unsigned species=1;species<=151;++species){
   if(M::kRB[species]!=M::kYellow[species])++changed;
   for(unsigned game=1;game<=3;++game){
     const auto which=static_cast<H::Game>(game);
     const auto original=game==3?M::kYellow[species]:M::kRB[species];
     for(unsigned idx=1;idx<=55;++idx){
       const bool hm=idx>50;
       const auto number=static_cast<std::uint8_t>(hm?idx-50:idx);
       const auto method=hm?H::Method::HM:H::Method::TM;
       const auto expected=(original&(std::uint64_t{1}<<(idx-1)))?M::kMoves[idx]:0;
       assert(H::sourceMachine(which,species,method,number)==expected);
       assert(M::moveFor(static_cast<std::uint8_t>(game),species,hm,number)==expected);
       assert(expected==0||(expected>=1&&expected<=165));
       if(expected){if(game==3)++yCount;else if(game==1)++rbCount;}
       cases+=3;
     }
   }
 }
 assert(changed==8);
 assert(rbCount==3037 && yCount==3043);
 // Invalid wire/enumeration values can NEVER inherit TM compatibility.
 // All 253 unrecognized Method bytes must return unsupported/Unknown.
 for(unsigned raw=3;raw<=255;++raw)
   for(unsigned game=1;game<=3;++game){
     assert(H::sourceMachine(static_cast<H::Game>(game),25,
            static_cast<H::Method>(raw),24)==0);
     cases+=1;
   }
 for(unsigned species=0;species<=152;++species){
   for(unsigned game=0;game<=5;++game){
     auto g=static_cast<H::Game>(game);
     assert(H::sourceMachine(g,species,H::Method::None,1)==0);
     assert(M::moveFor(game,species,false,0)==0);
     assert(M::moveFor(game,species,false,51)==0);
     assert(M::moveFor(game,species,true,6)==0);
     cases+=4;
   }
 }
 assert(M::kRB[151]==((std::uint64_t{1}<<55)-1));
 assert(M::kYellow[151]==((std::uint64_t{1}<<55)-1));
 // Original Yellow permits Fly for Charizard; non-party Pikachu's Surf is
 // NOT inferable from standard TM/HM compatibility.
 assert(M::moveFor(1,6,true,2)==0);
 assert(M::moveFor(3,6,true,2)==19);
 assert(M::moveFor(3,25,true,3)==0);
 std::cout<<"Original Gen I RBY/Yellow 151 species x 55 machines: "<<cases
          <<" candidate-compatibility assertions PASS; NOT trainer receipts\n";
}
