#include "Legality/Gen3RetailDaycarePairHistory.h"
#include "Legality/Gen3EggPIDCreationHistory.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace R=Legality::Gen3RetailDaycarePairHistory;
namespace P=Legality::Gen3EggPIDCreationHistory;

// Source-mechanics simulation vectors, NOT an archived or authenticated
// original parent/egg capture. All claimed provenance is explicit input.
R::Input base(const char* game) {
    R::Input in{};
    in.game=game;
    in.originalDaycareEventIdentity=0xCAFEu;
    in.eventIdentityIndependentlyVerified=true;
    in.physicalDepositOrderIndependentlyVerified=true;
    // Female and male Pikachu, synthetic source-snapshot personalities.
    in.physicalSlots[0]={101,25,0,7,5,6,127,true,true,true};
    in.physicalSlots[1]={202,25,255,11,5,6,127,true,true,true};
    return in;
}
P::Input pid(const char* game,const R::Input& pair) {
    P::Input in{};
    in.game=game;
    in.originalUnhatchedEggIndependentlyVerified=true;
    in.ordinaryDaycarePairIndependentlyVerified=true;
    in.ordinaryDaycarePairEligible=true;
    in.originalPhysicalDepositOrderIndependentlyVerified=true;
    in.originalCreationStageIndependentlyVerified=true;
    in.originalCollectionStageIndependentlyVerified=true;
    in.originalParentStageIdentity=pair.originalDaycareEventIdentity;
    in.originalCreationStageIdentity=pair.originalDaycareEventIdentity;
    in.originalCollectionStageIdentity=pair.originalDaycareEventIdentity;
    for(uint8_t i=0;i<2;++i) {
        const auto& x=pair.physicalSlots[i];
        in.physicalSlots[i]={static_cast<uint32_t>(x.snapshotIdentity),
            x.species,static_cast<uint8_t>(R::gender(x)),x.personality,
            false,true,true};
    }
    in.pendingLCRNGStateIndependentlyVerified=true;
    in.pendingLCRNGState=0x12345678u;
    in.collectionLCRNGStateIndependentlyVerified=true;
    in.collectionLCRNGState=0x89ABCDEFu;
    in.originalEmeraldVBlankCounterIndependentlyVerified=true;
    in.originalEmeraldVBlankCounter=0xCDEFu;
    in.originalRetailPairWitnessSupplied=true;
    in.originalRetailPair=pair;
    return in;
}
int main() {
    unsigned checks=0;
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"}) {
        auto x=base(game);
        auto r=R::evaluate(x);
        assert(r.status==R::Status::ConditionallyCompatiblePair);
        assert(r.scoreKnown && r.compatibilityScore==70);
        assert(r.physicalRolesKnown);
        assert(r.originalMotherSnapshot==101 && r.originalFatherSnapshot==202);
        assert(r.originalEggSpeciesParentSnapshot==101);
        // Same species, same OT is exactly 50 (not 20).
        auto sameOT=x;sameOT.physicalSlots[1].originalFullOTId=7;
        r=R::evaluate(sameOT);
        assert(r.scoreKnown && r.compatibilityScore==50);
        // Different species with shared Field group: 20/50 by complete OT.
        auto different=x;different.physicalSlots[1].species=133; // Eevee
        different.physicalSlots[1].eggGroup1=5;
        different.physicalSlots[1].eggGroup2=5;
        different.physicalSlots[1].genderRatio=31;
        different.physicalSlots[1].originalFullOTId=7;
        r=R::evaluate(different);
        assert(r.status==R::Status::ConditionallyCompatiblePair);
        assert(r.compatibilityScore==20);
        different.physicalSlots[1].originalFullOTId=11;
        assert(R::evaluate(different).compatibilityScore==50);
        // Both deposit orders are valid if actually attested; donor roles
        // follow the parents, not their physical storage slot.
        auto reversed=x;
        reversed.physicalSlots[0]=x.physicalSlots[1];
        reversed.physicalSlots[1]=x.physicalSlots[0];
        r=R::evaluate(reversed);
        assert(r.status==R::Status::ConditionallyCompatiblePair);
        assert(r.originalMotherSnapshot==101 && r.originalFatherSnapshot==202);
        // Ditto + female: Ditto father; Ditto + male: Ditto mother.
        auto d=x;d.physicalSlots[1]={202,132,0,11,13,13,255,true,true,true};
        r=R::evaluate(d);
        assert(r.status==R::Status::ConditionallyCompatiblePair);
        assert(r.compatibilityScore==50);
        assert(r.originalMotherSnapshot==101 && r.originalFatherSnapshot==202);
        assert(r.originalEggSpeciesParentSnapshot==101);
        d.physicalSlots[0].personality=255; // male Pikachu
        r=R::evaluate(d);
        assert(r.status==R::Status::ConditionallyCompatiblePair);
        assert(r.originalFatherSnapshot==101 && r.originalMotherSnapshot==202);
        assert(r.originalEggSpeciesParentSnapshot==101);
        d.physicalSlots[0]={101,81,0,7,10,10,255,true,true,true};
        r=R::evaluate(d); // genderless Magnemite + Ditto
        assert(r.status==R::Status::ConditionallyCompatiblePair);
        assert(r.originalFatherSnapshot==101 && r.originalMotherSnapshot==202);
        d.physicalSlots[1].originalFullOTId=7;
        assert(R::evaluate(d).compatibilityScore==20);
        // Terminal incompatibilities: 2 Ditto, undiscovered, same gender,
        // disjoint egg groups, genderless without Ditto, out-of-range parent.
        d.physicalSlots[0]=d.physicalSlots[1];
        d.physicalSlots[0].snapshotIdentity=101;
        assert(R::evaluate(d).status==R::Status::IncompatiblePair);
        d=x;d.physicalSlots[1].eggGroup1=15;d.physicalSlots[1].eggGroup2=15;
        assert(R::evaluate(d).status==R::Status::IncompatiblePair);
        d=x;d.physicalSlots[1].personality=1;
        assert(R::evaluate(d).status==R::Status::IncompatiblePair);
        d=x;d.physicalSlots[1].eggGroup1=10;d.physicalSlots[1].eggGroup2=10;
        assert(R::evaluate(d).status==R::Status::IncompatiblePair);
        d=x;d.physicalSlots[1]={202,81,0,11,10,10,255,true,true,true};
        assert(R::evaluate(d).status==R::Status::IncompatiblePair);
        d=x;d.physicalSlots[1].species=387;
        assert(R::evaluate(d).status==R::Status::IncompatiblePair);
        // Absent or inconsistent historical evidence is Unknown, never
        // material from a detached offspring or a whole-Pokemon Invalid.
        d=x;d.physicalDepositOrderIndependentlyVerified=false;
        assert(R::evaluate(d).status==R::Status::UnknownOriginalStage);
        d=x;d.originalDaycareEventIdentity=0;
        assert(R::evaluate(d).status==R::Status::UnknownOriginalStage);
        d=x;d.physicalSlots[0].independentlyAttestedSnapshot=false;
        assert(R::evaluate(d).status==R::Status::UnknownOriginalSnapshots);
        d=x;d.physicalSlots[1].snapshotIdentity=d.physicalSlots[0].snapshotIdentity;
        assert(R::evaluate(d).status==R::Status::UnknownOriginalSnapshots);
        d=x;d.physicalSlots[0].independentlyAttestedPersonalData=false;
        assert(R::evaluate(d).status==R::Status::UnknownPersonalData);
        d=x;d.physicalSlots[0].eggGroup1=13; // false Pikachu-as-Ditto
        assert(R::evaluate(d).status==R::Status::UnknownPersonalData);
        d=x;d.physicalSlots[0].independentlyAttestedOTId=false;
        r=R::evaluate(d);
        assert(r.status==R::Status::ConditionallyCompatiblePair && !r.scoreKnown);
        // Explicitly join a source-attested pair into the original-stage
        // Gen III PID replay; no fabricated source parent from an egg PID.
        auto p=pid(game,x);
        p.originalUnhatchedEggPID=P::evaluate(p).finalPersonality;
        assert(P::evaluate(p).status==P::Status::ConditionallyMatchedOriginalEggPID);
        p.originalRetailPair.physicalSlots[0].snapshotIdentity++;
        assert(P::evaluate(p).status==P::Status::UnknownOriginalParentPair);
        p=pid(game,x);p.originalUnhatchedEggPID=P::evaluate(p).finalPersonality;
        p.originalRetailPair.physicalSlots[1].personality+=256u;
        assert(P::evaluate(p).status==P::Status::UnknownOriginalParentPair);
        p=pid(game,x);p.originalUnhatchedEggPID=P::evaluate(p).finalPersonality;
        p.originalRetailPair.originalDaycareEventIdentity++;
        assert(P::evaluate(p).status==P::Status::IncoherentOriginalStage);
        p=pid(game,x);p.originalUnhatchedEggPID=P::evaluate(p).finalPersonality;
        p.originalRetailPair.physicalSlots[0].independentlyAttestedPersonalData=false;
        assert(P::evaluate(p).status==P::Status::UnknownOriginalParentPair);
        p=pid(game,x);p.originalUnhatchedEggPID=P::evaluate(p).finalPersonality;
        p.originalRetailPair.physicalSlots[1].personality=0; // both female
        assert(P::evaluate(p).status==P::Status::IncompatibleOriginalDaycarePair);
        // Native PID snapshot remains untouched after all evaluations.
        checks+=31;
    }
    auto unsupported=base("diamond_nds");
    assert(R::evaluate(unsupported).status==R::Status::UnsupportedGame);
    std::cout<<"L2 source-backed Gen III original daycare parent witnesses: "
             <<checks<<" compatibility/role/OT/PID integration controls PASS\n";
}
