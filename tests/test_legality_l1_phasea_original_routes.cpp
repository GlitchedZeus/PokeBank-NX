#include "Legality/Gen56PkhexOriginalRoute.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P=Legality::Gen56PkhexSourceCatalog;
namespace L=Legality::Gen56PkhexOriginalRoute;
using P::Game;using P::Table;using P::SourceKind;
L::SourceEvidence get(Game g,uint16_t species,uint16_t level,uint16_t location) {
 L::SourceEvidence s{};
 s.originalSaveId=333;s.originalSpecimenId=444;
 s.originalSaveAndSpecimenIndependentlyAttested=true;
 s.originalGameIndependentlyAttested=true;
 s.originalGame=g;
 s.originalSpeciesAndLevelIndependentlyAttested=true;
 s.originalSpecies=species;s.originalLevel=level;
 s.originalMetLocation=location;
 s.originalMetLocationIndependentlyAttested=true;
 return s;
}
void expect(Game g,SourceKind kind,uint16_t species,uint16_t level,
            uint16_t loc,Table table,uint16_t row){
 const auto r=L::collect(get(g,species,level,loc),kind);
 assert(r.status==L::ScanStatus::SourceCandidatesOnly);
 bool got=false;
 for(uint8_t i=0;i<r.matched;++i){
    if(r.hits[i].table==table && r.hits[i].sourceIndex==row) {
       assert(r.hits[i].localStatus==L::Status::SourceTemplateOnly);
       got=true;
    }
 }
 assert(got&&!r.exhaustiveOriginalHistories);
 assert(!r.globalInvalidSupported&&!r.globallyLegalSupported);
}
int main(){
 uint64_t checks=0;
 // Table selection is PKHeX EncounterPossible5/6's actual source-version
 // separation; different game legends and gift locations must NOT merge.
 expect(Game::B,SourceKind::StaticOrGift,643,50,45,Table::StaticB,0);
 expect(Game::W,SourceKind::StaticOrGift,644,50,45,Table::StaticW,0);
 expect(Game::B2,SourceKind::StaticOrGift,644,70,39,Table::StaticB2,4);
 expect(Game::W2,SourceKind::StaticOrGift,643,70,39,Table::StaticW2,4);
 expect(Game::B,SourceKind::StaticOrGift,495,5,4,Table::Encounter_BW,0);
 expect(Game::B2,SourceKind::StaticOrGift,495,5,117,Table::Encounter_B2W2_Regular,0);
 expect(Game::X,SourceKind::StaticOrGift,716,50,138,Table::StaticX,0);
 expect(Game::Y,SourceKind::StaticOrGift,717,50,138,Table::StaticY,0);
 expect(Game::X,SourceKind::StaticOrGift,650,5,10,Table::Encounter_XY,0);
 expect(Game::AS,SourceKind::StaticOrGift,380,30,320,Table::StaticA,0);
 expect(Game::OR,SourceKind::StaticOrGift,381,30,320,Table::StaticO,0);
 expect(Game::OR,SourceKind::StaticOrGift,252,5,204,Table::Encounter_AO_Regular,0);
 expect(Game::B,SourceKind::NpcTrade,548,15,0,Table::TradeGift_B,0);
 expect(Game::W,SourceKind::NpcTrade,546,15,0,Table::TradeGift_W,0);
 expect(Game::B2,SourceKind::NpcTrade,548,20,0,Table::TradeGift_B2,0);
 expect(Game::W2,SourceKind::NpcTrade,546,20,0,Table::TradeGift_W2,0);
 expect(Game::X,SourceKind::NpcTrade,129,5,0,Table::TradeGift_XY,0);
 expect(Game::OR,SourceKind::NpcTrade,296,9,0,Table::TradeGift_AO,0);
 checks+=18;
 // Missing a original species capture/source version cannot turn *any* of
 // those partial static/trade table misses into global Invalid.
 auto incompatible=L::collect(get(Game::W,643,50,45),SourceKind::StaticOrGift);
 assert(incompatible.status==L::ScanStatus::UnknownUnmodeledAlternatives);
 assert(incompatible.matched==0);checks+=2;
 auto otherVersion=L::collect(get(Game::W2,643,50,45),SourceKind::StaticOrGift);
 assert(otherVersion.status==L::ScanStatus::UnknownUnmodeledAlternatives);
 ++checks;
 auto unknown=get(Game::AS,380,30,320);
 unknown.originalGameIndependentlyAttested=false;
 assert(L::collect(unknown,SourceKind::StaticOrGift).status==
        L::ScanStatus::UnknownOriginalGame);++checks;
 unknown=get(Game::AS,380,30,320);
 unknown.originalSaveAndSpecimenIndependentlyAttested=false;
 assert(L::collect(unknown,SourceKind::StaticOrGift).status==
        L::ScanStatus::UnknownOriginalSource);++checks;
 unknown=get(Game::AS,380,30,320);
 unknown.originalMetLocationIndependentlyAttested=false;
 auto partial=L::collect(unknown,SourceKind::StaticOrGift);
 assert(partial.status==L::ScanStatus::SourceCandidatesOnly);
 assert(partial.unresolved>=1);
 assert(!partial.exhaustiveOriginalHistories);
 checks+=3;
 // Shiny/IV original histories cannot be borrowed from a detached PKM.
 auto item=L::row(Table::StaticX,0);
 assert(item && item->shinyPolicy==1&&item->flawlessIVs==3);
 auto spec=get(Game::X,716,50,138);
 spec.originalShinyIndependentlyAttested=true;spec.originalShiny=true;
 assert(L::evaluate(*item,spec).status==L::Status::CandidateLocalShinyMismatch);
 spec.originalShiny=false;spec.originalIVsIndependentlyAttested=true;
 assert(L::evaluate(*item,spec).status==L::Status::CandidateLocalFlawlessMismatch);
 spec.originalIVs.fill(31);
 assert(L::evaluate(*item,spec).status==L::Status::SourceTemplateOnly);
 checks+=4;
 // Gen V original Tornadus/Thundurus are roaming sources, so matching
 // the literal placeholder location (25) is not required or certified.
 auto roamer=L::row(Table::StaticB,2);
 assert(roamer&&(roamer->flags&16u));
 auto roamerWitness=get(Game::B,641,40,999);
 assert(L::evaluate(*roamer,roamerWitness).status==L::Status::SourceTemplateOnly);
 checks+=2;
 // Gen V/VI gift egg hatch locations are separately witnessed histories.
 auto eggRow=L::row(Table::Encounter_BW,16);
 assert(eggRow && (eggRow->flags&2u));
 auto eggWitness=get(Game::W,eggRow->species,eggRow->level,999);
 eggWitness.originalEggLocation=eggRow->eggLocation;
 eggWitness.originalEggLocationIndependentlyAttested=true;
 assert(L::evaluate(*eggRow,eggWitness).status==L::Status::SourceTemplateOnly);
 checks+=2;
 // ORAS Cosplay Pikachu location is dynamic; do NOT certify exact met map.
 spec=get(Game::OR,25,20,999);
 spec.originalForm=1;spec.originalFormIndependentlyAttested=true;
 auto cosplay=L::row(Table::Encounter_AO_Regular,60);
 assert(cosplay && (cosplay->flags&8));
 assert(L::evaluate(*cosplay,spec).status==L::Status::SourceTemplateOnly);
 checks+=2;
 // 256 original-version bytes, all invented generation domains, and both
 // documented category selectors. Unknown domains cannot manufacture a
 // positive or an exhaustive "zero original source" conclusion.
 for(unsigned byte=0;byte<256;++byte){
   const auto game=static_cast<Game>(byte);
   auto s=get(game,643,50,45);
   for(unsigned rawKind=0;rawKind<256;++rawKind){
      auto kind=static_cast<SourceKind>(rawKind);
      const auto result=L::collect(s,kind);
      assert(!result.globalInvalidSupported&&!result.globallyLegalSupported);
      assert(!result.exhaustiveOriginalHistories);
      if(!L::validGame(game))assert(result.status==L::ScanStatus::UnknownOriginalGame);
      else if(rawKind>1)assert(result.status==L::ScanStatus::UnknownOriginalSource);
      ++checks;
   }
 }
 std::cout<<"L1 PKHeX Gen V/VI exact title array dispatch, 256x256 adversarial inputs: "
          <<checks<<" tests PASS; zero rows never global Invalid\n";
}
