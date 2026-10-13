#include "Legality/Gen1OriginalNPCTradeMetadata.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace T=Legality::Gen1OriginalNPCTradeMetadata;
int main(){
 unsigned tests=0,reachable=0;
 for(unsigned edition=0;edition<256;++edition)
 for(unsigned index=0;index<256;++index){
   const auto r=T::row(static_cast<T::Edition>(edition),static_cast<uint8_t>(index));
   const bool supported=(edition==1||edition==2)&&index<10;
   assert((r.offered!=0)==supported);
   assert((r.received!=0)==supported);
   if(r.reachable) {
       assert(supported && r.offered<=151 && r.received<=151);
       ++reachable;
   }
   ++tests;
 }
 assert(tests==65536 && reachable==16);
 assert(T::row(T::Edition::EnglishYellow,4).received==151);
 assert(!T::row(T::Edition::EnglishYellow,4).reachable);
 assert(!T::row(T::Edition::EnglishYellow,6).reachable);
 assert(!T::row(T::Edition::EnglishRedBlue,2).reachable);
 assert(T::row(T::Edition::EnglishYellow,1).offered==35);
 assert(T::row(T::Edition::EnglishRedBlue,1).offered==63);
 std::cout<<"L4 source-verified English R/B/Y NPC metadata: "<<tests<<" tag cases; 16 reachable + 4 unusable rows: PASS\n";
}
