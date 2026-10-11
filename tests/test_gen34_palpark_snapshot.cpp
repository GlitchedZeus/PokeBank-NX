#include "Legality/Gen34PalParkSnapshotTransition.h"
#include <cassert>
#include <iostream>
namespace P=Legality::Gen34PalParkSnapshotTransition;
void runCases() {
    P::Input in{"emerald_gba","platinum_nds",1,
        true,true,true,{33,45,73,22},{33,45,73,22}};
    auto r=P::evaluate(in);
    assert(r.status==P::Status::CandidateConsistent && r.retainedSlots==4);
    // Source HM cannot cross: Surf 57, Cut 15, Flash 148, Waterfall
    // 127, Dive 291 etc. This excludes an explicitly supplied
    // unmodified outgoing snapshot, not the original stored Pokémon.
    for(uint16_t move:{uint16_t(15),uint16_t(19),uint16_t(57),
                       uint16_t(70),uint16_t(148),uint16_t(249),
                       uint16_t(127),uint16_t(291)}) {
        auto hm=in;hm.outboundGbaMoves[0]=move;
        assert(P::evaluate(hm).status==P::Status::CandidateContradicted);
    }
    auto unknownPre=in;unknownPre.independentlyVerifiedPreTransfer=false;
    unknownPre.outboundGbaMoves[0]=57;
    assert(P::evaluate(unknownPre).status==P::Status::UnknownStageWitness);
    auto unknownArrival=in;unknownArrival.independentlyVerifiedImmediateArrival=false;
    assert(P::evaluate(unknownArrival).status==P::Status::UnknownStageWitness);
    auto unknownEvolution=in;unknownEvolution.unchangedSpeciesThroughArrivalVerified=false;
    assert(P::evaluate(unknownEvolution).status==P::Status::UnknownStageWitness);
    auto differentMove=in;differentMove.immediateNdsMoves[0]=99;
    assert(P::evaluate(differentMove).status==P::Status::CandidateContradicted);
    auto postGen4Move=in;postGen4Move.immediateNdsMoves[0]=355;
    assert(P::evaluate(postGen4Move).status==P::Status::CandidateContradicted);
    auto gap=in;gap.outboundGbaMoves={33,0,73,0};
    assert(P::evaluate(gap).status==P::Status::MalformedCandidateSnapshot);
    auto wrongDestination=in;wrongDestination.destinationGame="black_nds";
    assert(P::evaluate(wrongDestination).status==P::Status::UnsupportedGames);
    auto wrongSource=in;wrongSource.sourceGame="heartgold_nds";
    assert(P::evaluate(wrongSource).status==P::Status::UnsupportedGames);
    auto gen4Species=in;gen4Species.species=387;
    assert(P::evaluate(gen4Species).status==P::Status::UnsupportedGames);
    auto empty=in;empty.outboundGbaMoves={};empty.immediateNdsMoves={};
    assert(P::evaluate(empty).status==P::Status::CandidateConsistent);
    auto got2=in;got2.sourceGame="firered_gba";got2.destinationGame="heartgold_nds";
    assert(P::evaluate(got2).status==P::Status::CandidateConsistent);
}
int main(){runCases();std::cout<<"Gen III->IV Pal Park exact move-stage snapshot: PASS\n";}
