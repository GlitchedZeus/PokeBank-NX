#include "Legality/Gen1IndexedOriginalSourceHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen1IndexedOriginalSourceHistory;
namespace E=Legality::Gen1Encounter;
using Status=H::Status;
int main(){
 static_assert(E::kStaticEntries.size()==31);
 static_assert(E::kTradeEntries.size()==17);
 static_assert(E::kPretTradeOffers.size()==20);
 static_assert(E::kPretStaticSources.size()==9);
 std::uint64_t compatible=0,negative=0,unknown=0;
 for(std::size_t i=0;i<E::kPretStaticSources.size();++i) {
    const auto& r=E::kPretStaticSources[i];
    for(auto game:{"red_gb","blue_gb","yellow_gb"}){
      H::PretStaticCandidate c{};
      c.originalGame=game;c.originalSceneIndex=i;c.originalMap=r.originalMap;
      c.originalSpecies=r.species;c.originalLevel=r.level;
      c.originalGameIndependentlyAttested=true;
      c.originalSceneIndependentlyAttested=true;
      c.originalMapIndependentlyAttested=true;
      c.originalSpeciesIndependentlyAttested=true;
      c.originalLevelIndependentlyAttested=true;
      assert(H::evaluatePretStatic(c).status==Status::ConditionalPretStaticSceneOnly);
      ++compatible;
      auto n=c;n.originalSceneIndependentlyAttested=false;
      assert(H::evaluatePretStatic(n).status==Status::UnknownOriginalRow);
      n=c;n.originalMapIndependentlyAttested=false;
      assert(H::evaluatePretStatic(n).status==Status::UnknownOriginalMap);
      n=c;n.originalSpeciesIndependentlyAttested=false;
      assert(H::evaluatePretStatic(n).status==Status::UnknownOriginalSpecies);
      n=c;n.originalLevelIndependentlyAttested=false;
      assert(H::evaluatePretStatic(n).status==Status::UnknownOriginalLevel);
      unknown+=4;
      auto bad=c;bad.originalMap="UnrelatedMap";
      assert(H::evaluatePretStatic(bad).status==Status::CandidateOriginalMapContradicted);
      bad=c;bad.originalSpecies=static_cast<uint16_t>(r.species==1?2:1);
      assert(H::evaluatePretStatic(bad).status==Status::CandidateOriginalSpeciesContradicted);
      bad=c;bad.originalLevel=static_cast<uint8_t>(r.level+1);
      assert(H::evaluatePretStatic(bad).status==Status::CandidateOriginalLevelContradicted);
      negative+=3;
    }
 }
 assert(E::kPretStaticSources[0].species==143);
 assert(E::kPretStaticSources[0].originalMap=="Route12");
 assert(E::kPretStaticSources[1].originalMap=="Route16");
 assert(E::kPretStaticSources[2].species==145);
 assert(E::kPretStaticSources[3].species==150);
 assert(E::kPretStaticSources[6].species==131);
 assert(E::kPretStaticSources[7].species==106);
 assert(E::kPretStaticSources[8].species==107);
 for(const auto& offer:E::kPretTradeOffers){
    for(unsigned g=0;g<3;++g){
      const char* title=g==0?"red_gb":g==1?"blue_gb":"yellow_gb";
      auto looked=E::originalPretTrade(1u<<g,offer.originalOfferIndex);
      if(!(offer.gameMask&(1u<<g)))continue;
      assert(looked&&looked->receivedSpecies==offer.receivedSpecies);
      assert(looked->requestedSpecies==offer.requestedSpecies);
      assert(looked->nickname==offer.nickname);
      H::PretTradeCandidate c{};
      c.originalGame=title;c.originalOfferIndex=offer.originalOfferIndex;
      c.requestedSpecies=offer.requestedSpecies;
      c.receivedSpecies=offer.receivedSpecies;
      c.originalGameIndependentlyAttested=true;
      c.originalOfferIndexIndependentlyAttested=true;
      c.originalRequestedSpeciesIndependentlyAttested=true;
      c.originalReceivedSpeciesIndependentlyAttested=true;
      c.originalNpcTrainerIndependentlyAttested=true;
      c.originalTransactionIndependentlyAttested=true;
      if(!offer.reachable){
         assert(H::evaluatePretNpcTrade(c).status==Status::UnknownUnreachableNpcTrade);
         ++unknown;continue;
      }
      assert(H::evaluatePretNpcTrade(c).status==Status::ConditionalPretNpcTradeOfferOnly);
      ++compatible;
      auto n=c;n.originalOfferIndexIndependentlyAttested=false;
      assert(H::evaluatePretNpcTrade(n).status==Status::UnknownOriginalRow);
      n=c;n.originalRequestedSpeciesIndependentlyAttested=false;
      assert(H::evaluatePretNpcTrade(n).status==Status::UnknownOriginalRequest);
      n=c;n.originalReceivedSpeciesIndependentlyAttested=false;
      assert(H::evaluatePretNpcTrade(n).status==Status::UnknownOriginalSpecies);
      n=c;n.originalNpcTrainerIndependentlyAttested=false;
      assert(H::evaluatePretNpcTrade(n).status==Status::UnknownTradeOT);
      n=c;n.originalTransactionIndependentlyAttested=false;
      assert(H::evaluatePretNpcTrade(n).status==Status::UnknownOriginalTradeReceipt);
      unknown+=5;
      auto bad=c;bad.requestedSpecies=static_cast<uint16_t>(offer.requestedSpecies==1?2:1);
      assert(H::evaluatePretNpcTrade(bad).status==Status::CandidateOriginalRequestContradicted);
      bad=c;bad.receivedSpecies=static_cast<uint16_t>(offer.receivedSpecies==1?2:1);
      assert(H::evaluatePretNpcTrade(bad).status==Status::CandidateOriginalSpeciesContradicted);
      negative+=2;
    }
 }
 // Unused Yellow Mew-for-Mew, Butterfree, Pidgeot offer slots are
 // explicitly not obtainable. They must never become positive simply
 // because the submitted species and source row match their ROM entries.
 for(unsigned slot: {2u,4u,6u}){
    const auto* row=E::originalPretTrade(4,static_cast<uint8_t>(slot));
    assert(row&&!row->reachable);
 }
 for(unsigned k=10;k<256;++k){
    assert(E::originalPretTrade(4,static_cast<uint8_t>(k))==nullptr);
    H::PretTradeCandidate c{};c.originalGame="yellow_gb";
    c.originalOfferIndex=static_cast<uint8_t>(k);
    c.originalGameIndependentlyAttested=true;
    c.originalOfferIndexIndependentlyAttested=true;
    assert(H::evaluatePretNpcTrade(c).status==Status::UnknownOriginalRow);
    ++unknown;
 }
 H::PretStaticCandidate incorrect{};
 incorrect.originalGame="red_gb";incorrect.originalGameIndependentlyAttested=true;
 incorrect.originalSceneIndependentlyAttested=true;
 incorrect.originalSceneIndex=99;
 assert(H::evaluatePretStatic(incorrect).status==Status::UnknownOriginalRow);
 std::cout<<"L1 pret RBY original source gifts/statics "<<E::kPretStaticSources.size()
          <<", NPC offers "<<E::kPretTradeOffers.size()
          <<": "<<compatible<<" template candidates, "<<negative
          <<" local negatives, "<<unknown<<" Unknown/unused assertions PASS\n";
}
