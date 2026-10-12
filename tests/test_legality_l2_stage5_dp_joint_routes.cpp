// Reuse the source-real Diamond/Pearl PID, IV and four-move fixture.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main l2_previous_dp_regression_entrypoint
#include "test_legality_l2_dp_alternative_egg_histories.cpp"
#undef main
#pragma GCC diagnostic pop
#include "Legality/Gen34OriginalSameRouteProof.h"
namespace Route=Legality::Gen34OriginalSameRouteProof;
static void attest(J::Input& input,unsigned index){
    auto& c=input.original.alternatives[index];
    auto& src=c.pid.originalParentWitness;
    src.supplied=true;
    src.originalDaycareEventIndependentlyVerified=true;
    src.physicalDepositOrderIndependentlyVerified=true;
    src.originalPairIdentityIndependentlyVerified=true;
    src.originalParentSnapshotsIndependentlyVerified=true;
    src.creationItemsIndependentlyVerified=true;
    src.collectionLanguagesIndependentlyVerified=true;
    src.originalEggEventIdentity=input.original.originalEggEventIdentity;
    src.originalPhysicalPairIdentity=c.ivPhysicalParentPairIdentity;
    src.physicalSlots[0]={101,25,1,7,229,2};
    src.physicalSlots[1]={202,25,0,11,0,2};
    input.originalEggSnapshotId=0x553311u;
    input.originalEggSnapshotIndependentlyAttested=true;
    auto& proof=input.sameRouteProofs[index];
    proof.supplied=true;
    for(auto& s:proof.stage)
        s={true,true,true,Route::gameFor(input.original.game),
            input.original.originalEggEventIdentity,input.originalEggSnapshotId,
            c.ivPhysicalParentPairIdentity,101,202,229,0};
}
int main(){
    unsigned checks=0;
    for(const char* game:{"diamond_nds","pearl_nds"})
    for(unsigned seed=0;seed<1024;++seed){
        auto in=joint(known(game,seed));
        attest(in,0);
        const auto positive=J::evaluate(in);
        assert(positive.status==J::Status::OneConditionallyConsistentFullHistory);
        assert(positive.sameRouteChecks[0]==Route::Status::ConditionallySameRoute);
        ++checks;
        auto wrong=in;
        wrong.sameRouteProofs[0].stage[2].eventId++;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);
        ++checks;
        wrong=in;wrong.sameRouteProofs[0].stage[3].slot0Id++;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);
        ++checks;
        wrong=in;wrong.sameRouteProofs[0].stage[1].held0=0;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);
        ++checks;
        wrong=in;wrong.sameRouteProofs[0].stage[0].game=Route::Game::Platinum;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);
        ++checks;
        wrong=in;wrong.originalEggSnapshotIndependentlyAttested=false;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);
        ++checks;
        wrong=in;wrong.original.alternatives[0].pid.originalParentWitness.supplied=false;
        assert(J::evaluate(wrong).status==J::Status::UnverifiedOriginalParentSources);
        ++checks;
        wrong=in;wrong.original.count=2;
        wrong.original.alternatives[1]=in.original.alternatives[0];
        wrong.original.alternatives[1].routeIdentity=2;
        wrong.moveHistories[1]=in.moveHistories[0];
        attest(wrong,1);
        wrong.sameRouteProofs[0].stage[0].eventId++;
        const auto recovered=J::evaluate(wrong);
        assert(recovered.status==J::Status::OneConditionallyConsistentFullHistory);
        assert(recovered.matched==1 && recovered.unverified>=1);
        ++checks;
    }
    assert(checks==16384);
    std::cout<<"L2 Stage5 DP same-route integration: "<<checks<<" PASS\n";
}
