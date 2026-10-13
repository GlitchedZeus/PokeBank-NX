#include "Legality/Gen1PretEvolutionStage.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen1PretEvolutionStage;
using S=G::Status;
bool independentlyWalk(uint16_t from,uint16_t to,uint8_t mask) {
 if(from==to)return true;
 for(const auto& edge:G::kPretEvolutionEdges) {
   if(edge.originalSpecies!=from||!(edge.gameMask&mask))continue;
   if(edge.evolvedSpecies==to)return true;
   for(const auto& second:G::kPretEvolutionEdges)
     if(second.originalSpecies==edge.evolvedSpecies &&
        second.evolvedSpecies==to && (second.gameMask&mask))
         return true;
 }
 return false;
}
int main(){
 static_assert(G::kPretEvolutionEdges.size()==72);
 uint64_t positive=0,negative=0,unknown=0;
 for(const auto& e:G::kPretEvolutionEdges){
   assert(e.originalSpecies>=1&&e.originalSpecies<=151);
   assert(e.evolvedSpecies>=1&&e.evolvedSpecies<=151);
   assert(e.originalSpecies!=e.evolvedSpecies);
   assert(e.minimumEvolutionLevel>=1&&e.minimumEvolutionLevel<=100);
   assert(e.gameMask==7);
   assert(e.method==G::PretEvolutionMethod::Level ||
          e.method==G::PretEvolutionMethod::Item ||
          e.method==G::PretEvolutionMethod::Trade);
   assert(G::reachable(e.originalSpecies,e.evolvedSpecies,1));
   assert(G::reachable(e.originalSpecies,e.evolvedSpecies,2));
   assert(G::reachable(e.originalSpecies,e.evolvedSpecies,4));
   positive+=9;
 }
 for(unsigned g=0;g<3;++g){
    const uint8_t mask=static_cast<uint8_t>(1u<<g);
    const char* title=g==0?"red_gb":g==1?"blue_gb":"yellow_gb";
    for(uint16_t from=1;from<=151;++from){
      for(uint16_t to=1;to<=151;++to){
        const bool target=independentlyWalk(from,to,mask);
        assert(G::reachable(from,to,mask)==target);
        G::Witness w{};
        w.originalGame=title;w.originalSpecies=from;w.currentSpecies=to;
        w.originalGameIndependentlyAttested=true;
        w.originalSpeciesIndependentlyAttested=true;
        w.currentSpeciesIndependentlyAttested=true;
        w.evolutionHistoryIndependentlyAttested=true;
        auto result=G::evaluate(w);
        assert(result.status==(target?S::SourceGraphCompatibleHistoryUnverified:
                                      S::CandidateLocalAncestryMismatch));
        assert(!result.globallyLegalSupported&&!result.globalInvalidSupported);
        if(target)positive+=3;else negative+=3;
        if(from==to)continue;
        w.evolutionHistoryIndependentlyAttested=false;
        assert(G::evaluate(w).status==S::UnknownIndependentlyAttestedLineage);
        ++unknown;
      }
    }
 }
 // Eevee's branches are siblings and cannot become one another.
 assert(G::reachable(133,134,1)&&G::reachable(133,135,2)&&G::reachable(133,136,4));
 assert(!G::reachable(134,135,1)&&!G::reachable(136,134,4));
 assert(G::reachable(10,12,1)&&G::reachable(13,15,2));
 assert(!G::reachable(13,12,1)&&!G::reachable(128,130,1));
 assert(G::reachable(67,68,4)&&G::reachable(92,94,1));
 assert(!G::reachable(137,233,1)); // later Gen II Porygon2 => Unknown elsewhere
 auto missing=G::Witness{};
 missing.originalGame="yellow_gb";missing.originalSpecies=133;
 missing.currentSpecies=135;
 assert(G::evaluate(missing).status==S::UnknownOriginalGame);
 missing.originalGameIndependentlyAttested=true;
 assert(G::evaluate(missing).status==S::UnknownOriginalSpecies);
 missing.originalSpeciesIndependentlyAttested=true;
 assert(G::evaluate(missing).status==S::UnknownCurrentSpecies);
 missing.currentSpecies=196;missing.currentSpeciesIndependentlyAttested=true;
 assert(G::evaluate(missing).status==S::UnknownLaterGenerationEvolution);
 unknown+=4;
 for(unsigned raw=0;raw<256;++raw){
   const uint8_t mask=static_cast<uint8_t>(raw);
   if(mask==1||mask==2||mask==4)continue;
   assert(!G::reachable(133,135,mask));
   assert(!G::reachable(133,133,mask));
   ++unknown;
 }
 std::cout<<"L1 exact original pret RBY 72 evolution edges, 151x151x3: "
          <<positive<<" graph-compatible, "<<negative
          <<" candidate-local and "<<unknown<<" Unknown/bounds checks PASS\n";
}
