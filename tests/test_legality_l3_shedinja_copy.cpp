#include "Legality/Gen14CandidateMoveShedinjaCopyStage.h"
#include <array>
#include <cassert>
#include <iostream>
namespace S=Legality::Gen14CandidateMoveShedinjaCopyStage;
namespace L=Legality::Gen3GbaVersionedLevelUpTimeline;
using G=S::Game;using D=L::Decision;using Status=S::Status;
constexpr std::array<G,5> versions{{
    G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen
}};
constexpr L::Input original(G game,uint8_t level){
    L::Input a{};a.game=game;a.nationalSpecies=290;
    a.initialLevel=static_cast<uint8_t>(level==20?13:level-1);
    a.observedLevel=level;
    a.initialMoves={10,106,141,28};a.observedMoves=a.initialMoves;
    a.initialSnapshotVerified=true;
    a.sameSpeciesAndGameContinuityVerified=true;
    a.noOtherMoveChangesVerified=true;
    a.allPlayerOfferDecisionsVerified=true;
    if(level==20) {
        a.stepCount=2;
        a.steps[0]={14,154,D::Decline,0};
        a.steps[1]={19,170,D::Decline,0};
    } else if(level==38) {
        a.stepCount=1;
        a.steps[0]={38,232,D::Decline,0};
    } else if(level==45) {
        a.stepCount=1;
        a.steps[0]={45,91,D::Decline,0};
    }
    return a;
}
constexpr S::Input atEvolution(G game,uint8_t level) {
    S::Input a{};a.originalNincadaCandidateId=150;
    a.game=game;a.evolutionLevel=level;a.originalNincada=original(game,level);
    a.originalNincadaToNinjaskEvolutionVerified=true;
    a.freePartySlotAtCreationVerified=true;
    a.allPostEvolutionNinjaskOffersVerified=true;
    a.createdAfterNinjaskMoveDecisionsVerified=true;
    a.originalShedinjaSiblingCopyVerified=true;
    a.laterShedinja.game=game;a.laterShedinja.nationalSpecies=292;
    a.laterShedinja.initialLevel=level;a.laterShedinja.observedLevel=level;
    a.laterShedinja.initialSnapshotVerified=true;
    a.laterShedinja.sameSpeciesAndGameContinuityVerified=true;
    a.laterShedinja.noOtherMoveChangesVerified=true;
    a.laterShedinja.allPlayerOfferDecisionsVerified=true;
    if(level==20) {
        a.evolutionDecisionCount=3;
        a.evolutionDecisions[0]={20,104,D::ReplaceSlot,0};
        a.evolutionDecisions[1]={20,210,D::Decline,0};
        a.evolutionDecisions[2]={20,103,D::ReplaceSlot,1};
        a.verifiedNinjaskAfterEvolution={104,103,141,28};
    } else if(level==38) {
        a.evolutionDecisionCount=1;
        a.evolutionDecisions[0]={38,97,D::ReplaceSlot,0};
        a.verifiedNinjaskAfterEvolution={97,106,141,28};
    } else if(level==45) {
        a.evolutionDecisionCount=1;
        a.evolutionDecisions[0]={45,226,D::ReplaceSlot,0};
        a.verifiedNinjaskAfterEvolution={226,106,141,28};
    }
    a.verifiedShedinjaAtCreation=a.verifiedNinjaskAfterEvolution;
    a.laterShedinja.initialMoves=a.verifiedShedinjaAtCreation;
    a.laterShedinja.observedMoves=a.verifiedShedinjaAtCreation;
    return a;
}
static_assert(S::evaluate(atEvolution(G::Ruby,20)).status==Status::CandidateConsistent);
static_assert(S::evaluate(atEvolution(G::Emerald,38)).status==Status::CandidateConsistent);
static_assert(S::evaluate(atEvolution(G::FireRed,45)).status==Status::CandidateConsistent);
static_assert(S::evaluate(atEvolution(G::LeafGreen,20)).evolutionOffers==3);
static_assert([]{auto a=atEvolution(G::Emerald,20);a.freePartySlotAtCreationVerified=false;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.originalShedinjaSiblingCopyVerified=false;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.createdAfterNinjaskMoveDecisionsVerified=false;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.originalNincadaToNinjaskEvolutionVerified=false;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.allPostEvolutionNinjaskOffersVerified=false;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.evolutionDecisionCount=2;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.evolutionDecisions[1].offeredMove=103;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.evolutionDecisions[0].decision=D::AutoLearn;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.evolutionDecisions[0].slot=2;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.verifiedShedinjaAtCreation={10,106,141,28};
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.verifiedNinjaskAfterEvolution={104,210,141,28};
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.originalNincada.steps[0].offeredMove=170;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.originalNincada.allPlayerOfferDecisionsVerified=false;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,38);a.evolutionDecisions[0].offeredMove=226;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,45);a.evolutionDecisions[0].offeredMove=97;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,45);a.verifiedShedinjaAtCreation={97,226,141,28};
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.laterShedinja.game=G::Platinum;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.laterShedinja.nationalSpecies=291;
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.laterShedinja.initialMoves={104,210,141,28};
    return S::evaluate(a).status==Status::Unresolved;}());
static_assert([]{auto a=atEvolution(G::Emerald,20);a.originalNincadaCandidateId=0;
    return S::evaluate(a).status==Status::Unsupported;}());
constexpr S::Input laterShedinja(G game){
    auto a=atEvolution(game,20);
    a.laterShedinja.observedLevel=31;
    a.laterShedinja.stepCount=2;
    a.laterShedinja.steps[0]={25,180,D::ReplaceSlot,2};
    a.laterShedinja.steps[1]={31,109,D::Decline,0};
    a.laterShedinja.observedMoves={104,103,180,28};
    return a;
}
static_assert(S::evaluate(laterShedinja(G::Sapphire)).status==Status::CandidateConsistent);
static_assert(S::evaluate(laterShedinja(G::FireRed)).observedShedinja[2]==180);
static_assert([]{auto a=laterShedinja(G::Emerald);a.laterShedinja.steps[0].offeredMove=206;
    return S::evaluate(a).status==Status::CandidateContradicted;}());
static_assert([]{auto a=laterShedinja(G::Emerald);a.laterShedinja.stepCount=1;
    return S::evaluate(a).status==Status::Unresolved;}());
int main(){
    uint32_t tested=0;
    for(auto version:versions)for(const uint8_t level:{20,38,45}) {
        auto a=atEvolution(version,level);
        auto r=S::evaluate(a);
        assert(r.status==Status::CandidateConsistent);
        assert(r.shedinjaAtCreation==a.verifiedNinjaskAfterEvolution);
        a.originalShedinjaSiblingCopyVerified=false;
        assert(S::evaluate(a).status==Status::Unresolved);
        ++tested;
    }
    assert(tested==15);
    for(auto version:versions)assert(S::evaluate(laterShedinja(version)).status==Status::CandidateConsistent);
    std::cout<<"L3 source-ordered 15 Nincada evolutions, 15 sibling-copy Unknown, 5 later Shedinja stages: PASS\n";
}
