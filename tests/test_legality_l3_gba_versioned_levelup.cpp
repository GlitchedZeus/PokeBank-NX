#include "Legality/Gen3GbaVersionedLevelUpTimeline.h"
#include <cassert>
#include <iostream>
namespace L=Legality::Gen3GbaVersionedLevelUpTimeline;
using G=L::Game; using D=L::Decision;using S=L::Status;
constexpr L::Input bulbasaur(G g){
    L::Input a{};a.game=g;a.nationalSpecies=1;
    a.initialLevel=5;a.observedLevel=15;
    a.initialMoves={33,45,0,0};a.observedMoves={77,79,73,22};
    a.initialSnapshotVerified=true;
    a.sameSpeciesAndGameContinuityVerified=true;
    a.noOtherMoveChangesVerified=true;
    a.allPlayerOfferDecisionsVerified=true;
    a.stepCount=4;
    a.steps[0]={7,73,D::AutoLearn,2};
    a.steps[1]={10,22,D::AutoLearn,3};
    a.steps[2]={15,77,D::ReplaceSlot,0};
    a.steps[3]={15,79,D::ReplaceSlot,1};
    return a;
}
static_assert(L::evaluate(bulbasaur(G::Ruby)).status==S::CandidateConsistent);
static_assert(L::evaluate(bulbasaur(G::Sapphire)).status==S::CandidateConsistent);
static_assert(L::evaluate(bulbasaur(G::Emerald)).status==S::CandidateConsistent);
static_assert(L::evaluate(bulbasaur(G::FireRed)).status==S::CandidateConsistent);
static_assert(L::evaluate(bulbasaur(G::LeafGreen)).status==S::CandidateConsistent);
static_assert(L::evaluate(bulbasaur(G::Ruby)).replaced==2);
static_assert([]{auto a=bulbasaur(G::Ruby);a.stepCount=3;
    return L::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.steps[3].offeredMove=77;
    return L::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.steps[1].decision=D::Decline;
    return L::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.steps[0].slot=3;
    return L::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.steps[2].decision=D::AlreadyKnown;
    return L::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.allPlayerOfferDecisionsVerified=false;
    return L::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.sameSpeciesAndGameContinuityVerified=false;
    return L::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.noOtherMoveChangesVerified=false;
    return L::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.initialMoves={33,0,45,0};
    return L::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.stepCount=5;
    return L::evaluate(a).status==S::CandidateContradicted;}());

constexpr L::Input charmander(G g,uint16_t move) {
    L::Input a{};a.game=g;a.nationalSpecies=4;
    a.initialLevel=12;a.observedLevel=13;
    a.initialMoves={10,45,52,0};a.observedMoves={10,45,52,move};
    a.initialSnapshotVerified=true;a.sameSpeciesAndGameContinuityVerified=true;
    a.noOtherMoveChangesVerified=true;a.allPlayerOfferDecisionsVerified=true;
    a.stepCount=1;a.steps[0]={13,move,D::AutoLearn,3};return a;
}
static_assert(L::evaluate(charmander(G::Ruby,108)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(G::Sapphire,108)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(G::Emerald,108)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(G::FireRed,232)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(G::LeafGreen,232)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(G::FireRed,108)).status==S::CandidateContradicted);
static_assert(L::evaluate(charmander(G::Ruby,232)).status==S::CandidateContradicted);
static_assert([]{auto a=charmander(G::FireRed,232);a.initialSnapshotVerified=false;
    return L::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=charmander(G::Ruby,108);a.nationalSpecies=133;
    return L::evaluate(a).status==S::Unresolved;}());

constexpr L::Input bulbasaurDecline(){
    auto a=bulbasaur(G::Ruby);
    a.initialLevel=39;a.observedLevel=46;
    a.initialMoves={33,45,73,22};a.observedMoves=a.initialMoves;
    a.stepCount=1;a.steps[0]={46,76,D::Decline,0};return a;
}
static_assert(L::evaluate(bulbasaurDecline()).status==S::CandidateConsistent);
static_assert([]{auto a=bulbasaurDecline();a.steps[0].decision=D::AutoLearn;
    return L::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaurDecline();a.steps[0].decision=D::AlreadyKnown;
    return L::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaurDecline();a.observedMoves={33,45,73,76};
    return L::evaluate(a).status==S::CandidateContradicted;}());

constexpr L::Input emerRepeat(){
    L::Input a{};a.game=G::Emerald;a.nationalSpecies=2;
    a.initialLevel=1;a.observedLevel=4;
    a.initialMoves={33,45,73,0};a.observedMoves=a.initialMoves;
    a.initialSnapshotVerified=true;a.sameSpeciesAndGameContinuityVerified=true;
    a.noOtherMoveChangesVerified=true;a.allPlayerOfferDecisionsVerified=true;
    a.stepCount=1;a.steps[0]={4,45,D::AlreadyKnown,0};return a;
}
static_assert(L::evaluate(emerRepeat()).status==S::CandidateConsistent);
static_assert([]{auto a=emerRepeat();a.steps[0].decision=D::AutoLearn;
    return L::evaluate(a).status==S::CandidateContradicted;}());

// The accepted Emerald table has full 386-species National Dex coverage.
void exerciseAllEmerald() {
    uint32_t entries=0;uint8_t peak=0;
    for(uint16_t species=1;species<=386;++species) {
        assert(L::supported(G::Emerald,species));
        const auto count=L::offerCount(G::Emerald,species);
        assert(count>=1 && count<=32);
        if(count>peak)peak=static_cast<uint8_t>(count);
        uint8_t priorLevel=0;
        for(uint16_t index=0;index<count;++index) {
            const auto offer=L::offerAt(G::Emerald,species,index);
            assert(offer.level>=1 && offer.level<=100);
            assert(offer.move>=1 && offer.move<=354);
            assert(offer.level>=priorLevel);
            priorLevel=offer.level;++entries;
        }
    }
    assert(entries==3947 && peak==17);
    assert(!L::supported(G::Ruby,386));
    assert(!L::supported(G::LeafGreen,386));
}
int main(){exerciseAllEmerald();std::cout<<"L3 pinned GBA ordered level-up four-slot replay: PASS\n";}
