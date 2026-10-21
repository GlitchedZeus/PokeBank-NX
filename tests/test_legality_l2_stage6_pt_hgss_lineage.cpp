// Reuse original Platinum/HGSS parent, nature, Power-item and move fixture.
#define LEGALITY_L2_IMPORT_STAGE5_FIXTURE 1
#include "test_legality_l2_stage5_pt_hgss_joint_routes.cpp"
#undef LEGALITY_L2_IMPORT_STAGE5_FIXTURE
namespace Lineage6=Legality::Gen34OriginalParentLineage;
namespace Route6=Legality::Gen34OriginalSameRouteProof;
void attachLineage(Bound::Input& in,unsigned index){
    auto& witness=in.originalParentLineages[index];
    witness.supplied=true;
    witness.originalEventOrderingIndependentlyAttested=true;
    witness.originalEggEventSequence=10000;
    const auto& c=in.full.original.alternatives[index];
    for(unsigned j=0;j<2;++j) {
        auto& p=witness.physicalSlots[j];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.deposited={c.pid.parents[j].snapshotId,
            Route6::gameFor(in.full.original.game),c.pid.parents[j].species};
        p.originalOrigin=p.deposited;
    }
    auto& imported=witness.physicalSlots[0];
    imported.originalOrigin={900,Route6::Game::Emerald,25};
    imported.transitions=1;
    imported.history[0]={true,true,false,Lineage6::TransitionKind::PalPark,
        5,imported.originalOrigin,imported.deposited};
    auto& evolved=witness.physicalSlots[1];
    evolved.originalOrigin={901,Route6::gameFor(in.full.original.game),172};
    evolved.transitions=1;
    evolved.history[0]={true,true,true,Lineage6::TransitionKind::Evolution,
        6,evolved.originalOrigin,evolved.deposited};
}
int main(){
    unsigned checks=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"})
    for(unsigned seed=0;seed<256;++seed){
        Bound::Input in{};
        in.full=source(game,seed,229,294);
        in.allArchivesIndependentlyEnumerated=true;
        attest(in,0);
        attachLineage(in,0);
        const auto yes=Bound::evaluate(in);
        assert(yes.status==Bound::Status::OneMatched);
        assert(yes.lineageChecks[0]==Lineage6::Status::ConditionallyReplayed);++checks;
        auto wrong=in;wrong.originalParentLineages[0].physicalSlots[1].history[0].evolutionMethodIndependentlyVerified=false;
        assert(Bound::evaluate(wrong).status==Bound::Status::UnknownSources);++checks;
        wrong=in;wrong.originalParentLineages[0].physicalSlots[0].history[0].after.species++;
        assert(Bound::evaluate(wrong).status==Bound::Status::UnknownSources);++checks;
        wrong=in;wrong.originalParentLineages[0].physicalSlots[1].history[0].before.identity++;
        assert(Bound::evaluate(wrong).status==Bound::Status::UnknownSources);++checks;
        wrong=in;wrong.originalParentLineages[0].physicalSlots[0].history[0].originalSequence=10000;
        assert(Bound::evaluate(wrong).status==Bound::Status::UnknownSources);++checks;
        wrong=in;wrong.originalParentLineages[0].supplied=false;
        assert(Bound::evaluate(wrong).status==Bound::Status::OneMatched);++checks;
        wrong=in;wrong.full.original.count=2;
        wrong.full.original.alternatives[1]=in.full.original.alternatives[0];
        wrong.full.original.alternatives[1].routeIdentity=2;
        wrong.full.moveStages[1]=in.full.moveStages[0];
        wrong.archives[1]=in.archives[0];
        attest(wrong,1);
        attachLineage(wrong,1);
        wrong.originalParentLineages[0].physicalSlots[0].deposited.identity++;
        const auto rescued=Bound::evaluate(wrong);
        assert(rescued.status==Bound::Status::OneMatched);
        assert(rescued.matched==1 && rescued.unverified>=1);++checks;
    }
    assert(checks==5376);
    std::cout<<"L2 Stage6 Pt HGSS migrated evolved parents: "<<checks<<" PASS\n";
}
