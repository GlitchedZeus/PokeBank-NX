#include "Legality/Gen4PlatinumHGSSOriginalDaycareSource.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace W=Legality::Gen4PlatinumHGSSOriginalDaycareSource;
int main(){
    std::uint64_t checks=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"})
    for(std::uint64_t seed=1;seed<=4096;++seed){
        W::Input x{};
        x.supplied=true;
        x.originalEventIndependentlyAttested=true;
        x.originalPairIdentityIndependentlyAttested=true;
        x.physicalDepositOrderIndependentlyAttested=true;
        x.bothParentSnapshotsIndependentlyAttested=true;
        x.creationHeldItemsIndependentlyAttested=true;
        x.collectionLanguagesIndependentlyAttested=true;
        x.parentPersonalitiesIndependentlyAttested=true;
        x.game=game;
        x.originalEggEvent=seed;
        x.physicalPairIdentity=seed+0x10000u;
        x.originalPhysicalSlots={{{101,25,229,1,2,7},{202,25,294,0,3,11}}};
        W::Expected y{game,seed,seed+0x10000u,x.originalPhysicalSlots,true,true};
        assert(W::evaluate(x,y)==W::Status::ConditionallyAttested);++checks;
        auto invalid=x;invalid.originalEggEvent++;
        assert(W::evaluate(invalid,y)==W::Status::CrossedOriginalRoute);++checks;
        invalid=x;invalid.physicalPairIdentity++;
        assert(W::evaluate(invalid,y)==W::Status::CrossedOriginalRoute);++checks;
        invalid=x;invalid.physicalDepositOrderIndependentlyAttested=false;
        assert(W::evaluate(invalid,y)==W::Status::UnknownOriginalSource);++checks;
        invalid=x;invalid.originalPhysicalSlots[0].snapshotId=0;
        assert(W::evaluate(invalid,y)==W::Status::UnknownOriginalSource);++checks;
        invalid=x;invalid.originalPhysicalSlots[1].snapshotId=101;
        assert(W::evaluate(invalid,y)==W::Status::UnknownOriginalSource);++checks;
        invalid=x;invalid.originalPhysicalSlots[0].species=132;
        assert(W::evaluate(invalid,y)==W::Status::ContradictoryOriginalSnapshots);++checks;
        invalid=x;invalid.originalPhysicalSlots[1].originalGender=2;
        assert(W::evaluate(invalid,y)==W::Status::ContradictoryOriginalSnapshots);++checks;
        invalid=x;invalid.originalPhysicalSlots[0].heldAtCreation=0;
        assert(W::evaluate(invalid,y)==W::Status::ContradictoryOriginalSnapshots);++checks;
        invalid=x;invalid.originalPhysicalSlots[1].languageAtCollection=2;
        assert(W::evaluate(invalid,y)==W::Status::ContradictoryOriginalSnapshots);++checks;
        invalid=x;invalid.originalPhysicalSlots[0].originalPersonality^=1;
        assert(W::evaluate(invalid,y)==W::Status::ContradictoryOriginalSnapshots);++checks;
        invalid=x;invalid.collectionLanguagesIndependentlyAttested=false;
        assert(W::evaluate(invalid,y)==W::Status::UnknownOriginalSource);++checks;
        invalid=x;invalid.creationHeldItemsIndependentlyAttested=false;
        assert(W::evaluate(invalid,y)==W::Status::UnknownOriginalSource);++checks;
        auto unknown=y;unknown.collectionLanguagesKnown=false;
        assert(W::evaluate(x,unknown)==W::Status::UnknownOriginalSource);++checks;
        unknown=y;unknown.originalPersonalitiesKnown=false;
        assert(W::evaluate(x,unknown)==W::Status::UnknownOriginalSource);++checks;
        invalid=x;invalid.supplied=false;
        assert(W::evaluate(invalid,y)==W::Status::NotSupplied);++checks;
        unknown=y;unknown.independentlyNamedPhysicalPair=0;
        assert(W::evaluate(x,unknown)==W::Status::ConditionallyAttested);++checks;
        unknown=y;unknown.game="pearl_nds";
        assert(W::evaluate(x,unknown)==W::Status::CrossedOriginalRoute);++checks;
    }
    assert(checks==221184);
    std::cout<<"L2 original Pt HGSS acquisition source: "<<checks<<" PASS\n";
}
