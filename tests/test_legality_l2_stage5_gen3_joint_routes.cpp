// Real original Gen III PID, IV and four-move source fixture.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main l2_previous_gen3_source_entrypoint
#include "test_legality_l2_gen3_joint_pid_iv_moves.cpp"
#undef main
#pragma GCC diagnostic pop
#include "Legality/Gen34OriginalSameRouteProof.h"
namespace Route=Legality::Gen34OriginalSameRouteProof;
static void attest(J::Input& in) {
    in.originalEggSnapshotId=0x8899u;
    in.originalPhysicalPairIdentity=0xBEEFCAFEu;
    in.originalEggSnapshotIndependentlyAttested=true;
    in.originalPairIdentityIndependentlyAttested=true;
    in.creationHeldItemsIndependentlyAttested=true;
    in.originalCreationHeldItems={0,0};
    auto& proof=in.sameRouteProof;
    proof.supplied=true;
    for(auto& s:proof.stage)
        s={true,true,true,Route::gameFor(in.pid.game),
           in.originalEggEventIdentity,in.originalEggSnapshotId,
           in.originalPhysicalPairIdentity,
           in.pid.physicalSlots[0].snapshotIdentity,
           in.pid.physicalSlots[1].snapshotIdentity,0,0};
}
int main() {
    unsigned checks=0;
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"})
    for(unsigned seed=0;seed<256;++seed)
    for(bool fatherFirst:{false,true}) {
        auto x=known(game,seed,fatherFirst);
        attest(x);
        const auto positive=J::evaluate(x);
        assert(positive.status==J::Status::AllThreeStagesConditionallyMatched);
        assert(positive.sameRouteCheck==Route::Status::ConditionallySameRoute);
        ++checks;
        auto wrong=x;wrong.sameRouteProof.stage[1].eggSnapshotId++;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.sameRouteProof.stage[2].slot0Id++;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.sameRouteProof.stage[3].eventId++;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.sameRouteProof.stage[0].game=Route::Game::Diamond;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.sameRouteProof.stage[2].held0=229;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.creationHeldItemsIndependentlyAttested=false;
        assert(J::evaluate(wrong).status==J::Status::UnknownOriginalPairEvidence);++checks;
        wrong=x;wrong.sameRouteProof.supplied=false;
        assert(J::evaluate(wrong).status==J::Status::AllThreeStagesConditionallyMatched);++checks;
    }
    assert(checks==20480);
    std::cout<<"L2 Stage5 GBA original same-route triple: "<<checks<<" PASS\n";
}
