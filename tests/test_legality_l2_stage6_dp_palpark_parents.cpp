// Reuse independently attested original DP PID/IV/Move source witness.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main l2_stage6_original_dp_fixture
#include "test_legality_l2_stage5_dp_joint_routes.cpp"
#undef main
#pragma GCC diagnostic pop
namespace Lineage6=Legality::Gen34OriginalParentLineage;
namespace Route6=Legality::Gen34OriginalSameRouteProof;
void attachMigration(J::Input& x,unsigned index){
    auto& witness=x.originalParentLineages[index];
    witness.supplied=true;
    witness.originalEventOrderingIndependentlyAttested=true;
    witness.originalEggEventSequence=10000;
    const auto& m=x.moveHistories[index];
    for(unsigned j=0;j<2;++j){
        auto& p=witness.physicalSlots[j];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.deposited={m.physicalSlots[j].snapshotIdentity,
            Route6::gameFor(x.original.game),
            m.physicalSlots[j].species};
        p.originalOrigin=p.deposited;
    }
    auto& p=witness.physicalSlots[0];
    p.originalOrigin={909,Route6::Game::Ruby,p.deposited.species};
    p.transitions=1;
    p.history[0]={true,true,false,Lineage6::TransitionKind::PalPark,
                   10,p.originalOrigin,p.deposited};
}
int main(){
    unsigned checks=0;
    for(const char* game:{"diamond_nds","pearl_nds"})
    for(unsigned seed=0;seed<256;++seed){
        auto x=joint(known(game,seed));
        attest(x,0);
        attachMigration(x,0);
        const auto yes=J::evaluate(x);
        assert(yes.status==J::Status::OneConditionallyConsistentFullHistory);
        assert(yes.lineageChecks[0]==Lineage6::Status::ConditionallyReplayed);++checks;
        auto wrong=x;wrong.originalParentLineages[0].physicalSlots[0].history[0].before.game=Route6::Game::HeartGold;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);++checks;
        wrong=x;wrong.originalParentLineages[0].physicalSlots[0].history[0].originalSequence=10000;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);++checks;
        wrong=x;wrong.originalParentLineages[0].physicalSlots[0].history[0].after.species++;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);++checks;
        wrong=x;wrong.originalParentLineages[0].physicalSlots[0].history[0].sameIndividualIndependentlyAttested=false;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);++checks;
        wrong=x;wrong.originalParentLineages[0].supplied=false;
        assert(J::evaluate(wrong).status==J::Status::OneConditionallyConsistentFullHistory);++checks;
        wrong=x;wrong.original.count=2;
        wrong.original.alternatives[1]=x.original.alternatives[0];
        wrong.original.alternatives[1].routeIdentity=2;
        wrong.moveHistories[1]=x.moveHistories[0];
        attest(wrong,1);
        attachMigration(wrong,1);
        wrong.originalParentLineages[0].physicalSlots[0].history[0].after.species++;
        const auto rescued=J::evaluate(wrong);
        assert(rescued.status==J::Status::OneConditionallyConsistentFullHistory);
        assert(rescued.matched==1 && rescued.unverified>=1);++checks;
    }
    assert(checks==3584);
    std::cout<<"L2 Stage6 DP imported original parents: "<<checks<<" PASS\n";
}
