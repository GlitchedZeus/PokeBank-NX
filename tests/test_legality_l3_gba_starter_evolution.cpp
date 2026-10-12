#include "Legality/Gen14CandidateMoveGbaEvolutionStages.h"
#include <array>
#include <cassert>
#include <iostream>
namespace E=Legality::Gen14CandidateMoveGbaEvolutionStages;
namespace L=Legality::Gen3GbaVersionedLevelUpTimeline;
using G=E::Game;using D=L::Decision;using S=E::Status;
constexpr L::Input stage(G game,uint16_t species,uint8_t from,uint8_t to,
                         std::array<uint16_t,4> initial,
                         std::array<uint16_t,4> final) {
    L::Input x{};
    x.game=game;x.nationalSpecies=species;
    x.initialLevel=from;x.observedLevel=to;
    x.initialMoves=initial;x.observedMoves=final;
    x.initialSnapshotVerified=true;
    x.sameSpeciesAndGameContinuityVerified=true;
    x.noOtherMoveChangesVerified=true;
    x.allPlayerOfferDecisionsVerified=true;
    return x;
}
constexpr E::Input bulbasaur(G game){
    E::Input x{};x.candidateId=9001;x.phaseCount=3;
    x.sameOriginalPokemonAcrossStagesVerified=true;
    x.exactEvolutionSequenceVerified=true;
    x.phases[0]=stage(game,1,10,16,{33,45,73,22},{33,45,73,22});
    x.phases[0].stepCount=2;
    x.phases[0].steps[0]={15,77,D::Decline,0};
    x.phases[0].steps[1]={15,79,D::Decline,0};
    x.evolutions[0]={1,2,16,true,true};
    x.phases[1]=stage(game,2,16,32,{33,45,73,22},{33,45,73,22});
    x.phases[1].stepCount=2;
    x.phases[1].steps[0]={22,75,D::Decline,0};
    x.phases[1].steps[1]={29,230,D::Decline,0};
    x.evolutions[1]={2,3,32,true,true};
    x.phases[2]=stage(game,3,32,41,{33,45,73,22},{74,45,73,22});
    x.phases[2].stepCount=1;
    x.phases[2].steps[0]={41,74,D::ReplaceSlot,0};
    return x;
}
constexpr E::Input squirtle(G game){
    E::Input x{};x.candidateId=9007;x.phaseCount=3;
    x.sameOriginalPokemonAcrossStagesVerified=true;
    x.exactEvolutionSequenceVerified=true;
    x.phases[0]=stage(game,7,14,16,{33,39,145,55},{33,39,145,55});
    x.evolutions[0]={7,8,16,true,true};
    x.phases[1]=stage(game,8,16,36,{33,39,145,55},{33,39,145,55});
    x.phases[1].stepCount=3;
    x.phases[1].steps[0]={19,44,D::Decline,0};
    x.phases[1].steps[1]={25,229,D::Decline,0};
    x.phases[1].steps[2]={31,182,D::Decline,0};
    x.evolutions[1]={8,9,36,true,true};
    x.phases[2]=stage(game,9,36,42,{33,39,145,55},{33,39,145,55});
    x.phases[2].stepCount=1;
    x.phases[2].steps[0]={42,240,D::Decline,0};
    return x;
}
constexpr E::Input charmander(G game,bool delayEvolution){
    E::Input x{};x.candidateId=9004;x.phaseCount=3;
    x.sameOriginalPokemonAcrossStagesVerified=true;
    x.exactEvolutionSequenceVerified=true;
    const bool frlg=game==G::FireRed || game==G::LeafGreen;
    const uint16_t offered=frlg?232u:108u;
    x.phases[0]=stage(game,4,12,16,{10,45,52,0},{10,45,52,offered});
    x.phases[0].stepCount=1;
    x.phases[0].steps[0]={13,offered,D::AutoLearn,3};
    x.evolutions[0]={4,5,16,true,true};
    const uint8_t evolutionLevel=delayEvolution?37:36;
    x.phases[1]=stage(game,5,16,evolutionLevel,
                      {10,45,52,offered},{10,45,52,offered});
    x.phases[1].stepCount=3;
    x.phases[1].steps[0]={20,static_cast<uint16_t>(frlg?108u:99u),D::Decline,0};
    x.phases[1].steps[1]={27,184,D::Decline,0};
    x.phases[1].steps[2]={34,53,D::Decline,0};
    x.evolutions[1]={5,6,evolutionLevel,true,true};
    x.phases[2]=stage(game,6,evolutionLevel,44,
                      {10,45,52,offered},{10,45,52,offered});
    x.phases[2].stepCount=1;
    x.phases[2].steps[0]={44,163,D::Decline,0};
    return x;
}
static_assert(E::evaluate(bulbasaur(G::Ruby)).status==S::CandidateConsistent);
static_assert(E::evaluate(bulbasaur(G::Emerald)).status==S::CandidateConsistent);
static_assert(E::evaluate(bulbasaur(G::LeafGreen)).verifiedEvolutions==2);
static_assert(E::evaluate(squirtle(G::FireRed)).status==S::CandidateConsistent);
static_assert(E::evaluate(squirtle(G::Sapphire)).finalSpecies==9);

// Original pinned pret GBA evolution scene: species changes before the
// evolved form's level-up move teaching. An independently verified level-36
// Charizard decision therefore can be replayed without inventing a
// Charmeleon history before it evolved.
constexpr E::Input charizardAt36(G game,bool learned){
    auto a=charmander(game,false);
    const uint16_t old=game==G::FireRed || game==G::LeafGreen?232u:108u;
    auto& edge=a.evolutions[1];
    edge.allTargetSpeciesEvolutionOffersVerified=true;
    edge.evolutionOfferCount=1;
    edge.evolutionOffers[0]={36,17,learned?D::ReplaceSlot:D::Decline,2};
    if(learned) {
        a.phases[2].initialMoves={10,45,17,old};
        a.phases[2].observedMoves=a.phases[2].initialMoves;
    }
    return a;
}
static_assert(E::evaluate(charizardAt36(G::Ruby,true)).status==S::CandidateConsistent);
static_assert(E::evaluate(charizardAt36(G::LeafGreen,true)).status==S::CandidateConsistent);
static_assert(E::evaluate(charizardAt36(G::Emerald,false)).status==S::CandidateConsistent);
static_assert(E::evaluate(charizardAt36(G::FireRed,false)).status==S::CandidateConsistent);
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.evolutions[1].allTargetSpeciesEvolutionOffersVerified=false;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.evolutions[1].evolutionOfferCount=0;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.evolutions[1].evolutionOffers[0].offeredMove=97;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.evolutions[1].evolutionOffers[0].decision=D::AutoLearn;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.evolutions[1].evolutionOffers[0].slot=5;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.evolutions[1].evolutionOffers[0].level=37;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=charizardAt36(G::Ruby,true);
    a.phases[2].initialMoves={10,45,52,108};
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=charizardAt36(G::Ruby,false);
    a.evolutions[1].evolutionOffers[0].decision=D::AlreadyKnown;
    return E::evaluate(a).status==S::CandidateContradicted;}());

static_assert(E::evaluate(charmander(G::Ruby,true)).status==S::CandidateConsistent);
static_assert(E::evaluate(charmander(G::FireRed,true)).status==S::CandidateConsistent);
static_assert(E::evaluate(charmander(G::FireRed,false)).status==S::Unresolved);
static_assert(E::evaluate(charmander(G::Emerald,false)).status==S::Unresolved);
static_assert([]{auto a=bulbasaur(G::Ruby);a.sameOriginalPokemonAcrossStagesVerified=false;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.exactEvolutionSequenceVerified=false;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.evolutions[0].level=15;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.evolutions[1].toSpecies=6;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.evolutions[1].level=31;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.evolutions[0].independentlyVerifiedSourceEvolution=false;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.evolutions[0].noExtraMoveMutationOnEvolutionVerified=false;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[1].initialMoves={33,45,73,76};
    // Verified no other move mutation: invented carryover is a
    // contradiction to this exact candidate, not a global Invalid.
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[1].game=G::Emerald;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[0].stepCount=1;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[0].steps[0].offeredMove=76;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[2].steps[0].slot=2;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[2].observedMoves={33,45,73,22};
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=bulbasaur(G::Ruby);a.phases[1].nationalSpecies=133;
    return E::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=charmander(G::FireRed,true);
    a.phases[0].steps[0].offeredMove=108;
    return E::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=charmander(G::Emerald,true);
    a.phases[0].steps[0].offeredMove=232;
    return E::evaluate(a).status==S::CandidateContradicted;}());
int main(){
    constexpr std::array<G,5> sources{
        G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen};
    for(const auto game:sources) {
        for(const auto& chain:{bulbasaur(game),squirtle(game),
                              charmander(game,true)}) {
            const auto result=E::evaluate(chain);
            assert(result.status==S::CandidateConsistent);
            assert(result.verifiedPhases==3 &&
                   result.verifiedEvolutions==2);
        }
        assert(E::evaluate(charmander(game,false)).status==S::Unresolved);
        assert(E::evaluate(charizardAt36(game,true)).status==S::CandidateConsistent);
        assert(E::evaluate(charizardAt36(game,false)).status==S::CandidateConsistent);
    }
    std::cout<<"L3 source-pinned five-game Kanto starter evolution histories: PASS\n";
}
