// Reuse the real existing L2 production fixture, not a mocked stage.
#define main l2_previous_full_stage_regression_main
#include "test_legality_l2_gen4_pt_hgss_full_moves.cpp"
#undef main
#include "Legality/Gen4PlatinumHGSSSourceBoundEggHistories.h"
namespace Bound=Legality::Gen4PlatinumHGSSSourceBoundEggHistories;
namespace Archive=Legality::Gen4PlatinumHGSSOriginalDaycareSource;
Bound::Input attested(const char* game,uint32_t seed,uint16_t a,uint16_t b){
    Bound::Input in{};
    in.full=source(game,seed,a,b);
    in.allArchivesIndependentlyEnumerated=true;
    auto& archive=in.archives[0];
    auto& r=in.full.original.alternatives[0];
    auto& p=r.pid;
    archive.supplied=true;
    archive.originalEventIndependentlyAttested=true;
    archive.originalPairIdentityIndependentlyAttested=true;
    archive.physicalDepositOrderIndependentlyAttested=true;
    archive.bothParentSnapshotsIndependentlyAttested=true;
    archive.creationHeldItemsIndependentlyAttested=true;
    archive.collectionLanguagesIndependentlyAttested=true;
    archive.parentPersonalitiesIndependentlyAttested=true;
    archive.game=game;
    archive.originalEggEvent=in.full.original.originalEggEventIdentity;
    archive.physicalPairIdentity=r.originalDaycarePairIdentity;
    for(unsigned i=0;i<2;++i) {
        archive.originalPhysicalSlots[i]={
            p.parents[i].snapshotId,p.parents[i].species,
            p.parents[i].heldItem,p.parents[i].gender,
            p.originalParentLanguages[i],p.parents[i].originalPID};
    }
    return in;
}
int main(){
    std::uint64_t checks=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"})
    for(uint32_t seed=0;seed<64;++seed)
    for(uint16_t a:items)
    for(uint16_t b:items){
        auto in=attested(game,seed,a,b);
        auto yes=Bound::evaluate(in);
        assert(yes.status==Bound::Status::OneMatched && yes.matched==1);
        assert(yes.sourceChecks[0]==Archive::Status::ConditionallyAttested);
        ++checks;
        auto bad=in;bad.archives[0].originalEggEvent++;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.archives[0].physicalPairIdentity++;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.archives[0].originalPhysicalSlots[0].snapshotId++;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.archives[0].originalPhysicalSlots[1].heldAtCreation^=1;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.archives[0].originalPhysicalSlots[1].languageAtCollection=1;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.archives[0].parentPersonalitiesIndependentlyAttested=false;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.full.original.alternatives[0].pid.parents[0].species=132;
        bad.full.original.alternatives[0].pid.parents[0].gender=0;
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownSources);++checks;
        bad=in;bad.full.moveStages[0].originalFatherMachineMoves={33,0,0,0};
        const auto contradicted=Bound::evaluate(bad);
        assert(contradicted.status==Bound::Status::AllLocalContradicted);
        assert(contradicted.contradicted==1 && !contradicted.matched);
        ++checks;
        bad=in;
        bad.allArchivesIndependentlyEnumerated=false;
        bad.full.moveStages[0].originalFatherMachineMoves={33,0,0,0};
        assert(Bound::evaluate(bad).status==Bound::Status::UnknownEnumeration);
        ++checks;
        bad=in;
        bad.full.original.count=2;
        bad.full.original.alternatives[1]=in.full.original.alternatives[0];
        bad.full.original.alternatives[1].routeIdentity=2;
        bad.full.moveStages[1]=in.full.moveStages[0];
        bad.archives[1]=in.archives[0];
        bad.archives[0].originalPhysicalSlots[0].snapshotId++;
        const auto surviving=Bound::evaluate(bad);
        assert(surviving.status==Bound::Status::OneMatched);
        assert(surviving.matched==1 && surviving.unverified==1);
        ++checks;
    }
    assert(checks==135168);
    std::cout<<"L2 Pt HGSS source-bound PID IV item move joint: "<<checks<<" PASS\n";
}
