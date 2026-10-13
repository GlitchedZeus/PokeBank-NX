#include "Legality/Gen1PretTradeMetadataStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
namespace M=Legality::Gen1PretTradeMetadataStage;
using S=M::Status;
namespace Source {
struct PretTradeEntry {
 uint8_t gameMask,originalOfferIndex;
 uint16_t requestedSpecies,receivedSpecies;
 bool reachable;
 std::string_view nickname;
};
#include "Legality/Gen1PretTrades.inc"
}
M::Witness witness(const Source::PretTradeEntry& row,
                   const char* game) {
 M::Witness c{};
 c.originalGame=game;c.originalGameAttested=true;
 c.originalOfferIndex=row.originalOfferIndex;
 c.sourceRowAttested=true;
 c.originalReceivedNickname=row.nickname;
 c.originalReceivedOT="<TRAINER>";
 c.originalNicknameAttested=c.originalTrainerNameAttested=true;
 c.originalPostTradeSpecies=row.receivedSpecies;
 c.originalPostTradeSpeciesAttested=true;
 return c;
}
int main(){
 static_assert(Source::kPretTradeOffers.size()==20);
 uint64_t positive=0,negative=0,unknown=0;
 for(const auto& row:Source::kPretTradeOffers){
   for(const char* game:{"red_gb","blue_gb","yellow_gb"}){
     if((row.gameMask&M::gameMask(game))==0)continue;
     auto c=witness(row,game);
     auto got=M::evaluate(row,c);
     assert(!got.globallyLegalSupported&&!got.globalInvalidSupported);
     assert(!got.originalOTIDWasSourceFixed);
     if(!row.reachable) {
       assert(got.status==S::UnknownUnreachableOffer);++unknown;continue;
     }
     assert(got.status==S::ConditionalOriginalNpcMetadataOnly);
     assert(got.sourceReceivedSpecies==row.receivedSpecies);
     positive+=2;
     auto no=c;no.originalNicknameAttested=false;
     assert(M::evaluate(row,no).status==S::UnknownOriginalNickname);
     no=c;no.originalTrainerNameAttested=false;
     assert(M::evaluate(row,no).status==S::UnknownOriginalTrainerString);
     no=c;no.originalPostTradeSpeciesAttested=false;
     assert(M::evaluate(row,no).status==S::UnknownOriginalReceivedStage);
     no=c;no.sourceRowAttested=false;
     assert(M::evaluate(row,no).status==S::UnknownOriginalSourceRow);
     unknown+=4;
     auto bad=c;bad.originalReceivedNickname="FAKE";
     assert(M::evaluate(row,bad).status==S::CandidateLocalNicknameMismatch);
     bad=c;bad.originalReceivedOT="FIXED123";
     assert(M::evaluate(row,bad).status==S::CandidateLocalTrainerStringMismatch);
     bad=c;bad.originalPostTradeSpecies=
       static_cast<uint16_t>(row.receivedSpecies==1?2:1);
     assert(M::evaluate(row,bad).status==S::CandidateLocalReceivedSpeciesMismatch);
     negative+=3;
     // An original generated 16-bit OT ID is not equal to a fixed
     // 0x0000 or 0xFFFF constant. Different valid IDs cannot become
     // negative without an independent original frame witness.
     for(unsigned originalID:{0u,1u,0x1234u,0xffffu}){
       (void)originalID;
       assert(M::evaluate(row,c).status==S::ConditionalOriginalNpcMetadataOnly);
       ++positive;
     }
   }
 }
 // Yellow slot 9: Cubone -> Machoke, which Yellow's post-receipt
 // InGameTrade_CheckForTradeEvo can immediately evolve to Machamp.
 const auto& yellow=Source::kPretTradeOffers[19];
 assert(yellow.originalOfferIndex==9&&yellow.receivedSpecies==67);
 auto y=witness(yellow,"yellow_gb");
 y.originalPostTradeSpecies=68;
 assert(M::evaluate(yellow,y).status==S::UnknownOriginalPostTradeEvolution);
 ++unknown;
 y.originalPostTradeEvolutionAttested=true;
 y.originalTradeEvolutionCompleted=true;
 assert(M::evaluate(yellow,y).status==S::ConditionalOriginalNpcMetadataOnly);
 assert(M::evaluate(yellow,y).sourcePostTradeSpecies==68);
 positive+=2;
 y.originalTradeEvolutionCompleted=false;
 assert(M::evaluate(yellow,y).status==S::CandidateLocalReceivedSpeciesMismatch);
 ++negative;
 // This Yellow behavior must never be projected into Red or Blue.
 y=witness(yellow,"red_gb");
 assert(M::evaluate(yellow,y).status==S::UnknownOriginalSourceRow);
 ++unknown;
 for(unsigned i=0;i<256;++i){
   auto c=witness(Source::kPretTradeOffers[0],"red_gb");
   c.originalOfferIndex=static_cast<uint8_t>(i);
   const auto r=M::evaluate(Source::kPretTradeOffers[0],c);
   assert(r.status==(i==0?S::ConditionalOriginalNpcMetadataOnly:
                           S::UnknownOriginalSourceRow));
   ++unknown;
 }
 std::cout<<"L1 original RBY NPC metadata: "<<positive
          <<" conditional, "<<negative<<" local, "<<unknown
          <<" Unknown cases PASS\n";
}
