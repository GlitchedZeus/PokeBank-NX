#include "Legality/Gen1PretGiftEventStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen1PretGiftEventStage;
using S=G::Status;
G::Witness good(std::size_t i,const char* game) {
 const auto& scene=G::kScenes[i];
 G::Witness w{};
 w.game=game;w.sourceIndex=i;w.map=scene.map;
 w.species=scene.species;w.level=scene.level;
 w.gameAttested=w.sceneAttested=w.mapAttested=true;
 w.speciesAttested=w.levelAttested=true;
 w.priorUnclaimedAttested=true;w.priorUnclaimed=true;
 w.nativeGiftReceiptAttested=true;w.nativeGiftSucceeded=true;
 w.originalPaymentAttested=true;w.paidMoney=scene.sourcePrice;
 w.originalPikachuHappinessAttested=true;w.pikachuHappiness=147;
 w.originalThunderBadgeAttested=true;w.thunderBadge=true;
 return w;
}
int main() {
 static_assert(G::kScenes.size()==5);
 uint64_t conditional=0,local=0,unknown=0;
 for(unsigned g=0;g<3;++g) {
    const char* game=g==0?"red_gb":g==1?"blue_gb":"yellow_gb";
    const uint8_t mask=static_cast<uint8_t>(1u<<g);
    for(std::size_t i=0;i<G::kScenes.size();++i) {
      const auto& row=G::kScenes[i];
      auto w=good(i,game);
      auto r=G::evaluate(w);
      assert(!r.globalInvalidSupported&&!r.globallyLegalSupported);
      if(!(row.gameMask&mask)){
        assert(r.status==S::CandidateLocalForeignGame);
        ++local;continue;
      }
      assert(r.status==S::ConditionalSourceSceneOnly);
      assert(r.sourceSpecies==row.species&&r.sourceLevel==row.level);
      conditional+=3;
      auto n=w;n.gameAttested=false;
      assert(G::evaluate(n).status==S::UnknownOriginalGame);
      n=w;n.sceneAttested=false;
      assert(G::evaluate(n).status==S::UnknownOriginalScene);
      n=w;n.mapAttested=false;
      assert(G::evaluate(n).status==S::UnknownOriginalMap);
      n=w;n.speciesAttested=false;
      assert(G::evaluate(n).status==S::UnknownOriginalSpecies);
      n=w;n.levelAttested=false;
      assert(G::evaluate(n).status==S::UnknownOriginalLevel);
      n=w;n.priorUnclaimedAttested=false;
      assert(G::evaluate(n).status==S::UnknownOriginalEventFlag);
      n=w;n.nativeGiftReceiptAttested=false;
      assert(G::evaluate(n).status==
            (row.gate==G::Gate::PaidMagikarp?
             S::UnknownOriginalPurchaseReceipt:S::UnknownOriginalGiftReceipt));
      unknown+=7;
      auto bad=w;bad.map="WrongMap";
      assert(G::evaluate(bad).status==S::CandidateLocalMapMismatch);
      bad=w;bad.species=static_cast<uint16_t>(row.species==1?2:1);
      assert(G::evaluate(bad).status==S::CandidateLocalSpeciesMismatch);
      bad=w;bad.level=static_cast<uint8_t>(row.level+1);
      assert(G::evaluate(bad).status==S::CandidateLocalLevelMismatch);
      bad=w;bad.priorUnclaimed=false;
      assert(G::evaluate(bad).status==S::CandidateLocalAlreadyClaimed);
      bad=w;bad.nativeGiftSucceeded=false;
      assert(G::evaluate(bad).status==S::CandidateLocalGiftNotAccepted);
      local+=5;
      if(row.gate==G::Gate::PaidMagikarp) {
         n=w;n.originalPaymentAttested=false;
         assert(G::evaluate(n).status==S::UnknownOriginalPayment);
         ++unknown;
         bad=w;bad.paidMoney=499;
         assert(G::evaluate(bad).status==S::CandidateLocalPaymentMismatch);
         ++local;
         assert(row.sourcePrice==500&&row.species==129&&row.level==5);
      }
      if(row.gate==G::Gate::YellowHappiness147) {
         n=w;n.originalPikachuHappinessAttested=false;
         assert(G::evaluate(n).status==S::UnknownOriginalSceneCondition);
         ++unknown;
         for(unsigned value=0;value<256;++value) {
            auto happiness=w;happiness.pikachuHappiness=static_cast<uint8_t>(value);
            const auto status=G::evaluate(happiness).status;
            assert(status==(value<147?
                     S::CandidateLocalHappinessBelowThreshold:
                     S::ConditionalSourceSceneOnly));
            if(value<147)++local;else ++conditional;
         }
      }
      if(row.gate==G::Gate::YellowThunderBadge) {
         n=w;n.originalThunderBadgeAttested=false;
         assert(G::evaluate(n).status==S::UnknownOriginalSceneCondition);
         ++unknown;
         bad=w;bad.thunderBadge=false;
         assert(G::evaluate(bad).status==S::CandidateLocalBadgeMissing);
         ++local;
      }
    }
 }
 for(unsigned rawGame=0;rawGame<256;++rawGame) {
    for(unsigned i=0;i<256;++i) {
       auto w=good(0,"red_gb");
       w.sourceIndex=i;
       if(rawGame%3==0)w.game="red_gb";
       else if(rawGame%3==1)w.game="blue_gb";
       else w.game="yellow_gb";
       const auto r=G::evaluate(w);
       assert(!r.globalInvalidSupported&&!r.globallyLegalSupported);
       assert(r.status==(i<5?(i>=2?S::CandidateLocalForeignGame:
              i==0?S::ConditionalSourceSceneOnly:S::CandidateLocalMapMismatch):
              S::UnknownOriginalScene));
       ++unknown;
    }
 }
 assert(G::kScenes[0].sourcePrice==500);
 assert(G::kScenes[1].species==133&&G::kScenes[1].level==25);
 assert(G::kScenes[2].species==1&&G::kScenes[2].level==10);
 assert(G::kScenes[3].species==4&&G::kScenes[3].level==10);
 assert(G::kScenes[4].species==7&&G::kScenes[4].level==10);
 std::cout<<"L1 Gen I scripted acquisition originals: "<<conditional
          <<" conditional checks, "<<local<<" candidate-local negatives and "
          <<unknown<<" Unknown/context/bounds checks PASS\n";
}
