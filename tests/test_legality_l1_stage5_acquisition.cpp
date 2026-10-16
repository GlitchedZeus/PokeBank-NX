#include "Legality/Gen4OriginalAcquisitionReconciliation.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
using namespace Legality::Gen4OriginalAcquisitionReconciliation;
Candidate make(Game game,Family family,Stage stage=Stage::OriginalRngMatched){
 Candidate c{};c.originalSaveIdentity=33;c.physicalCreatureIdentity=44;c.routeIdentity=55;
 c.game=game;c.family=family;c.stage=stage;
 c.originalSaveAndCreatureIndependentlyAttested=true;
 c.originalRouteAndFamilyIndependentlyAttested=true;
 c.originalSourceRowIndependentlyAttested=true;
 c.originalCaptureContextIndependentlyAttested=true;
 c.originalPidIvStageIndependentlyAttested=true;
 c.originalHoneySlatherTimerAndTreeAttested=true;
 c.originalHoneyTrainerForMunchlaxAttested=true;
 c.originalRadarActivationAttested=true;
 c.originalRadarChainHistoryAttested=true;
 c.originalTreePositionAndGroupAttested=true;
 c.originalFishingRodTimeAndSwarmAttested=true;
 c.originalSafariBlocksAndTimeAttested=true;
 c.originalBugContestDayAndRerollAttested=true;
 c.originalRockSmashAreaAndRateAttested=true;
 c.originalGiftOrEggSceneAttested=true;
 c.originalRoamerActivationAndRouteAttested=true;
 return c;
}
int main(){
 unsigned positives=0,unknowns=0,local=0;
 for(unsigned game=0;game<256;++game)for(unsigned family=0;family<256;++family){
   auto c=make(static_cast<Game>(game),static_cast<Family>(family));
   const auto r=evaluate(c);
   const bool allowed=pathMatchesGame(c.game,c.family);
   if(allowed){assert(r.status==Status::ConditionalOriginalRngStageOnly);++positives;}
   else{assert(r.status==Status::UnknownOriginalGame ||
               r.status==Status::UnknownOriginalSource);++unknowns;}
   assert(!r.globalInvalidSupported&&!r.globallyLegalSupported);
 }
 assert(positives==5*12-3*5-2*4); // 37: 3 Sinnoh omit 5 HGSS; 2 HGSS omit 4 Sinnoh
 for(unsigned f=1;f<=12;++f){
   const Family family=static_cast<Family>(f);
   const Game game=f<=4?Game::Platinum:Game::HeartGold;
   auto c=make(game,family);
   assert(evaluate(c).status==Status::ConditionalOriginalRngStageOnly);++positives;
   auto missing=c;missing.originalCaptureContextIndependentlyAttested=false;
   assert(evaluate(missing).status==Status::UnknownOriginalContext);++unknowns;
   missing=c;missing.originalPidIvStageIndependentlyAttested=false;
   assert(evaluate(missing).status==Status::UnknownStage);++unknowns;
   missing=c;missing.stage=Stage::BoundedExhausted;
   assert(evaluate(missing).status==Status::UnknownSearchBound);++unknowns;
   missing=c;missing.stage=Stage::SourceTemplate;
   missing.originalPidIvStageIndependentlyAttested=false;
   assert(evaluate(missing).status==Status::ConditionalOriginalSourceOnly);++positives;
   missing=c;missing.stage=Stage::CandidateContradiction;
   assert(evaluate(missing).status==Status::CandidateLocalContradiction);++local;
   missing=c;missing.originalSaveIdentity=0;
   assert(evaluate(missing).status==Status::UnknownRoot);++unknowns;
   missing=c;missing.routeIdentity=0;
   assert(evaluate(missing).status==Status::UnknownOriginalSource);++unknowns;
   missing=c;missing.originalRouteAndFamilyIndependentlyAttested=false;
   assert(evaluate(missing).status==Status::UnknownOriginalSource);++unknowns;
   auto check=[&](bool Candidate::*flag){ auto b=c;b.*flag=false;
       assert(evaluate(b).status==Status::UnknownOriginalContext);++unknowns; };
   switch(family){
   case Family::SinnohHoney:
      check(&Candidate::originalHoneySlatherTimerAndTreeAttested);
      check(&Candidate::originalHoneyTrainerForMunchlaxAttested);break;
   case Family::SinnohRadar:
      check(&Candidate::originalRadarActivationAttested);break;
   case Family::SinnohChainShiny:
      check(&Candidate::originalRadarActivationAttested);
      check(&Candidate::originalRadarChainHistoryAttested);break;
   case Family::HgssHeadbutt:
      check(&Candidate::originalTreePositionAndGroupAttested);break;
   case Family::HgssFishing:
      check(&Candidate::originalFishingRodTimeAndSwarmAttested);break;
   case Family::HgssSafari:
      check(&Candidate::originalSafariBlocksAndTimeAttested);break;
   case Family::HgssBugContest:
      check(&Candidate::originalBugContestDayAndRerollAttested);break;
   case Family::HgssRockSmash:
      check(&Candidate::originalRockSmashAreaAndRateAttested);break;
   case Family::StaticGift:case Family::GiftEgg:
      check(&Candidate::originalGiftOrEggSceneAttested);break;
   case Family::Roamer:
      check(&Candidate::originalRoamerActivationAndRouteAttested);break;
   default:break;
   }
 }
 // Exhaust all 3^8 route-evidence permutations; every route has a distinct
 // original identity, but no set can be considered the entire retail universe.
 for(unsigned ternary=0;ternary<6561;++ternary){
    auto n=ternary;
    std::array<Candidate,8> routes{};
    unsigned matched=0,contradicted=0,unresolved=0;
    for(unsigned i=0;i<8;++i){
       const auto type=n%3u;n/=3u;
       routes[i]=make(Game::HeartGold,Family::HgssSafari);
       routes[i].routeIdentity=i+1;
       routes[i].stage=type==0?Stage::OriginalRngMatched:
                       type==1?Stage::CandidateContradiction:Stage::BoundedExhausted;
       matched+=type==0;contradicted+=type==1;unresolved+=type==2;
    }
    const auto a=reconcile(44,true,routes);
    assert(a.compatible==matched&&a.contradicted==contradicted&&a.unknown==unresolved);
    assert(a.status==(matched?UniverseStatus::OneConditionalSourcePath:
             unresolved?UniverseStatus::UnknownAlternatives:
                        UniverseStatus::ProvidedPathsLocallyContradictedOnly));
    assert(!a.globalInvalidSupported&&!a.globallyLegalSupported);
    ++positives;
    routes[0].routeIdentity=routes[1].routeIdentity;
    assert(reconcile(44,true,routes).status==UniverseStatus::UnknownRouteIdentity);
    ++unknowns;
 }
 std::array<Candidate,17> overflow{};
 for(unsigned i=0;i<overflow.size();++i){overflow[i]=make(Game::Platinum,Family::SinnohHoney);overflow[i].routeIdentity=i+1;}
 assert(reconcile(44,true,overflow).status==UniverseStatus::UnknownCapacity);
 assert(reconcile(0,true,std::span<const Candidate>()).status==UniverseStatus::UnknownRoot);
 assert(reconcile(44,true,std::span<const Candidate>()).status==UniverseStatus::UnknownAlternatives);
 ++unknowns;
 std::cout<<"L1 Gen IV acquisition 65536 malformed game/family domains + 6561 alternative histories: "
          <<positives<<" positive/route checks, "<<local<<" local, "<<unknowns<<" Unknown PASS\n";
}
