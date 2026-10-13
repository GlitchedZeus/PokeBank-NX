#include "Legality/Gen2OriginalRoamerRouteStage.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){
    namespace H=Legality::Gen2OriginalRoamerRouteStage;
    using S=H::Status;
    static_assert(H::kRouteEdges.size()==40);
    static_assert(!H::knownSourceMap(40));
    static_assert(!H::knownSourceMap(41));
    constexpr std::array<const char*,3> games{"gold_gbc","silver_gbc","crystal_gbc"};
    std::size_t positive=0,negative=0,unknown=0;
    for(const char* game:games){
        for(const auto edge:H::kRouteEdges){
            H::Candidate c{};
            c.originalGame=game;
            c.originalSourceMap=edge.from;
            c.originalDestinationMap=edge.to;
            c.originalGameAttested=c.transitionIndependentlyAttested=true;
            c.sourceMapAttested=c.destinationMapAttested=true;
            assert(H::evaluate(c)==S::ConditionalSourceTransitionOnly);
            ++positive;
            auto missing=c;missing.originalGameAttested=false;
            assert(H::evaluate(missing)==S::UnknownOriginalGame);
            missing=c;missing.transitionIndependentlyAttested=false;
            assert(H::evaluate(missing)==S::UnknownTransition);
            missing=c;missing.sourceMapAttested=false;
            assert(H::evaluate(missing)==S::UnknownOriginalSourceMap);
            missing=c;missing.destinationMapAttested=false;
            assert(H::evaluate(missing)==S::UnknownOriginalDestinationMap);
            missing=c;missing.originalDestinationMap=40;
            assert(H::evaluate(missing)==S::UnknownOriginalDestinationMap);
            missing=c;missing.originalSourceMap=41;
            assert(H::evaluate(missing)==S::UnknownOriginalSourceMap);
            unknown+=6;
            bool foundDifferent=false;
            for(uint8_t dest=29;dest<=46;++dest){
                if(!H::knownSourceMap(dest))continue;
                c.originalDestinationMap=dest;
                if(H::evaluate(c)==S::CandidateOriginalTransitionContradicted){
                    foundDifferent=true;break;
                }
            }
            assert(foundDifferent);
            ++negative;
        }
    }
    H::Candidate corrupt{};
    corrupt.originalGame="platinum_ds";
    corrupt.originalGameAttested=true;
    assert(H::evaluate(corrupt)==S::UnknownOriginalGame);
    assert(positive==120&&negative==120&&unknown==720);
    std::cout<<"L1 GSC roamer graph: "<<positive<<" original transition positives, "
             <<negative<<" local negatives, "<<unknown<<" Unknown PASS\n";
}
