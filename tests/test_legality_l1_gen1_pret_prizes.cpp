#include "Legality/Gen1PretAcquisitionStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen1PretAcquisitionStage;
using S=G::Status;
G::Candidate valid(const G::PretGameCornerEntry& row) {
 G::Candidate c{};
 c.originalGame=row.gameMask==1?"red_gb":row.gameMask==2?"blue_gb":"yellow_gb";
 c.vendor=row.vendor;c.physicalSlot=row.physicalSlot;
 c.originalSpecies=row.species;c.originalLevel=row.level;
 c.originalCoinsPaid=row.coinPrice;
 c.gameIndependentlyAttested=c.vendorIndependentlyAttested=true;
 c.sourceSlotIndependentlyAttested=c.speciesIndependentlyAttested=true;
 c.levelIndependentlyAttested=c.purchaseIndependentlyAttested=true;
 c.paymentIndependentlyAttested=true;
 return c;
}
int main() {
 static_assert(G::kPretGameCornerOffers.size()==18);
 uint64_t positive=0,negative=0,unknown=0;
 for(const auto& row:G::kPretGameCornerOffers){
    auto c=valid(row);
    auto actual=G::offer(row.gameMask,row.vendor,row.physicalSlot);
    assert(actual==&row);
    auto r=G::evaluate(c);
    assert(r.status==S::ConditionalOriginalRedemptionOnly);
    assert(r.sourcePrice==row.coinPrice && r.sourceLevel==row.level);
    assert(r.sourceSpecies==row.species);
    assert(!r.globallyLegalSupported&&!r.globalInvalidSupported);
    positive+=4;
    auto bad=c;bad.originalSpecies=static_cast<uint16_t>(c.originalSpecies==1?2:1);
    assert(G::evaluate(bad).status==S::CandidateOriginalSpeciesMismatch);
    bad=c;bad.originalLevel=static_cast<uint8_t>(c.originalLevel==100?99:c.originalLevel+1);
    assert(G::evaluate(bad).status==S::CandidateOriginalLevelMismatch);
    bad=c;bad.originalCoinsPaid=static_cast<uint16_t>(row.coinPrice+1u);
    assert(G::evaluate(bad).status==S::CandidateOriginalPriceMismatch);
    negative+=3;
    auto n=c;n.gameIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalGame);
    n=c;n.vendorIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalVendor);
    n=c;n.sourceSlotIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalSlot);
    n=c;n.speciesIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalSpecies);
    n=c;n.levelIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalLevel);
    n=c;n.purchaseIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalPurchase);
    n=c;n.paymentIndependentlyAttested=false;
    assert(G::evaluate(n).status==S::UnknownOriginalPayment);
    unknown+=7;
    // A source menu/prize matchup is no original transaction receipt.
    n=c;n.purchaseIndependentlyAttested=false;
    n.originalCoinsPaid=row.coinPrice;
    assert(G::evaluate(n).status==S::UnknownOriginalPurchase);
    ++unknown;
 }
 // Original source Red vs Blue Abra differs in both level AND coin price.
 assert(G::offer(1,0,0)->level==9&&G::offer(1,0,0)->coinPrice==180);
 assert(G::offer(2,0,0)->level==6&&G::offer(2,0,0)->coinPrice==120);
 assert(G::offer(4,0,0)->level==15&&G::offer(4,0,0)->coinPrice==230);
 assert(G::offer(1,1,1)->species==123);
 assert(G::offer(2,1,0)->species==127);
 assert(G::offer(4,0,2)->species==40);
 assert(G::offer(1,1,2)->coinPrice==9999);
 assert(G::offer(2,1,2)->coinPrice==6500);
 positive+=8;
 for(unsigned raw=0;raw<256;++raw){
   const uint8_t mask=static_cast<uint8_t>(raw);
   for(unsigned vendor=0;vendor<256;++vendor){
     auto p=G::offer(mask,static_cast<uint8_t>(vendor),0);
     if((mask==1||mask==2||mask==4)&&vendor<2)
       assert(p!=nullptr);
     else assert(p==nullptr);
     ++unknown;
   }
 }
 for(const char* title:{"red_gb","blue_gb","yellow_gb","unknown_gb"}){
   for(unsigned vendor=2;vendor<256;++vendor){
     auto c=valid(G::kPretGameCornerOffers[0]);c.originalGame=title;
     c.vendor=static_cast<uint8_t>(vendor);
     auto outcome=G::evaluate(c);
     assert(outcome.status==(G::gameMask(title)?S::UnknownOriginalVendor:
                                                  S::UnknownOriginalGame));
     ++unknown;
   }
 }
 // Original Yellow Oak Pikachu is an entirely different historical
 // source: the original 0xA3 catch-rate/item byte is NOT a wild Pikachu
 // catch rate and is NOT inferred from modern native held-item status.
 G::YellowStarterWitness starter{};
 starter.originalGame="yellow_gb";starter.originalSpecies=25;
 starter.originalLevel=5;starter.originalCatchByte=0xA3;
 starter.originalGameIndependentlyAttested=true;
 starter.originalGiftSceneIndependentlyAttested=true;
 starter.originalCatchByteIndependentlyAttested=true;
 using Y=G::YellowStarterStatus;
 assert(G::evaluateYellowStarterByte(starter).status==
        Y::ConditionalOriginalYellowStarterByteOnly);
 ++positive;
 auto missingStarter=starter;missingStarter.originalGiftSceneIndependentlyAttested=false;
 assert(G::evaluateYellowStarterByte(missingStarter).status==Y::UnknownOriginalGiftScene);
 missingStarter=starter;missingStarter.originalCatchByteIndependentlyAttested=false;
 assert(G::evaluateYellowStarterByte(missingStarter).status==Y::UnknownOriginalCatchByte);
 missingStarter=starter;missingStarter.originalGame="red_gb";
 assert(G::evaluateYellowStarterByte(missingStarter).status==Y::UnknownOriginalGame);
 unknown+=3;
 for(unsigned originalByte=0;originalByte<256;++originalByte){
    auto special=starter;
    special.originalCatchByte=static_cast<uint8_t>(originalByte);
    const auto status=G::evaluateYellowStarterByte(special).status;
    assert(status==(originalByte==0xA3?
             Y::ConditionalOriginalYellowStarterByteOnly:
             Y::CandidateLocalCatchByteMismatch));
    if(originalByte==0xA3)++positive;else ++negative;
 }
 // Current PK1 bytes alone cannot establish this original acquisition.
 auto laterCapsule=starter;
 laterCapsule.originalCatchByteIndependentlyAttested=false;
 laterCapsule.originalCatchByte=0x60;
 assert(G::evaluateYellowStarterByte(laterCapsule).status==Y::UnknownOriginalCatchByte);
 ++unknown;
 std::cout<<"L1 pret RBY Game Corner 18 original prize rows, costs, levels, "
          <<positive<<" positive source-only, "<<negative<<" local negative and "
          <<unknown<<" Unknown/bounds assertions PASS\n";
}
