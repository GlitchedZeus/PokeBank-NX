#include "Legality/Gen56PkhexOriginalRoute.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
namespace S=Legality::Gen56PkhexSourceCatalog;
namespace L=Legality::Gen56PkhexOriginalRoute;
constexpr std::array<S::Game,8> games{{S::Game::W,S::Game::B,S::Game::W2,S::Game::B2,
                                        S::Game::X,S::Game::Y,S::Game::AS,S::Game::OR}};
L::SourceEvidence witness(S::Game game,uint16_t species,uint16_t level,uint16_t loc=0) {
 L::SourceEvidence e{};
 e.originalSaveId=111;e.originalSpecimenId=222;e.originalGame=game;
 e.originalSaveAndSpecimenIndependentlyAttested=true;
 e.originalGameIndependentlyAttested=true;
 e.originalSpeciesAndLevelIndependentlyAttested=true;
 e.originalSpecies=species;e.originalLevel=level;
 e.originalMetLocation=loc;e.originalMetLocationIndependentlyAttested=true;
 return e;
}
int main(){
 uint64_t checks=0,gameMatches=0,foreign=0,unknown=0;
 static_assert(S::kTables.size()==21 && S::kRows.size()==278);
 std::array<uint16_t,21> seen{};
 for(const auto& r:S::kRows) {
   const auto id=static_cast<unsigned>(r.table);
   assert(id<S::kTables.size());++seen[id];
   assert(r.sourceIndex<S::kTables[id].count);
   assert(r.level>=1&&r.level<=100);
   assert(r.species>=1&&r.species<=(id<13?649:721));
   assert(r.gameMask!=0);
   assert(L::row(r.table,r.sourceIndex)==&r);
   assert(L::tableKind(r.table)==S::kTables[id].kind);
   checks+=7;
   // All game-version intersections can be matched *only as a source
   // template*, with no right to globally accept or reject retail origin.
   for(auto game:games){
      auto e=witness(game,r.species,r.level,r.location);
      e.originalEggLocation=r.eggLocation;
      e.originalEggLocationIndependentlyAttested=true;
      e.originalForm=r.form;e.originalFormIndependentlyAttested=true;
      const auto s=L::evaluate(r,e);
      const bool applicable=(r.gameMask&L::mask(game))!=0;
      assert(s.status==(applicable?L::Status::SourceTemplateOnly:
                                    L::Status::CandidateLocalForeignGame));
      assert(!s.globalInvalidSupported&&!s.globallyLegalSupported);
      if(applicable)++gameMatches;else ++foreign;
      checks+=2;
      if(!applicable)continue;
      auto missing=e;missing.originalGameIndependentlyAttested=false;
      assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalGame);
      ++unknown;
      missing=e;missing.originalSaveAndSpecimenIndependentlyAttested=false;
      assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalSpecimen);
      ++unknown;
      missing=e;missing.originalSpeciesAndLevelIndependentlyAttested=false;
      assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalSpecies);
      ++unknown;
      missing=e;missing.originalLevel=0;
      assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalLevel);
      ++unknown;
      if(r.flags&1u){
        missing=e;missing.originalMetLocationIndependentlyAttested=false;
        assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalLocation);
        ++unknown;
        missing=e;missing.originalMetLocation=static_cast<uint16_t>(e.originalMetLocation+1);
        assert(L::evaluate(r,missing).status==L::Status::CandidateLocalLocationMismatch);
        ++checks;
      }
      if(r.flags&2u){
        missing=e;missing.originalEggLocationIndependentlyAttested=false;
        assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalEggContext);
        ++unknown;
        missing=e;missing.originalEggLocation+=1;
        assert(L::evaluate(r,missing).status==L::Status::CandidateLocalEggMismatch);
        ++checks;
      }
      if(r.flags&4u){
        missing=e;missing.originalFormIndependentlyAttested=false;
        assert(L::evaluate(r,missing).status==L::Status::UnknownOriginalForm);
        ++unknown;
      }
      if(r.shinyPolicy){
        missing=e;missing.originalShinyIndependentlyAttested=true;
        missing.originalShiny=r.shinyPolicy==1u;
        assert(L::evaluate(r,missing).status==L::Status::CandidateLocalShinyMismatch);
        ++checks;
      }
      if(r.flawlessIVs){
        missing=e;missing.originalIVsIndependentlyAttested=true;
        assert(L::evaluate(r,missing).status==L::Status::CandidateLocalFlawlessMismatch);
        ++checks;
        missing.originalIVs.fill(31);
        assert(L::evaluate(r,missing).status==L::Status::SourceTemplateOnly);
        ++checks;
      }
   }
 }
 for(unsigned i=0;i<seen.size();++i)assert(seen[i]==S::kTables[i].count);
 auto b=L::row(S::Table::StaticB,0);
 auto w=L::row(S::Table::StaticW,0);
 assert(b&&b->species==643&&b->level==50&&b->location==45);
 assert(w&&w->species==644&&w->level==50&&w->location==45);
 assert(L::row(S::Table::StaticX,0)->species==716);
 assert(L::row(S::Table::StaticY,0)->species==717);
 assert(L::row(S::Table::Encounter_BW,0)->species==495);
 assert(L::row(S::Table::Encounter_BW,0)->location==4);
 assert(L::row(S::Table::Encounter_B2W2_Regular,0)->location==117);
 assert(L::row(S::Table::Encounter_XY,0)->species==650);
 assert(L::row(S::Table::Encounter_AO_Regular,0)->species==252);
 for(unsigned i=60;i<66;++i) {
   auto cosplay=L::row(S::Table::Encounter_AO_Regular,i);
   assert(cosplay&&cosplay->species==25&&cosplay->level==20);
   assert((cosplay->flags&1u)==0u);
   assert(cosplay->form==(i==65?6:i-59));
 }
 checks+=15;
 // N's B2W2 original fixed encounter table is not interchangeable with BW.
 assert(L::tableCount(S::Game::B2,S::SourceKind::StaticOrGift)==3);
 assert(L::tableCount(S::Game::W,S::SourceKind::StaticOrGift)==2);
 assert(L::row(S::Table::Encounter_B2W2_N,0));
 std::cout<<"L1 Phase A exact Git-pinned PKHeX C# arrays "<<S::kRows.size()
          <<" source rows, "<<gameMatches<<" compatible versions, "<<foreign
          <<" foreign-source candidates, "<<unknown<<" Unknown, "
          <<checks<<" explicit checks PASS\n";
}
