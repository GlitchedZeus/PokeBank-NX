#include "Legality/Gen34OriginalJointRouteLedger.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen34OriginalJointRouteLedger;
namespace P=Legality::Gen34OriginalSameRouteProof;
L::Candidate original(unsigned game,uint64_t seed){
    L::Candidate c{};c.routeId=seed+1;
    c.originalIdentity={static_cast<P::Game>(game),seed+1,seed+100,
        seed+200,seed+300,seed+400,true,229,236};
    c.originalStages.supplied=true;
    for(auto& s:c.originalStages.stage)
        s={true,true,true,c.originalIdentity.game,c.originalIdentity.eventId,
           c.originalIdentity.eggSnapshotId,c.originalIdentity.pairId,
           c.originalIdentity.slot0Id,c.originalIdentity.slot1Id,229,236};
    c.stageVerdicts.fill(L::Verdict::Matched);
    c.allMethodsForThisRouteIndependentlyCovered=true;
    return c;
}
int main(){
    uint64_t checks=0;
    for(unsigned game=1;game<=10;++game)for(uint64_t seed=1;seed<=1024;++seed){
        L::Input in{};in.count=1;in.allOriginalRoutesIndependentlyEnumerated=true;
        in.routes[0]=original(game,seed);
        assert(L::evaluate(in).status==L::Status::OneConditionallyMatched);++checks;
        for(unsigned j=0;j<4;++j){
            auto v=in;v.routes[0].stageVerdicts[j]=L::Verdict::Contradicted;
            assert(L::evaluate(v).status==L::Status::AllAttestedLocalHistoriesContradicted);++checks;
            v.routes[0].stageVerdicts[(j+1)%4]=L::Verdict::Unknown;
            assert(L::evaluate(v).status==L::Status::UnresolvedSourceHistory);++checks;
            v=in;v.routes[0].stageVerdicts[j]=L::Verdict::Contradicted;
            v.routes[0].allMethodsForThisRouteIndependentlyCovered=false;
            assert(L::evaluate(v).status==L::Status::UnresolvedSourceHistory);++checks;
        }
        auto alternatives=in;alternatives.count=2;
        alternatives.routes[1]=alternatives.routes[0];
        alternatives.routes[0].stageVerdicts[0]=L::Verdict::Contradicted;
        alternatives.routes[1].stageVerdicts[1]=L::Verdict::Contradicted;
        assert(L::evaluate(alternatives).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        alternatives.routes[1].routeId++;
        assert(L::evaluate(alternatives).status==L::Status::AllAttestedLocalHistoriesContradicted);++checks;
        alternatives.routes[1].stageVerdicts[1]=L::Verdict::Unknown;
        assert(L::evaluate(alternatives).status==L::Status::UnresolvedSourceHistory);++checks;
        alternatives.routes[1].stageVerdicts.fill(L::Verdict::Matched);
        assert(L::evaluate(alternatives).status==L::Status::OneConditionallyMatched);++checks;
        alternatives.routes[0].originalStages.stage[2].eventId++;
        assert(L::evaluate(alternatives).status==L::Status::OneConditionallyMatched);++checks;
        alternatives.routes[1].originalStages.stage[3].eventId++;
        assert(L::evaluate(alternatives).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        auto noHistory=in;noHistory.allOriginalRoutesIndependentlyEnumerated=false;
        noHistory.routes[0].stageVerdicts[0]=L::Verdict::Contradicted;
        assert(L::evaluate(noHistory).status==L::Status::UnknownExhaustiveEnumeration);++checks;
        noHistory=in;noHistory.routes[0].originalIdentity.itemsKnown=false;
        assert(L::evaluate(noHistory).status==L::Status::UnverifiedOrDuplicateSources);++checks;
    }
    L::Input invalid{};
    assert(L::evaluate(invalid).status==L::Status::NoCandidates);++checks;
    invalid.count=9;
    assert(L::evaluate(invalid).status==L::Status::CapacityUnknown);++checks;
    assert(checks==215042);
    std::cout<<"L2 Stage5 bounded original alternative routes: "<<checks<<" PASS\n";
}
