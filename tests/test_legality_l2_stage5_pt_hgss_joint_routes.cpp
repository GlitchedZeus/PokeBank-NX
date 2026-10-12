// Import the existing source-backed Platinum/HGSS full egg fixture.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main l2_previous_pt_hgss_regression_entrypoint
#include "test_legality_l2_gen4_pt_hgss_full_moves.cpp"
#undef main
#pragma GCC diagnostic pop
#include "Legality/Gen4PlatinumHGSSSourceBoundEggHistories.h"
namespace Bound=Legality::Gen4PlatinumHGSSSourceBoundEggHistories;
namespace Route=Legality::Gen34OriginalSameRouteProof;
static void attest(Bound::Input& input,unsigned index){
    const auto& c=input.full.original.alternatives[index];
    auto& a=input.archives[index];
    a.supplied=true;
    a.originalEventIndependentlyAttested=true;
    a.originalPairIdentityIndependentlyAttested=true;
    a.physicalDepositOrderIndependentlyAttested=true;
    a.bothParentSnapshotsIndependentlyAttested=true;
    a.creationHeldItemsIndependentlyAttested=true;
    a.collectionLanguagesIndependentlyAttested=true;
    a.parentPersonalitiesIndependentlyAttested=true;
    a.game=input.full.original.game;
    a.originalEggEvent=input.full.original.originalEggEventIdentity;
    a.physicalPairIdentity=c.originalDaycarePairIdentity;
    for(unsigned slot=0;slot<2;++slot) {
        const auto& p=c.pid.parents[slot];
        a.originalPhysicalSlots[slot]={p.snapshotId,p.species,p.heldItem,
            p.gender,c.pid.originalParentLanguages[slot],p.originalPID};
    }
    input.originalEggSnapshotId=0x337755u;
    input.originalEggSnapshotIndependentlyAttested=true;
    auto& proof=input.sameRouteProofs[index];
    proof.supplied=true;
    for(auto& s:proof.stage)
        s={true,true,true,Route::gameFor(input.full.original.game),
           input.full.original.originalEggEventIdentity,
           input.originalEggSnapshotId,c.originalDaycarePairIdentity,
           c.pid.parents[0].snapshotId,c.pid.parents[1].snapshotId,
           c.pid.parents[0].heldItem,c.pid.parents[1].heldItem};
}
int main(){
    unsigned checks=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"})
    for(unsigned seed=0;seed<512;++seed){
        Bound::Input input{};
        input.full=source(game,seed,229,294);
        input.allArchivesIndependentlyEnumerated=true;
        attest(input,0);
        auto yes=Bound::evaluate(input);
        assert(yes.status==Bound::Status::OneMatched);
        assert(yes.sameRouteChecks[0]==Route::Status::ConditionallySameRoute);
        ++checks;
        auto bad=input;bad.sameRouteProofs[0].stage[1].eventId++;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=input;bad.sameRouteProofs[0].stage[2].held1=0;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=input;bad.sameRouteProofs[0].stage[3].slot0Id++;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=input;bad.sameRouteProofs[0].stage[0].game=Route::Game::Pearl;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=input;bad.originalEggSnapshotIndependentlyAttested=false;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=input;bad.archives[0].originalPhysicalSlots[1].snapshotId++;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=input;bad.full.original.count=2;
        bad.full.original.alternatives[1]=input.full.original.alternatives[0];
        bad.full.original.alternatives[1].routeIdentity=2;
        bad.full.moveStages[1]=input.full.moveStages[0];
        attest(bad,1);
        bad.sameRouteProofs[0].stage[1].pairId++;
        const auto saved=Bound::evaluate(bad);
        assert(saved.status==Bound::Status::OneMatched);
        assert(saved.matched==1 && saved.unverified>=1);
        ++checks;
    }
    assert(checks==12288);
    std::cout<<"L2 Stage5 Pt HGSS same-route integration: "<<checks<<" PASS\n";
}
