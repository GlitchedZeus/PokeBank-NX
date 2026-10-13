#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main l2_original_route_fixture_entrypoint
#include "legality_l2_joint_route_alternatives.cpp"
#undef main
#pragma GCC diagnostic pop
namespace Lineage=Legality::Gen34OriginalParentLineage;
static void provenance(L::Candidate& route,unsigned game,unsigned seed){
    route.originalParentLineage.supplied=true;
    route.originalParentLineage.originalEventOrderingIndependentlyAttested=true;
    route.originalParentLineage.originalEggEventSequence=20000+seed;
    route.depositedSpeciesIndependentlyAttested=true;
    route.depositedSpecies={25,132};
    for(unsigned j=0;j<2;++j){
        auto& p=route.originalParentLineage.physicalSlots[j];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.deposited={j?route.originalIdentity.slot1Id:route.originalIdentity.slot0Id,
             static_cast<P::Game>(game),static_cast<uint16_t>(j?132:25)};
        p.originalOrigin=p.deposited;
    }
}
int main(){
    uint64_t checks=0;
    for(unsigned game=1;game<=10;++game)for(unsigned seed=1;seed<=512;++seed){
        L::Input input{};input.count=1;input.allOriginalRoutesIndependentlyEnumerated=true;
        input.routes[0]=original(game,seed);
        provenance(input.routes[0],game,seed);
        const auto yes=L::evaluate(input);
        assert(yes.status==L::Status::OneConditionallyMatched);
        assert(yes.lineageChecks[0]==Lineage::Status::ConditionallyReplayed);++checks;
        auto bad=input;bad.routes[0].originalParentLineage.physicalSlots[0].deposited.identity++;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        bad=input;bad.routes[0].originalParentLineage.originalEventOrderingIndependentlyAttested=false;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        bad=input;bad.routes[0].depositedSpeciesIndependentlyAttested=false;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        bad=input;bad.routes[0].originalParentLineage.physicalSlots[0].originalOrigin.game=P::Game::Unknown;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        bad=input;bad.routes[0].stageVerdicts[0]=L::Verdict::Contradicted;
        assert(L::evaluate(bad).status==L::Status::AllAttestedLocalHistoriesContradicted);++checks;
        bad=input;bad.routes[0].stageVerdicts[0]=L::Verdict::Contradicted;
        bad.routes[0].originalParentLineage.physicalSlots[0].deposited.identity++;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        bad=input;bad.count=2;bad.routes[1]=input.routes[0];bad.routes[1].routeId++;
        bad.routes[0].originalParentLineage.physicalSlots[0].deposited.identity++;
        assert(L::evaluate(bad).status==L::Status::OneConditionallyMatched);++checks;
        bad.routes[1].originalParentLineage.physicalSlots[0].deposited.identity++;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
        bad=input;bad.count=2;bad.routes[1]=bad.routes[0];
        bad.routes[0].stageVerdicts[0]=L::Verdict::Contradicted;
        bad.routes[1].stageVerdicts[1]=L::Verdict::Contradicted;
        assert(L::evaluate(bad).status==L::Status::UnverifiedOrDuplicateSources);++checks;
    }
    assert(checks==51200);
    std::cout<<"L2 Stage6 lineage candidate alternatives: "<<checks<<" PASS\n";
}
