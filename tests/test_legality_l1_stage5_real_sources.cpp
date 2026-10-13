#include "Legality/Gen14OriginalHistoryLedger.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace Legality::Gen14OriginalHistoryLedger;
using namespace Legality::Gen14OriginalEvidenceBridge;
constexpr Witness witness(uint64_t id,Family f,Stage stage,uint32_t gameId){
 Witness w{};w.originalSpecimenRoot=0xDEC0DE;w.routeId=id;
 w.originalGameId=gameId;w.originalSaveId=id*100;
 w.family=f;w.stage=stage;w.evidence=Class::Unknown;
 w.rootIdentityIndependentlyAttested=true;
 w.originalGameAndSaveIndependentlyAttested=true;
 w.sourceMethodIndependentlyAttested=true;
 return w;
}
int main(){
 // Real Stage 1 source-table evaluator, all 149 physical rows and all game
 // combinations, not hardcoded "favorable" statuses.
 uint32_t pos1=0,neg1=0;
 for(std::size_t i=0;i<G1::kVerifiedSlots.size();++i){
   const auto& s=G1::kVerifiedSlots[i];
   for(uint8_t game=1;game<=4;game*=2){
     G1::Candidate c{};
     c.originalGame=game==1?"red_gb":game==2?"blue_gb":"yellow_gb";
     c.originalSourceIndex=i;c.originalMap=s.map;
     c.originalMethod=s.method;c.originalSpecies=s.species;c.originalLevel=s.level;
     c.originalGameAttested=true;c.originalSourceSlotAttested=true;
     c.originalMapAttested=true;c.originalMethodAttested=true;
     c.originalSpeciesAttested=true;c.originalLevelAttested=true;
     const auto kind=map(G1::evaluate(c).status);
     if(game&s.gameMask){assert(kind==Class::SourceOnly);++pos1;
       auto bad=c;bad.originalSpecies=static_cast<uint16_t>(s.species==1?2:1);
       assert(map(G1::evaluate(bad).status)==Class::CandidateLocalContradiction);++neg1;
       bad=c;bad.originalLevelAttested=false;
       assert(map(G1::evaluate(bad).status)==Class::Unknown);
     }else assert(kind==Class::Unknown);
   }
 }
 assert(pos1==268&&neg1==268);
 // Real G/S/C fishing physical rows: day/night slot provenance stays separate.
 uint32_t f2=0;
 for(std::size_t i=0;i<F2::kOriginalSlots.size();++i){
   const auto& s=F2::kOriginalSlots[i];
   F2::Candidate c{};c.exactOriginalGame="gold_gbc";c.originalRowIndex=i;
   c.originalFishGroup=s.fishGroup;c.originalRod=s.rod;
   c.originalNight=s.phase==F2::DayPhase::Night;
   c.originalSwarmActive=s.fishGroup==5||s.fishGroup==6;
   c.originalSpecies=s.species;c.originalLevel=s.level;
   c.gameAttested=true;c.rowAttested=true;c.groupAttested=true;
   c.rodAttested=true;c.dayNightAttested=true;c.swarmStateAttested=true;
   c.speciesAttested=true;c.levelAttested=true;
   assert(map(F2::evaluate(c).status)==Class::SourceOnly);
   ++f2;
   auto bad=c;bad.originalSpecies=static_cast<uint16_t>(s.species==1?2:1);
   assert(map(F2::evaluate(bad).status)==Class::CandidateLocalContradiction);
 }
 assert(f2==F2::kOriginalSlots.size());
 // Gen III bounded source catalog: its 306 matched source rows remain mere
 // source compatibility even when every table row is positively witnessed.
 uint32_t wild3=0;
 for(std::size_t i=0;i<W3::kGroups.size();++i){
   const auto& g=W3::kGroups[i];
   for(std::size_t j=0;j<W3::slotCount(g);++j){
     const auto row=W3::decode(g,j);
     W3::Candidate c{};c.originalGame=g.game;
     c.exactGroupIndex=i;c.physicalSlot=j;c.originalMap=g.map;
     c.originalMethod=g.method;c.originalRod=g.method==W3::Method::Fishing?
       W3::requiredRod(j):W3::Rod::Unknown;
     c.originalSpecies=W3::kSpecies[row.speciesIndex];c.originalLevel=row.minLevel;
     c.originalGameAttested=true;c.originalGroupAndSlotAttested=true;
     c.originalMapAndMethodAttested=true;c.originalSpeciesAttested=true;
     c.originalLevelAttested=true;c.originalRodAttested=true;
     assert(map(W3::evaluate(c).status)==Class::SourceOnly);
     auto malformed=c;malformed.originalGame=W3::Game::Unknown;
     assert(map(W3::evaluate(malformed).status)==Class::Unknown);
     ++wild3;
   }
 }
 assert(wild3==306);
 // Original capture Method J/K, independent original pre-nature RNG witness.
 JK4::Candidate j{};j.originalGame=JK4::Game::Platinum;
 j.originalGameIndependentlyAttested=true;
 j.historicalLead=JK4::Lead::NoLead;j.originalLeadIndependentlyAttested=true;
 j.originalNatureStageIndependentlyAttested=true;
 j.originalPidSnapshotIndependentlyAttested=true;
 j.maxPidPairs=4096;
 bool found=false;
 for(uint32_t seed=0;seed<2000&&!found;++seed){
    j.witnessedPreNatureState=seed;
    j.witnessedTargetNature=JK4::rollNature(j.originalGame,
                              static_cast<uint16_t>(JK4::next(seed)>>16));
    uint32_t v=JK4::next(seed);
    for(int t=0;t<4096;++t){
      v=JK4::next(v);const uint32_t lo=v>>16;
      v=JK4::next(v);const uint32_t pid=(v&0xFFFF0000u)|lo;
      if(pid%25u==j.witnessedTargetNature){j.originalPid=pid;found=true;break;}
    }
 }
 assert(found&&map(JK4::evaluate(j).status)==Class::RngStageOnly);
 const auto rngOnly=witness(5,Family::Gen4Sinnoh,Stage::RngNaturePid,40);
 // HGSS known tree group (original ID + observed physical group) is a source,
 // never a complete Method K wild history.
 Tree4::Candidate tree{};
 tree.exactOriginalGame="heartgold_nds";tree.originalFullTrainerId=0xABCD1234;
 tree.originalDesignatedTreeCount=5;tree.originalTreeId=1;
 tree.originalSaveIndependentlyAttested=true;
 tree.fullTrainerIdIndependentlyAttested=true;
 tree.originalTreeListIndependentlyAttested=true;
 tree.physicalTreeIdIndependentlyAttested=true;
 tree.originalGroupIndependentlyAttested=true;
 tree.witnessedGroup=Tree4::groupFor(5,1,tree.originalFullTrainerId);
 const auto treeStatus=Tree4::evaluate(tree).status;
 assert(map(treeStatus)==Class::SourceOnly ||
        map(treeStatus)==Class::CandidateLocalContradiction);
 // GameCube native stage cannot reconstruct Colo versus XD from PK3 game=15.
 GC3::Candidate gc{};gc.originalGame=GC3::Game::XD;
 gc.originalRoute=GC3::Route::ShadowPurified;
 gc.originalGameIndependentlyAttested=true;gc.originalSceneIndependentlyAttested=true;
 gc.originalPurificationIndependentlyAttested=true;
 gc.originalCapturedPidSpeciesOtIndependentlyAttested=true;
 gc.originalSpecies=197;gc.originalPersonality=0x12345678;
 gc.originalTrainerId=0xAABBCCDD;
 gc.native={gc.originalPersonality,gc.originalTrainerId,197,15,{1,2,3,4,5,6},true};
 assert(map(GC3::evaluate(gc).status)==Class::ConditionalNative);
 auto noScene=gc;noScene.originalPurificationIndependentlyAttested=false;
 assert(map(GC3::evaluate(noScene).status)==Class::Unknown);
 // Same original specimen root with multiple competing *possible* original
 // games: a source-match GC route must not erase an alternative unknown egg.
 std::array<Witness,5> alts{{
    withEvidence(witness(1,Family::Gen1Wild,Stage::PhysicalSource,1),
       G1::Status::CandidateOriginalSpeciesContradicted),
    withEvidence(witness(2,Family::Gen2Fishing,Stage::PhysicalSource,2),
       F2::Status::SourceSlotCompatibleWorldAndRngUnverified),
    withEvidence(witness(3,Family::Gen3Wild,Stage::PhysicalSource,3),
       W3::Status::SourceTemplateCompatibleHistoryUnverified),
    withEvidence(witness(4,Family::Gen3GameCube,Stage::NativeOriginalIdentity,15),
       GC3::evaluate(gc).status),
    withEvidence(witness(5,Family::Gen4Sinnoh,Stage::RngNaturePid,40),
       JK4::evaluate(j).status)
 }};
 assert(rngOnly.originalSpecimenRoot==alts[0].originalSpecimenRoot);
 auto result=summarize(0xDEC0DE,true,alts);
 assert(result.verdict==Verdict::ConditionalOriginalCandidateOnly);
 assert(result.candidateCompatible==4&&result.candidateContradicted==1);
 assert(result.noGlobalDecision());
 alts[4].family=Family::EggGiftTradeEventTransfer;
 alts[4].evidence=Class::Unknown;
 alts[4].stage=Stage::Other;
 result=summarize(0xDEC0DE,true,alts);
 assert(result.verdict==Verdict::UnknownAlternativeHistories);
 assert(result.noGlobalDecision());
 std::cout<<"L1 S5 real source fixtures: G1="<<pos1<<", G2 fish="<<f2
          <<", G3 wild="<<wild3<<", native GC/Gen4 stage and alternatives PASS\n";
}
