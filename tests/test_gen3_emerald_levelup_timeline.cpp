#include "Legality/Gen3EmeraldLevelUpTimeline.h"
#include <array>
#include <cassert>
#include <iostream>
namespace E=Legality::Gen3EmeraldLevelUpTimeline;
using D=E::Decision;
void runCases() {
    // Source-pinned Emerald Bulbasaur level-5->32: at levels 7,10,
    // 15 twice,20,25,32. Exact order and simultaneous four slots.
    E::Input in{};
    in.game="emerald_gba";in.species=1;
    in.encounterLevel=5;in.observedLevel=32;
    in.initialSnapshotVerified=true;
    in.sameSpeciesVerified=true;
    in.allOffersAndDecisionsVerified=true;
    in.initialMoves={33,45,0,0};in.stepCount=7;
    in.steps[0]={7,73,D::AutoLearn,2};
    in.steps[1]={10,22,D::AutoLearn,3};
    in.steps[2]={15,77,D::ReplaceSlot,0};
    in.steps[3]={15,79,D::ReplaceSlot,1};
    in.steps[4]={20,75,D::ReplaceSlot,2};
    in.steps[5]={25,230,D::Decline,0};
    in.steps[6]={32,74,D::Decline,0};
    in.observedMoves={77,79,75,22};
    const auto success=E::evaluate(in);
    assert(success.status==E::Status::CandidateConsistent);
    assert(success.offers==7 && success.replacements==3 && success.declines==2);
    assert((success.reconstructed==std::array<uint16_t,4>{77,79,75,22}));
    auto wrongOrder=in;wrongOrder.steps[2].offeredMove=79;
    assert(E::evaluate(wrongOrder).status==E::Status::CandidateContradicted);
    auto wrongAuto=in;wrongAuto.steps[0].replacementSlot=0;
    assert(E::evaluate(wrongAuto).status==E::Status::CandidateContradicted);
    auto skipWithSpace=in;skipWithSpace.steps[0].decision=D::Decline;
    assert(E::evaluate(skipWithSpace).status==E::Status::CandidateContradicted);
    auto wrongFinal=in;wrongFinal.observedMoves={77,75,79,22};
    assert(E::evaluate(wrongFinal).status==E::Status::CandidateContradicted);
    auto missingSnapshot=in;missingSnapshot.initialSnapshotVerified=false;
    assert(E::evaluate(missingSnapshot).status==E::Status::UnknownSnapshot);
    auto unknownEvo=in;unknownEvo.sameSpeciesVerified=false;
    assert(E::evaluate(unknownEvo).status==E::Status::UnknownSpeciesContinuity);
    auto partial=in;partial.allOffersAndDecisionsVerified=false;
    assert(E::evaluate(partial).status==E::Status::IncompleteTrainingHistory);
    auto missingOne=in;missingOne.stepCount=6;
    assert(E::evaluate(missingOne).status==E::Status::IncompleteTrainingHistory);
    auto extra=in;extra.stepCount=8;extra.steps[7]={40,200,D::Decline,0};
    assert(E::evaluate(extra).status==E::Status::CandidateContradicted);
    auto invalidGame=in;invalidGame.game="firered_gba";
    assert(E::evaluate(invalidGame).status==E::Status::Unsupported);
    auto invalidSpecies=in;invalidSpecies.species=387;
    assert(E::evaluate(invalidSpecies).status==E::Status::Unsupported);
    auto invalidSlots=in;invalidSlots.initialMoves={33,0,45,0};
    assert(E::evaluate(invalidSlots).status==E::Status::CandidateContradicted);
    auto identical=in;identical.encounterLevel=32;
    identical.initialMoves=in.observedMoves;identical.stepCount=0;
    assert(E::evaluate(identical).status==E::Status::CandidateConsistent);
    auto tooMany=in;tooMany.stepCount=25;
    assert(E::evaluate(tooMany).status==E::Status::IncompleteTrainingHistory);
    // Walk the entire independently pinned 386-species / 3,947-move
    // Emerald resource under the real host and sanitizer compilers.
    // Validate ordered levels, full 0..354 move domain and stage
    // capacity for ALL species rather than only one Bulbasaur fixture.
    namespace Table=Legality::Gen3EmeraldLevelUp;
    assert(Table::kEmeraldLevelOffsets[387]==3947);
    uint32_t verifiedEntries=0;
    uint16_t maximumOffers=0;
    for(uint16_t species=1;species<=386;++species) {
        const auto begin=Table::kEmeraldLevelOffsets[species];
        const auto end=Table::kEmeraldLevelOffsets[species+1];
        assert(end>begin && end-begin<=24);
        if(end-begin>maximumOffers)
            maximumOffers=static_cast<uint16_t>(end-begin);
        uint8_t lastLevel=0;
        for(uint16_t i=begin;i<end;++i) {
            const uint16_t packed=Table::kEmeraldPackedLevelMoves[i];
            const uint8_t level=static_cast<uint8_t>(packed>>9);
            const uint16_t move=packed&511u;
            assert(level>=1 && level<=100 && level>=lastLevel);
            assert(move>=1 && move<=354);
            lastLevel=level;
            ++verifiedEntries;
        }
    }
    assert(verifiedEntries==3947 && maximumOffers==17);
    assert(Legality::Gen3EmeraldLevelUp::firstLevelBetween(1,73,5,10)==7);
    assert(Legality::Gen3EmeraldLevelUp::firstLevelBetween(1,79,14,15)==15);
}
int main(){runCases();std::cout<<"Emerald 4-slot level-up trainer decisions: PASS\n";}
