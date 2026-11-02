#include "Legality/Gen4DiamondPearlOriginalParentWitness.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace W=Legality::Gen4DiamondPearlOriginalParentWitness;
int main() {
    std::uint64_t checks=0;
    for(unsigned game=0;game<2;++game)
    for(std::uint64_t seed=1;seed<=4096;++seed) {
        W::Input source{};
        source.supplied=true;
        source.originalDaycareEventIndependentlyVerified=true;
        source.physicalDepositOrderIndependentlyVerified=true;
        source.originalPairIdentityIndependentlyVerified=true;
        source.originalParentSnapshotsIndependentlyVerified=true;
        source.creationItemsIndependentlyVerified=true;
        source.collectionLanguagesIndependentlyVerified=true;
        source.originalEggEventIdentity=seed;
        source.originalPhysicalPairIdentity=seed+0x10000;
        source.physicalSlots[0]={101,25,1,7,229,2};
        source.physicalSlots[1]={202,25,0,11,0,static_cast<std::uint8_t>(2+game)};
        W::Expected expected{};
        expected.originalEggEventIdentity=seed;
        expected.otherStagePhysicalPairIdentity=seed+0x10000;
        expected.species={25,25};
        expected.gender={1,0};
        expected.personalities={7,11};
        expected.personalitiesIndependentlyVerified={true,true};
        expected.creationItems={229,0};
        expected.collectionLanguages={2,static_cast<std::uint8_t>(2+game)};
        expected.collectionLanguagesIndependentlyVerified=true;
        assert(W::evaluate(source,expected)==W::Status::ConditionallyMatched);
        ++checks;
        auto other=source;other.originalEggEventIdentity++;
        assert(W::evaluate(other,expected)==W::Status::IncoherentPairOrEvent);
        ++checks;
        other=source;other.originalPhysicalPairIdentity++;
        assert(W::evaluate(other,expected)==W::Status::IncoherentPairOrEvent);
        ++checks;
        other=source;other.physicalSlots[0].snapshotIdentity=0;
        assert(W::evaluate(other,expected)==W::Status::UnknownSourceEvidence);
        ++checks;
        other=source;other.physicalSlots[1].snapshotIdentity=101;
        assert(W::evaluate(other,expected)==W::Status::UnknownSourceEvidence);
        ++checks;
        other=source;other.physicalSlots[0].species=81;
        assert(W::evaluate(other,expected)==W::Status::IncoherentOriginalSnapshots);
        ++checks;
        other=source;other.physicalSlots[1].gender=2;
        assert(W::evaluate(other,expected)==W::Status::IncoherentOriginalSnapshots);
        ++checks;
        other=source;other.physicalSlots[0].personality^=1u;
        assert(W::evaluate(other,expected)==W::Status::IncoherentOriginalSnapshots);
        ++checks;
        other=source;other.physicalSlots[1].heldItemAtCreation=236;
        assert(W::evaluate(other,expected)==W::Status::IncoherentOriginalSnapshots);
        ++checks;
        other=source;other.physicalSlots[1].languageAtCollection=1;
        assert(W::evaluate(other,expected)==W::Status::IncoherentOriginalSnapshots);
        ++checks;
        other=source;other.originalParentSnapshotsIndependentlyVerified=false;
        assert(W::evaluate(other,expected)==W::Status::UnknownSourceEvidence);
        ++checks;
        other=source;other.collectionLanguagesIndependentlyVerified=false;
        assert(W::evaluate(other,expected)==W::Status::UnknownSourceEvidence);
        ++checks;
        auto unknown=expected;unknown.collectionLanguagesIndependentlyVerified=false;
        assert(W::evaluate(source,unknown)==W::Status::UnknownSourceEvidence);
        ++checks;
        unknown=expected;unknown.personalitiesIndependentlyVerified[0]=false;
        assert(W::evaluate(source,unknown)==W::Status::UnknownSourceEvidence);
        ++checks;
        other=source;other.supplied=false;
        assert(W::evaluate(other,expected)==W::Status::NotSupplied);
        ++checks;
        unknown=expected;unknown.otherStagePhysicalPairIdentity=0;
        assert(W::evaluate(source,unknown)==W::Status::ConditionallyMatched);
        ++checks;
    }
    assert(checks==131072);
    std::cout<<"L2 DP original parent source identity witness: "<<checks<<" PASS\n";
}
