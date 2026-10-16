// Production-source B07/B08 original PID/IV/four-move fixture.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main l2_stage6_original_gen3_fixture
#include "test_legality_l2_gen3_joint_pid_iv_moves.cpp"
#undef main
#pragma GCC diagnostic pop
namespace Lineage6=Legality::Gen34OriginalParentLineage;
namespace Route6=Legality::Gen34OriginalSameRouteProof;
void attachTrade(J::Input& candidate){
    auto& in=candidate.originalParentLineage;
    in.supplied=true;
    in.originalEventOrderingIndependentlyAttested=true;
    in.originalEggEventSequence=10000;
    for(unsigned s=0;s<2;++s){
        const auto& actual=candidate.pid.physicalSlots[s];
        auto& p=in.physicalSlots[s];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.originalOrigin={500+s,Route6::gameFor(candidate.pid.game),actual.species};
        p.deposited={actual.snapshotIdentity,Route6::gameFor(candidate.pid.game),actual.species};
        p.transitions=1;
        p.history[0]={true,true,false,Lineage6::TransitionKind::Trade,5,
                      p.originalOrigin,p.deposited};
    }
}
int main(){
    unsigned checks=0;
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"})
    for(uint32_t seed=0;seed<256;++seed)
    for(bool fatherFirst:{false,true}){
        auto x=known(game,seed,fatherFirst);
        attachTrade(x);
        const auto yes=J::evaluate(x);
        assert(yes.status==J::Status::AllThreeStagesConditionallyMatched);
        assert(yes.lineageStatus==Lineage6::Status::ConditionallyReplayed);++checks;
        auto wrong=x;wrong.originalParentLineage.physicalSlots[0].history[0].before.identity++;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.originalParentLineage.physicalSlots[1].history[0].originalSequence=10000;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.originalParentLineage.physicalSlots[0].deposited.identity++;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.originalParentLineage.physicalSlots[0].history[0].after.game=Route6::Game::Diamond;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.originalParentLineage.physicalSlots[1].history[0].sameIndividualIndependentlyAttested=false;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.originalParentLineage.supplied=false;
        assert(J::evaluate(wrong).status==J::Status::AllThreeStagesConditionallyMatched);++checks;
    }
    assert(checks==17920);
    std::cout<<"L2 Stage6 GenIII traded original parents: "<<checks<<" PASS\n";
}
